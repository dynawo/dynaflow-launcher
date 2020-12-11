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
#include <DYNConverterInterface.h>
#include <DYNDataInterfaceFactory.h>
#include <DYNGeneratorInterface.h>
#include <DYNHvdcLineInterface.h>
#include <DYNLccConverterInterface.h>
#include <DYNLineInterface.h>
#include <DYNLoadInterface.h>
#include <DYNNetworkInterface.h>
#include <DYNThreeWTransformerInterface.h>
#include <DYNTwoWTransformerInterface.h>
#include <DYNVoltageLevelInterface.h>
#include <DYNVscConverterInterface.h>
#include <boost/make_shared.hpp>

namespace dfl {
namespace inputs {

NetworkManager::NetworkManager(const std::string& filepath) :
    interface_(DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM, filepath)),
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
  for (auto it_v = voltageLevels.begin(); it_v != voltageLevels.end(); ++it_v) {
    if ((*it_v)->getVoltageLevelTopologyKind() == DYN::VoltageLevelInterface::NODE_BREAKER) {
      LOG(error) << MESS(NodeBreakerInterface, (*it_v)->getID()) << LOG_ENDL;
      // This is done in class constructor so we exit directly
      std::exit(EXIT_FAILURE);
    }

    const auto& buses = (*it_v)->getBuses();
    for (auto it_b = buses.begin(); it_b != buses.end(); ++it_b) {
#if _DEBUG_
      // ids of nodes should be unique
      assert(nodes_.count((*it_b)->getID()) == 0);
#endif
      nodes_[(*it_b)->getID()] = std::make_shared<Node>((*it_b)->getID(), (*it_v)->getVNom());
      LOG(debug) << "Node " << (*it_b)->getID() << " created" << LOG_ENDL;
      if (opt_id && *opt_id == (*it_b)->getID()) {
        LOG(debug) << "Slack node with id " << *opt_id << " found in network" << LOG_ENDL;
        slackNode_ = nodes_[(*it_b)->getID()];
      }
    }

    const auto& loads = (*it_v)->getLoads();
    for (auto it_l = loads.begin(); it_l != loads.end(); ++it_l) {
      // if load is not connected, it is ignored
      if (!(*it_l)->getInitialConnected())
        continue;
      auto nodeid = (*it_l)->getBusInterface()->getID();
#if _DEBUG_
      // node should exist at this point
      assert(nodes_.count(nodeid));
#endif
      nodes_[nodeid]->loads.emplace_back((*it_l)->getID());
      LOG(debug) << "Node " << nodeid << " contains load " << (*it_l)->getID() << LOG_ENDL;
    }

    const auto& generators = (*it_v)->getGenerators();
    for (auto it_g = generators.begin(); it_g != generators.end(); ++it_g) {
      // if generator is not connected, it is ignored
      if (!(*it_g)->getInitialConnected())
        continue;
      auto nodeid = (*it_g)->getBusInterface()->getID();
#if _DEBUG_
      // node should exist at this point
      assert(nodes_.count(nodeid));
#endif
      if ((*it_g)->isVoltageRegulationOn()) {
        // We don't use dynamic models for generators with voltage regulation disabled
        nodes_[nodeid]->generators.emplace_back((*it_g)->getID(), (*it_g)->getReactiveCurvesPoints(), (*it_g)->getQMin(), (*it_g)->getQMax(),
                                                (*it_g)->getPMin(), (*it_g)->getPMax());
        LOG(debug) << "Node " << nodeid << " contains generator " << (*it_g)->getID() << LOG_ENDL;
      }
    }
  }

  // perform connections
  const auto& lines = network->getLines();
  for (auto it_l = lines.begin(); it_l != lines.end(); ++it_l) {
    auto bus1 = (*it_l)->getBusInterface1();
    auto bus2 = (*it_l)->getBusInterface2();
    if ((*it_l)->getInitialConnected1() && (*it_l)->getInitialConnected2()) {
      auto id = bus1->getID();
#if _DEBUG_
      assert(nodes_.count(bus1->getID()) > 0);
      assert(nodes_.count(bus2->getID()) > 0);
#endif
      nodes_[bus1->getID()]->neighbours.push_back(nodes_.at(bus2->getID()));
      nodes_[bus2->getID()]->neighbours.push_back(nodes_.at(bus1->getID()));
      LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " by line " << (*it_l)->getID() << LOG_ENDL;
    }
  }

  const auto& transfos = network->getTwoWTransformers();
  for (auto it_t = transfos.begin(); it_t != transfos.end(); ++it_t) {
    auto bus1 = (*it_t)->getBusInterface1();
    auto bus2 = (*it_t)->getBusInterface2();
    if ((*it_t)->getInitialConnected1() && (*it_t)->getInitialConnected2()) {
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus2->getID()]);
      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
      LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " by 2W " << (*it_t)->getID() << LOG_ENDL;
    }
  }

  const auto& transfos_three = network->getThreeWTransformers();
  for (auto it_t = transfos_three.begin(); it_t != transfos_three.end(); ++it_t) {
    auto bus1 = (*it_t)->getBusInterface1();
    auto bus2 = (*it_t)->getBusInterface2();
    auto bus3 = (*it_t)->getBusInterface3();
    if ((*it_t)->getInitialConnected1() && (*it_t)->getInitialConnected2() && (*it_t)->getInitialConnected3()) {
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus2->getID()]);
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus3->getID()]);

      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus3->getID()]);

      nodes_[bus3->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
      nodes_[bus3->getID()]->neighbours.push_back(nodes_[bus2->getID()]);

      LOG(debug) << "Node " << bus1->getID() << " connected to " << bus2->getID() << " and " << bus3->getID() << " by 3W " << (*it_t)->getID() << LOG_ENDL;
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
