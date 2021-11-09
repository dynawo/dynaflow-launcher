//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  NetworkManager.cpp
 *
 * @brief Network manager implementation file
 *
 */

#include "NetworkManager.h"

#include "Log.h"
#include "Message.hpp"

#include <DYNBusInterface.h>
#include <DYNCommon.h>
#include <DYNConverterInterface.h>
#include <DYNDanglingLineInterface.h>
#include <DYNDataInterfaceFactory.h>
#include <DYNGeneratorInterface.h>
#include <DYNHvdcLineInterface.h>
#include <DYNLccConverterInterface.h>
#include <DYNLineInterface.h>
#include <DYNLoadInterface.h>
#include <DYNNetworkInterface.h>
#include <DYNShuntCompensatorInterface.h>
#include <DYNStaticVarCompensatorInterface.h>
#include <DYNSwitchInterface.h>
#include <DYNThreeWTransformerInterface.h>
#include <DYNTwoWTransformerInterface.h>
#include <DYNVoltageLevelInterface.h>
#include <DYNVscConverterInterface.h>
#include <boost/make_shared.hpp>

namespace dfl {
namespace inputs {

NetworkManager::NetworkManager(const boost::filesystem::path& filepath) :
    interface_(DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, filepath.generic_string())),
    slackNode_{},
    nodes_{},
    nodesCallbacks_{} {
  buildTree();
}

auto
NetworkManager::updateMapRegulatingBuses(BusMapRegulating& map, const std::string& elementId, const boost::shared_ptr<DYN::DataInterface>& dataInterface)
    -> BusId {
  auto regulatedBus = dataInterface->getServiceManager()->getRegulatedBus(elementId)->getID();
  auto it = map.find(regulatedBus);
  if (it == map.end()) {
    map.insert({regulatedBus, NbOfRegulating::ONE});
  } else {
    it->second = NbOfRegulating::MULTIPLES;
  }
  return regulatedBus;
}

void
NetworkManager::buildTree() {
  auto network = interface_->getNetwork();

  auto opt_id = network->getSlackNodeBusId();

  const auto& voltageLevels = network->getVoltageLevels();
  for (const auto& networkVL : voltageLevels) {
    const auto& shunts = networkVL->getShuntCompensators();
    std::unordered_map<Node::NodeId, std::vector<Shunt>> shuntsMap;
    for (const auto& shunt : shunts) {
      // We take into account even disconnected shunts as dynamic models may aim to connect them
      (shuntsMap[shunt->getBusInterface()->getID()]).push_back(std::move(Shunt(shunt->getID())));
    }

    auto vl = std::make_shared<VoltageLevel>(networkVL->getID());
    voltagelevels_.push_back(vl);

    const auto& buses = networkVL->getBuses();
    for (const auto& bus : buses) {
      const auto& nodeId = bus->getID();
#if _DEBUG_
      // ids of nodes should be unique
      assert(nodes_.count(nodeId) == 0);
#endif
      auto found = shuntsMap.find(nodeId);
      nodes_[nodeId] = Node::build(nodeId, vl, networkVL->getVNom(), (found != shuntsMap.end()) ? found->second : std::vector<Shunt>{}, bus->isFictitious());
      LOG(debug) << "Node " << nodeId << " created" << (bus->isFictitious() ? " (fictitious)" : "") << LOG_ENDL;
      if (opt_id && *opt_id == nodeId) {
        LOG(debug) << "Slack node with id " << *opt_id << " found in network" << LOG_ENDL;
        slackNode_ = nodes_[nodeId];
      }

      for (const auto& busBarSection : bus->getBusBarSectionIdentifiers()) {
        nodes_[bus->getID()]->busBarSections.emplace_back(busBarSection);
      }
    }

    const auto& loads = networkVL->getLoads();
    for (const auto& load : loads) {
      // if load is not connected, it is ignored
      if (!load->getInitialConnected())
        continue;
      auto nodeid = load->getBusInterface()->getID();
#if _DEBUG_
      // node should exist at this point
      assert(nodes_.count(nodeid));
#endif
      nodes_[nodeid]->loads.emplace_back(load->getID());
      LOG(debug) << "Node " << nodeid << " contains load " << load->getID() << LOG_ENDL;
    }

    const auto& generators = networkVL->getGenerators();
    for (const auto& generator : generators) {
      // if generator is not connected, it is ignored
      if (!generator->getInitialConnected())
        continue;
      auto nodeid = generator->getBusInterface()->getID();
#if _DEBUG_
      // node should exist at this point
      assert(nodes_.count(nodeid));
#endif
      auto targetP = generator->getTargetP();
      auto pmin = generator->getPMin();
      auto pmax = generator->getPMax();
      bool defineGeneratorModel = false;
      boost::optional<BusId> regulated_bus;
      if (generator->isVoltageRegulationOn() && (DYN::doubleEquals(-targetP, pmin) || -targetP > pmin) &&
          (DYN::doubleEquals(-targetP, pmax) || -targetP < pmax)) {
        // We don't use dynamic models for generators with voltage regulation disabled and an active power reference outside the generator's PQ diagram
        regulated_bus = boost::make_optional(updateMapRegulatingBuses(mapBusGeneratorsBusId_, generator->getID(), interface_));
        defineGeneratorModel = true;
      } else {
        LOG(warn) << MESS(GeneratorVRegDisabledOrBadP, generator->getID()) << LOG_ENDL;
      }
      nodes_[nodeid]->generators.emplace_back(generator->getID(), generator->getReactiveCurvesPoints(), generator->getQMin(), generator->getQMax(), pmin, pmax,
                                              targetP, regulated_bus, nodeid, defineGeneratorModel);
      LOG(debug) << "Node " << nodeid << " contains generator " << generator->getID() << LOG_ENDL;
    }

    const auto& switches = networkVL->getSwitches();
    for (const auto& sw : switches) {
      if (!sw->isOpen()) {
        auto bus1 = sw->getBusInterface1();
        auto bus2 = sw->getBusInterface2();
#ifdef _DEBUG_
        // By construction buses in switches are all inside the voltage level, so the nodes already exist
        assert(nodes_.count(bus1->getID()) > 0);
        assert(nodes_.count(bus2->getID()) > 0);
#endif
        nodes_[bus1->getID()]->neighbours.push_back(nodes_.at(bus2->getID()));
        nodes_[bus2->getID()]->neighbours.push_back(nodes_.at(bus1->getID()));
        LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " by switch " << sw->getID() << LOG_ENDL;
      }
    }

    const auto& svarcs = networkVL->getStaticVarCompensators();
    for (const auto& svarc : svarcs) {
      bool defineSvarcModel = true;
      if (!svarc->getInitialConnected()) {
        LOG(warn) << MESS(SVarCInitiallyDisconnected, svarc->getID()) << LOG_ENDL;
        defineSvarcModel = false;
      } else if (!svarc->hasStandbyAutomaton()) {
        LOG(warn) << MESS(SVarCIIDMExtensionNotFound, "standByAutomaton", svarc->getID()) << LOG_ENDL;
        defineSvarcModel = false;
      } else if (!svarc->hasVoltagePerReactivePowerControl()) {
        LOG(warn) << MESS(SVarCIIDMExtensionNotFound, "voltagePerReactivePowerControl", svarc->getID()) << LOG_ENDL;
        defineSvarcModel = false;
      }
      auto nodeid = svarc->getBusInterface()->getID();
      nodes_[nodeid]->svarcs.emplace_back(svarc->getID(), svarc->getBMin(), svarc->getBMax(), svarc->getVSetPoint(), svarc->getVNom(),
                                          svarc->getUMinActivation(), svarc->getUMaxActivation(), svarc->getUSetPointMin(), svarc->getUSetPointMax(),
                                          svarc->getB0(), svarc->getSlope(), defineSvarcModel);
      LOG(debug) << "Node " << nodeid << " contains static var compensator " << svarc->getID() << LOG_ENDL;
    }

    const auto& dangling_lines = networkVL->getDanglingLines();
    for (const auto& dline : dangling_lines) {
      nodes_[dline->getBusInterface()->getID()]->danglingLines.emplace_back(dline->getID());
    }
  }

  // perform connections
  const auto& lines = network->getLines();
  for (const auto& line : lines) {
    auto bus1 = line->getBusInterface1();
    auto bus2 = line->getBusInterface2();
    if (line->getInitialConnected1() && line->getInitialConnected2()) {
#if _DEBUG_
      assert(nodes_.count(bus1->getID()) > 0);
      assert(nodes_.count(bus2->getID()) > 0);
#endif
      LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " by line " << line->getID() << LOG_ENDL;
      auto season = line->getActiveSeason();
      auto new_line = Line::build(line->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), season);
      lines_.push_back(new_line);
    }
  }

