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

namespace dfl {
namespace inputs {

NetworkManager::NetworkManager(const boost::filesystem::path &filepath)
    : interface_(DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, filepath.generic_string())), slackNode_{}, nodes_{},
      nodesCallbacks_{}, isPartiallyConditioned_(false), isFullyConditioned_(true) {
  buildTree();
}

void NetworkManager::updateMapRegulatingBuses(BusMapRegulating &map, const std::shared_ptr<Node> &node) {
  std::vector<std::string> busesConnected = node->getBusesConnectedByVoltageLevel();
  busesConnected.push_back(node->id);
  for (const std::string &busId : busesConnected) {
    auto it = map.find(busId);
    if (it == map.end()) {
      map.insert({busId, NbOfRegulating::ONE});
    } else {
      it->second = NbOfRegulating::MULTIPLES;
    }
  }
}

void NetworkManager::updateConditioningStatus(const std::shared_ptr<DYN::ComponentInterface> &componentInterface) {
  if (componentInterface->hasInitialConditions()) {
    isPartiallyConditioned_ = true;
  } else {
    isFullyConditioned_ = false;
  }
}

void NetworkManager::buildTree() {
  auto network = interface_->getNetwork();

  auto opt_id = network->getSlackNodeBusId();

  const auto &voltageLevels = network->getVoltageLevels();
  // We first initialize all nodes
  for (const auto &networkVL : voltageLevels) {
    const auto &shunts = networkVL->getShuntCompensators();
    std::unordered_map<Node::NodeId, std::vector<Shunt>> shuntsMap;
    for (const auto &shunt : shunts) {
      updateConditioningStatus(shunt);
      // We take into account even disconnected shunts as dynamic models may aim to connect them
      (shuntsMap[shunt->getBusInterface()->getID()]).push_back(std::move(Shunt(shunt->getID())));
    }

    auto vl = std::make_shared<VoltageLevel>(networkVL->getID());
    voltagelevels_.push_back(vl);

    const auto &buses = networkVL->getBuses();
    for (const auto &bus : buses) {
      updateConditioningStatus(bus);
      const auto &nodeId = bus->getID();
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

      for (const auto &busBarSection : bus->getBusBarSectionIdentifiers()) {
        nodes_[bus->getID()]->busBarSections.emplace_back(busBarSection);
      }
    }
  }
  // And then we update others elements
  for (const auto &networkVL : voltageLevels) {
    const auto &shunts = networkVL->getShuntCompensators();
    std::unordered_map<Node::NodeId, std::vector<Shunt>> shuntsMap;
    for (const auto &shunt : shunts) {
      updateConditioningStatus(shunt);
      // We take into account even disconnected shunts as dynamic models may aim to connect them
      (shuntsMap[shunt->getBusInterface()->getID()]).push_back(std::move(Shunt(shunt->getID())));
    }

    const auto &loads = networkVL->getLoads();
    for (const auto &load : loads) {
      updateConditioningStatus(load);
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

    const auto &generators = networkVL->getGenerators();
    for (const auto &generator : generators) {
      updateConditioningStatus(generator);
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
        updateMapRegulatingBuses(mapBusIdToNumberOfRegulation_, nodes_[regulatedBusId]);
      }
      nodes_[nodeid]->generators.emplace_back(generator->getID(), generator->isVoltageRegulationOn(), generator->getReactiveCurvesPoints(),
                                              generator->getQMin(), generator->getQMax(), pmin, pmax, -generator->getQ(), targetP,
                                              generator->getBusInterface()->getVNom(), regulatedBusId, nodeid,
                                              generator->getEnergySource() == DYN::GeneratorInterface::SOURCE_NUCLEAR, generator->hasActivePowerControl());
      LOG(debug, NodeContainsGen, nodeid, generator->getID());
    }

    const auto &switches = networkVL->getSwitches();
    for (const auto &sw : switches) {
      if (sw->isOpen() && sw->isRetained()) {
        // only keep opened retained switch if they are connected to a shunt
        auto bus1 = sw->getBusInterface1();
        auto bus2 = sw->getBusInterface2();
        auto found1 = shuntsMap.find(bus1->getID());
        auto found2 = shuntsMap.find(bus1->getID());
        if (found1 == shuntsMap.end() && found2 == shuntsMap.end())
          continue;
      }
      if (!sw->isOpen() || sw->isRetained()) {
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

    const auto &svarcs = networkVL->getStaticVarCompensators();
    for (const auto &svarc : svarcs) {
      updateConditioningStatus(svarc);
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
      LOG(debug, NodeContainsSVarC, nodeid, svarc->getID());
    }

    const auto &dangling_lines = networkVL->getDanglingLines();
    for (const auto &dline : dangling_lines) {
      updateConditioningStatus(dline);
      nodes_[dline->getBusInterface()->getID()]->danglingLines.emplace_back(dline->getID());
    }
  }

  // perform connections
  const auto &lines = network->getLines();
  for (const auto &line : lines) {
    updateConditioningStatus(line);
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

  const auto &transfos = network->getTwoWTransformers();
  for (const auto &transfo : transfos) {
    updateConditioningStatus(transfo);
    auto bus1 = transfo->getBusInterface1();
    auto bus2 = transfo->getBusInterface2();
    if (transfo->getInitialConnected1() || transfo->getInitialConnected2()) {
      auto tfo = Tfo::build(transfo->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), transfo->getActiveSeason(), transfo->getInitialConnected1(),
                            transfo->getInitialConnected2());
      tfos_.push_back(tfo);
      if (transfo->getInitialConnected1() && transfo->getInitialConnected2()) {
        LOG(debug, NodeConnectionBy2WT, bus1->getID(), bus2->getID(), transfo->getID());
      }
    }
  }

  const auto &transfos_three = network->getThreeWTransformers();
  for (const auto &transfo : transfos_three) {
    auto bus1 = transfo->getBusInterface1();
    auto bus2 = transfo->getBusInterface2();
    auto bus3 = transfo->getBusInterface3();
    if (transfo->getInitialConnected1() || transfo->getInitialConnected2() || transfo->getInitialConnected3()) {
      auto tfo = Tfo::build(transfo->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()), nodes_.at(bus3->getID()), transfo->getActiveSeason(),
                            transfo->getInitialConnected1(), transfo->getInitialConnected2(), transfo->getInitialConnected3());
      tfos_.push_back(tfo);
      if (transfo->getInitialConnected1() && transfo->getInitialConnected2() && transfo->getInitialConnected3()) {
        LOG(debug, NodeConnectionBy3WT, bus1->getID(), bus2->getID(), bus3->getID(), transfo->getID());
      }
    }
  }

  const auto &hvdcLines = network->getHvdcLines();
  for (const auto &hvdcLine : hvdcLines) {
    const auto &converterDyn1 = hvdcLine->getConverter1();
    const auto &converterDyn2 = hvdcLine->getConverter2();
    if (!converterDyn1->getInitialConnected() || !converterDyn2->getInitialConnected()) {
      continue;
    }
    std::shared_ptr<Converter> converter1;
    std::shared_ptr<Converter> converter2;

    HvdcLine::ConverterType converterType;
    if (converterDyn1->getConverterType() == DYN::ConverterInterface::ConverterType_t::VSC_CONVERTER) {
      converterType = HvdcLine::ConverterType::VSC;
      auto vscConverterDyn1 = std::dynamic_pointer_cast<DYN::VscConverterInterface>(converterDyn1);
      updateConditioningStatus(vscConverterDyn1);
      bool voltageRegulationOn = vscConverterDyn1->getVoltageRegulatorOn();
      converter1 = std::make_shared<VSCConverter>(converterDyn1->getID(), converterDyn1->getBusInterface()->getID(), nullptr, voltageRegulationOn,
                                                  vscConverterDyn1->getQMax(), vscConverterDyn1->getQMin(), vscConverterDyn1->getQ(),
                                                  vscConverterDyn1->getReactiveCurvesPoints());
      if (voltageRegulationOn) {
        updateMapRegulatingBuses(mapBusIdToNumberOfRegulation_, nodes_[converterDyn1->getBusInterface()->getID()]);
      }
      auto vscConverterDyn2 = std::dynamic_pointer_cast<DYN::VscConverterInterface>(converterDyn2);
      updateConditioningStatus(vscConverterDyn2);
      voltageRegulationOn = vscConverterDyn2->getVoltageRegulatorOn();
      converter2 = std::make_shared<VSCConverter>(converterDyn2->getID(), converterDyn2->getBusInterface()->getID(), nullptr, voltageRegulationOn,
                                                  vscConverterDyn2->getQMax(), vscConverterDyn2->getQMin(), vscConverterDyn2->getQ(),
                                                  vscConverterDyn2->getReactiveCurvesPoints());
      if (voltageRegulationOn) {
        updateMapRegulatingBuses(mapBusIdToNumberOfRegulation_, nodes_[converterDyn2->getBusInterface()->getID()]);
      }
    } else {
      converterType = HvdcLine::ConverterType::LCC;
      auto lccConverterDyn1 = std::dynamic_pointer_cast<DYN::LccConverterInterface>(converterDyn1);
      updateConditioningStatus(lccConverterDyn1);
      converter1 =
          std::make_shared<LCCConverter>(converterDyn1->getID(), converterDyn1->getBusInterface()->getID(), nullptr, lccConverterDyn1->getPowerFactor());

      auto lccConverterDyn2 = std::dynamic_pointer_cast<DYN::LccConverterInterface>(converterDyn2);
      updateConditioningStatus(lccConverterDyn2);
      converter2 =
          std::make_shared<LCCConverter>(converterDyn2->getID(), converterDyn2->getBusInterface()->getID(), nullptr, lccConverterDyn2->getPowerFactor());
    }

    // active power control external IIDM extension
    const bool activePowerEnabled = hvdcLine->isActivePowerControlEnabled().get_value_or(false);
    if (activePowerEnabled && DYN::doubleIsZero(hvdcLine->getDroop().value()))
      LOG(warn, HvdcActivePowerControlActivatedNoDroop, hvdcLine->getID());
    auto activePowerControl =
        activePowerEnabled
            ? boost::optional<HvdcLine::ActivePowerControl>(HvdcLine::ActivePowerControl(hvdcLine->getDroop().value(), hvdcLine->getP0().value()))
            : boost::none;

    bool isConverter1Rectifier = false;
    if (hvdcLine->getConverterMode() == DYN::HvdcLineInterface::ConverterMode_t::RECTIFIER_INVERTER) {
      isConverter1Rectifier = true;
    }
    std::array<double, 2> lossFactors = {converterDyn1->getLossFactor() / 100., converterDyn2->getLossFactor() / 100.};
    auto hvdcLineCreated =
        HvdcLine::build(hvdcLine->getID(), converterType, converter1, converter2, activePowerControl, hvdcLine->getPmax(), isConverter1Rectifier,
                        hvdcLine->getVNom(), hvdcLine->getActivePowerSetpoint(), hvdcLine->getResistanceDC(), lossFactors);
    hvdcLines_.emplace_back(hvdcLineCreated);
    nodes_[converterDyn1->getBusInterface()->getID()]->converters.push_back(converter1);
    nodes_[converterDyn2->getBusInterface()->getID()]->converters.push_back(converter2);
    LOG(debug, HvdcLineInNetwork, hvdcLine->getID(), hvdcLine->getIdConverter1(), hvdcLine->getIdConverter2());
  }
}

void NetworkManager::walkNodes() const {
  for (const auto &node : nodes_) {
    for (const auto &cbk : nodesCallbacks_) {
      cbk(node.second);
    }
  }
}

std::unordered_set<std::shared_ptr<Converter>> NetworkManager::computeVSCConverters() const {
  std::unordered_set<std::shared_ptr<Converter>> ret;
  for (const auto &hvdcLine : hvdcLines_) {
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
