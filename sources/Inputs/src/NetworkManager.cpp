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

void
NetworkManager::updateMapRegulatingBuses(BusMapRegulating& map, const std::string& elementId, const boost::shared_ptr<DYN::DataInterface>& dataInterface) {
  auto regulatedBus = dataInterface->getServiceManager()->getRegulatedBus(elementId)->getID();
  auto it = map.find(regulatedBus);
  if (it == map.end()) {
    map.insert({regulatedBus, NbOfRegulating::ONE});
  } else {
    it->second = NbOfRegulating::MULTIPLES;
  }
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
      nodes_[nodeId] = Node::build(nodeId, vl, networkVL->getVNom(), (found != shuntsMap.end()) ? found->second : std::vector<Shunt>{}, bus->isFictitious(),
                                   interface_->getServiceManager());
      if (bus->isFictitious())
        LOG(debug, FictitiousNodeCreation, nodeId);
      else
        LOG(debug, NodeCreation, nodeId);
      if (opt_id && *opt_id == nodeId) {
        LOG(debug, SlackNodeFound, *opt_id);
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
      bool isNotInjecting = (DYN::doubleIsZero(load->getP0()) && DYN::doubleIsZero(load->getQ0()));
#if _DEBUG_
      // node should exist at this point
      assert(nodes_.count(nodeid));
#endif
      nodes_[nodeid]->loads.emplace_back(load->getID(), load->isFictitious(), isNotInjecting);
      LOG(debug, NodeContainsLoad, nodeid, load->getID());
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
      std::string regulatedBusId = "";
      auto regulatedBus = interface_->getServiceManager()->getRegulatedBus(generator->getID());
      if (regulatedBus) {
        regulatedBusId = regulatedBus->getID();
      }
      // we verify here that the generators is in voltage regulation to properly fill the map mapBusGeneratorBusId_.
      // This test is done also on algorithms.
      // The reason it is checked also here is to avoid to go through all the nodes later on
      if (generator->isVoltageRegulationOn()) {
        // We don't use dynamic models for generators with voltage regulation disabled
        updateMapRegulatingBuses(mapBusGeneratorsBusId_, generator->getID(), interface_);
      }
      nodes_[nodeid]->generators.emplace_back(generator->getID(), generator->isVoltageRegulationOn(), generator->getReactiveCurvesPoints(),
                                              generator->getQMin(), generator->getQMax(), pmin, pmax, targetP, generator->getBusInterface()->getVNom(),
                                              regulatedBusId, nodeid);
      LOG(debug, NodeContainsGen, nodeid, generator->getID());
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
        LOG(debug, NodeConnectionBySwitch, bus1->getID(), bus2->getID(), sw->getID());
      }
    }

    const auto& svarcs = networkVL->getStaticVarCompensators();
    for (const auto& svarc : svarcs) {
      if (!svarc->getInitialConnected()) {
        continue;
      }
      auto nodeid = svarc->getBusInterface()->getID();
      const bool isRegulatingVoltage = (svarc->getRegulationMode() == DYN::StaticVarCompensatorInterface::RegulationMode_t::OFF ||
                                        svarc->getRegulationMode() == DYN::StaticVarCompensatorInterface::RegulationMode_t::RUNNING_Q)
                                           ? false
                                           : true;
      auto regulatedBus = interface_->getServiceManager()->getRegulatedBus(svarc->getID());
      const double voltageSetPoint = isRegulatingVoltage ? svarc->getVSetPoint() : 0.;
      const bool hasStandByAutomaton = svarc->hasStandbyAutomaton();
      const double b0 = hasStandByAutomaton ? svarc->getB0() : 0.;
      const double uMinActivation = hasStandByAutomaton ? svarc->getUMinActivation() : 0.;
      const double uMaxActivation = hasStandByAutomaton ? svarc->getUMaxActivation() : 0.;
      const double uSetPointMin = hasStandByAutomaton ? svarc->getUSetPointMin() : 0.;
      const double uSetPointMax = hasStandByAutomaton ? svarc->getUSetPointMax() : 0.;
      nodes_[nodeid]->svarcs.emplace_back(svarc->getID(), isRegulatingVoltage, svarc->getBMin(), svarc->getBMax(), voltageSetPoint, svarc->getVNom(),
                                          uMinActivation, uMaxActivation, uSetPointMin, uSetPointMax, b0, svarc->getSlope(), svarc->hasStandbyAutomaton(),
                                          svarc->hasVoltagePerReactivePowerControl(), regulatedBus->getID(), nodeid, regulatedBus->getVNom());
      LOG(debug, NodeContainsSVC, nodeid, svarc->getID());
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
    if (line->getInitialConnected1() || line->getInitialConnected2()) {
#if _DEBUG_
      assert(nodes_.count(bus1->getID()) > 0);
      assert(nodes_.count(bus2->getID()) > 0);
#endif
      if (line->getInitialConnected1() && line->getInitialConnected2()) {
        LOG(debug, NodeConnectionByLine, bus1->getID(), bus2->getID(), line->getID());
      }
      auto season = line->getActiveSeason();
      auto new_line =
          Line::build(line->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), season, line->getInitialConnected1(), line->getInitialConnected2());
      lines_.push_back(new_line);
    }
  }

  const auto& transfos = network->getTwoWTransformers();
  for (const auto& transfo : transfos) {
    auto bus1 = transfo->getBusInterface1();
    auto bus2 = transfo->getBusInterface2();
    if (transfo->getInitialConnected1() || transfo->getInitialConnected2()) {
      auto tfo =
          Tfo::build(transfo->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), transfo->getInitialConnected1(), transfo->getInitialConnected2());
      tfos_.push_back(tfo);
      if (transfo->getInitialConnected1() && transfo->getInitialConnected2()) {
        LOG(debug, NodeConnectionBy2WT, bus1->getID(), bus2->getID(), transfo->getID());
      }
    }
  }

  const auto& transfos_three = network->getThreeWTransformers();
  for (const auto& transfo : transfos_three) {
    auto bus1 = transfo->getBusInterface1();
    auto bus2 = transfo->getBusInterface2();
    auto bus3 = transfo->getBusInterface3();
    if (transfo->getInitialConnected1() || transfo->getInitialConnected2() || transfo->getInitialConnected3()) {
      auto tfo = Tfo::build(transfo->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), nodes_.at(bus3->getID()), transfo->getInitialConnected1(),
                            transfo->getInitialConnected2(), transfo->getInitialConnected3());
      tfos_.push_back(tfo);
      if (transfo->getInitialConnected1() && transfo->getInitialConnected2() && transfo->getInitialConnected3()) {
        LOG(debug, NodeConnectionBy3WT, bus1->getID(), bus2->getID(), bus3->getID(), transfo->getID());
      }
    }
  }

  const auto& hvdcLines = network->getHvdcLines();
  for (const auto& hvdcLine : hvdcLines) {
    const auto& converterDyn1 = hvdcLine->getConverter1();
    const auto& converterDyn2 = hvdcLine->getConverter2();
    if (!converterDyn1->getInitialConnected() || !converterDyn2->getInitialConnected()) {
      continue;
    }

    boost::optional<double> powerFactor1 = boost::none;
    boost::optional<double> powerFactor2 = boost::none;

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
      powerFactor1 = lccConverterDyn1->getPowerFactor();
      powerFactor2 = lccConverterDyn2->getPowerFactor();
    }

    // active power control external IIDM extension
    const bool activePowerEnabled = hvdcLine->isActivePowerControlEnabled().get_value_or(false);
    auto activePowerControl =
        activePowerEnabled
            ? boost::optional<HvdcLine::ActivePowerControl>(HvdcLine::ActivePowerControl(hvdcLine->getDroop().value(), hvdcLine->getP0().value()))
            : boost::none;

    bool isConverter1Rectifier = false;
    if (hvdcLine->getConverterMode() == DYN::HvdcLineInterface::ConverterMode_t::RECTIFIER_INVERTER) {
      isConverter1Rectifier = true;
    }
    auto hvdcLineCreated =
        HvdcLine::build(hvdcLine->getID(),
                        converterType,
                        converter1,
                        converter2,
                        activePowerControl,
                        hvdcLine->getPmax(),
                        isConverter1Rectifier,
                        hvdcLine->getVNom(),
                        hvdcLine->getActivePowerSetpoint(),
                        hvdcLine->getResistanceDC(),
                        hvdcLine->getConverter1()->getLossFactor() / 100.,
                        hvdcLine->getConverter2()->getLossFactor() / 100.,
                        powerFactor1,
                        powerFactor2);
    hvdcLines_.emplace_back(hvdcLineCreated);
    nodes_[converterDyn1->getBusInterface()->getID()]->converters.push_back(converter1);
    nodes_[converterDyn2->getBusInterface()->getID()]->converters.push_back(converter2);
    LOG(debug, HvdcLineInNetwork, hvdcLine->getID(), hvdcLine->getIdConverter1(), hvdcLine->getIdConverter2());
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
