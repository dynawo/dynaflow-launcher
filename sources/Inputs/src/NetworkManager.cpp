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
#include <DYNDataInterfaceFactory.h>
#include <DYNGeneratorInterface.h>
#include <DYNHvdcLineInterface.h>
#include <DYNLccConverterInterface.h>
#include <DYNLineInterface.h>
#include <DYNLoadInterface.h>
#include <DYNNetworkInterface.h>
#include <DYNShuntCompensatorInterface.h>
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
NetworkManager::buildTree() {
  auto network = interface_->getNetwork();

  auto opt_id = network->getSlackNodeBusId();

  const auto& voltageLevels = network->getVoltageLevels();
  for (const auto& networkVL : voltageLevels) {
    const auto& shunts = networkVL->getShuntCompensators();
    std::map<Node::NodeId, std::vector<Shunt>> shuntsMap;
    for (const auto& shunt : shunts) {
      // We take into account even disconnected shunts as automatons may aim to connect them
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
      nodes_[nodeId] = Node::build(nodeId, vl, networkVL->getVNom(), (found != shuntsMap.end()) ? found->second : std::vector<Shunt>{});
      LOG(debug) << "Node " << nodeId << " created" << LOG_ENDL;
      if (opt_id && *opt_id == nodeId) {
        LOG(debug) << "Slack node with id " << *opt_id << " found in network" << LOG_ENDL;
        slackNode_ = nodes_[nodeId];
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
      if (generator->isVoltageRegulationOn() && (DYN::doubleEquals(-targetP, pmin) || -targetP > pmin) &&
          (DYN::doubleEquals(-targetP, pmax) || -targetP < pmax)) {
        // We don't use dynamic models for generators with voltage regulation disabled and an active power reference outside the generator's PQ diagram
        auto regulated_bus = interface_->getServiceManager()->getRegulatedBus(generator->getID())->getID();
        auto it = mapBusId_.find(regulated_bus);
        if (it == mapBusId_.end()) {
          mapBusId_.insert({regulated_bus, NbOfRegulatingGenerators::ONE});
        } else {
          it->second = NbOfRegulatingGenerators::MULTIPLES;
        }

        nodes_[nodeid]->generators.emplace_back(generator->getID(), generator->getReactiveCurvesPoints(), generator->getQMin(), generator->getQMax(), pmin,
                                                pmax, targetP, regulated_bus, nodeid);
        LOG(debug) << "Node " << nodeid << " contains generator " << generator->getID() << LOG_ENDL;
      }
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
      auto new_line = Line::build(line->getID(), nodes_.at(bus1->getID()), nodes_.at(bus2->getID()));
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
    const auto& converter_dyn_1 = hvdcLine->getConverter1();
    const auto& converter_dyn_2 = hvdcLine->getConverter2();
    if (!converter_dyn_1->getInitialConnected() || !converter_dyn_2->getInitialConnected()) {
      continue;
    }
    auto converter_1 = ConverterInterface(converter_dyn_1->getID(), converter_dyn_1->getBusInterface()->getID());
    auto converter_2 = ConverterInterface(converter_dyn_2->getID(), converter_dyn_2->getBusInterface()->getID());

    HvdcLine::ConverterType converterType;
    if (converter_dyn_1->getConverterType() == DYN::ConverterInterface::ConverterType_t::VSC_CONVERTER) {
      converterType = HvdcLine::ConverterType::VSC;
      auto vsc_converter_dyn_1 = boost::static_pointer_cast<DYN::VscConverterInterface>(converter_dyn_1);
      converter_1.voltageRegulationOn = vsc_converter_dyn_1->getVoltageRegulatorOn();

      auto vsc_converter_dyn_2 = boost::static_pointer_cast<DYN::VscConverterInterface>(converter_dyn_2);
      converter_2.voltageRegulationOn = vsc_converter_dyn_2->getVoltageRegulatorOn();
    } else {
      converterType = HvdcLine::ConverterType::LCC;
    }
    auto hvdcLineCreated =
        std::make_shared<dfl::inputs::HvdcLine>(hvdcLine->getID(), converterType, converter_1.converterId, converter_1.busId, converter_1.voltageRegulationOn,
                                                converter_2.converterId, converter_2.busId, converter_2.voltageRegulationOn);
    hvdcLines_.emplace_back(hvdcLineCreated);
    converter_1.hvdcLine = hvdcLineCreated;
    converter_2.hvdcLine = hvdcLineCreated;
    nodes_[converter_dyn_1->getBusInterface()->getID()]->converterInterfaces.push_back(converter_1);
    nodes_[converter_dyn_2->getBusInterface()->getID()]->converterInterfaces.push_back(converter_2);
    LOG(debug) << "Network contains hvdcLine " << hvdcLine->getID() << " with converterStation " << hvdcLine->getIdConverter1() << " and converterStation "
               << hvdcLine->getIdConverter2() << LOG_ENDL;
  }
}

void
NetworkManager::walkNodes() {
  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    for (auto it_c = nodesCallbacks_.begin(); it_c != nodesCallbacks_.end(); ++it_c) {
      (*it_c)(it->second);
    }
  }
}

}  // namespace inputs
}  // namespace dfl