  const auto& transfos = network->getTwoWTransformers();
  for (const auto& transfo : transfos) {
    auto bus1 = transfo->getBusInterface1();
    auto bus2 = transfo->getBusInterface2();
    if (transfo->getInitialConnected1() && transfo->getInitialConnected2()) {
      auto tfo = Tfo::build(transfo->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()));
      tfos_.push_back(tfo);

      LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " by 2W " << transfo->getID() << LOG_ENDL;
    }
  }

  const auto& transfos_three = network->getThreeWTransformers();
  for (const auto& transfo : transfos_three) {
    auto bus1 = transfo->getBusInterface1();
    auto bus2 = transfo->getBusInterface2();
    auto bus3 = transfo->getBusInterface3();
    if (transfo->getInitialConnected1() && transfo->getInitialConnected2() && transfo->getInitialConnected3()) {
      auto tfo = Tfo::build(transfo->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), nodes_.at(bus3->getID()));
      tfos_.push_back(tfo);

      LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " and " << bus3->getID() << " by 3W " << transfo->getID() << LOG_ENDL;
    }
  }

  const auto& hvdcLines = network->getHvdcLines();
  for (const auto& hvdcLine : hvdcLines) {
    const auto& converterDyn1 = hvdcLine->getConverter1();
    const auto& converterDyn2 = hvdcLine->getConverter2();
    if (!converterDyn1->getInitialConnected() || !converterDyn2->getInitialConnected()) {
      continue;
    }
    std::shared_ptr<Converter> converter1;
    std::shared_ptr<Converter> converter2;

    HvdcLine::ConverterType converterType;
    if (converterDyn1->getConverterType() == DYN::ConverterInterface::ConverterType_t::VSC_CONVERTER) {
      converterType = HvdcLine::ConverterType::VSC;
      auto vscConverterDyn1 = boost::dynamic_pointer_cast<DYN::VscConverterInterface>(converterDyn1);
      bool voltageRegulationOn = vscConverterDyn1->getVoltageRegulatorOn();
      converter1 = std::make_shared<VSCConverter>(converterDyn1->getID(), converterDyn1->getBusInterface()->getID(), nullptr, voltageRegulationOn,
                                                  vscConverterDyn1->getQMax(), vscConverterDyn1->getQMin(), vscConverterDyn1->getReactiveCurvesPoints());
      updateMapRegulatingBuses(mapBusVSCConvertersBusId_, converterDyn1->getID(), interface_);

      auto vscConverterDyn2 = boost::dynamic_pointer_cast<DYN::VscConverterInterface>(converterDyn2);
      voltageRegulationOn = vscConverterDyn2->getVoltageRegulatorOn();
      converter2 = std::make_shared<VSCConverter>(converterDyn2->getID(), converterDyn2->getBusInterface()->getID(), nullptr, voltageRegulationOn,
                                                  vscConverterDyn2->getQMax(), vscConverterDyn2->getQMin(), vscConverterDyn2->getReactiveCurvesPoints());
      updateMapRegulatingBuses(mapBusVSCConvertersBusId_, converterDyn2->getID(), interface_);
    } else {
      converterType = HvdcLine::ConverterType::LCC;
      auto lccConverterDyn1 = boost::dynamic_pointer_cast<DYN::LccConverterInterface>(converterDyn1);
      converter1 =
          std::make_shared<LCCConverter>(converterDyn1->getID(), converterDyn1->getBusInterface()->getID(), nullptr, lccConverterDyn1->getPowerFactor());

      auto lccConverterDyn2 = boost::dynamic_pointer_cast<DYN::LccConverterInterface>(converterDyn2);
      converter2 =
          std::make_shared<LCCConverter>(converterDyn2->getID(), converterDyn2->getBusInterface()->getID(), nullptr, lccConverterDyn2->getPowerFactor());
    }

    // active power control external IIDM extension
    const bool activePowerEnabled = hvdcLine->isActivePowerControlEnabled().get_value_or(false);
    auto activePowerControl =
        activePowerEnabled
            ? boost::optional<HvdcLine::ActivePowerControl>(HvdcLine::ActivePowerControl(hvdcLine->getDroop().value(), hvdcLine->getP0().value()))
            : boost::none;

    auto hvdcLineCreated = HvdcLine::build(hvdcLine->getID(), converterType, converter1, converter2, activePowerControl, hvdcLine->getPmax());
    hvdcLines_.emplace_back(hvdcLineCreated);
    nodes_[converterDyn1->getBusInterface()->getID()]->converters.push_back(converter1);
    nodes_[converterDyn2->getBusInterface()->getID()]->converters.push_back(converter2);
    LOG(debug) << "Network contains hvdcLine " << hvdcLine->getID() << " with converterStation " << hvdcLine->getIdConverter1() << " and converterStation "
               << hvdcLine->getIdConverter2() << LOG_ENDL;
  }
}

void
NetworkManager::walkNodes() const {
  for (const auto& node : nodes_) {
    for (const auto& cbk : nodesCallbacks_) {
      cbk(node.second);
    }
  }
}

std::unordered_set<std::shared_ptr<Converter>>
NetworkManager::computeVSCConverters() const {
  std::unordered_set<std::shared_ptr<Converter>> ret;
  for (const auto& hvdcLine : hvdcLines_) {
    if (hvdcLine->converterType != HvdcLine::ConverterType::VSC) {
      continue;
    }
    ret.insert(hvdcLine->converter1);
    ret.insert(hvdcLine->converter2);
  }
  return ret;
}

}  // namespace inputs
}  // namespace dfl
