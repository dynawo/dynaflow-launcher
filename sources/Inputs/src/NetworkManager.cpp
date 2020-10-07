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

#include <DYNBusInterface.h>
#include <DYNDataInterfaceFactory.h>
#include <DYNGeneratorInterface.h>
#include <DYNLineInterface.h>
#include <DYNLoadInterface.h>
#include <DYNNetworkInterface.h>
#include <DYNThreeWTransformerInterface.h>
#include <DYNTwoWTransformerInterface.h>
#include <DYNVoltageLevelInterface.h>
#include <boost/make_shared.hpp>

namespace dfl {
namespace inputs {

NetworkManager::NetworkManager(const std::string& filepath) :
    interface_(DYN::DataInterfaceFactory::build(DYN::DataInterfaceFactory::DATAINTERFACE_IIDM1, filepath)) {
  buildTree();
}

void
NetworkManager::buildTree() {
  auto network = interface_->getNetwork();

  const auto& voltageLevels = network->getVoltageLevels();
  for (auto it_v = voltageLevels.begin(); it_v != voltageLevels.end(); ++it_v) {
    if ((*it_v)->getVoltageLevelTopologyKind() == DYN::VoltageLevelInterface::BUS_BREAKER) {
      const auto& buses = (*it_v)->getBuses();
      for (auto it_b = buses.begin(); it_b != buses.end(); ++it_b) {
#if _DEBUG_
        // ids of nodes should be unique
        assert(nodes_.count((*it_b)->getID()) == 0);
#endif
        nodes_[(*it_b)->getID()] = std::make_shared<Node>((*it_b)->getID(), (*it_v)->getVNom());
      }

      const auto& loads = (*it_v)->getLoads();
      for (auto it_l = loads.begin(); it_l != loads.end(); ++it_l) {
        auto nodeid = (*it_l)->getBusInterface()->getID();
#if _DEBUG_
        // node should exist at this point
        assert(nodes_.count(nodeid));
#endif
        nodes_[nodeid]->loads.emplace_back((*it_l)->getID());
      }

      const auto& generators = (*it_v)->getGenerators();
      for (auto it_g = generators.begin(); it_g != generators.end(); ++it_g) {
        auto nodeid = (*it_g)->getBusInterface()->getID();
#if _DEBUG_
        // node should exist at this point
        assert(nodes_.count(nodeid));
#endif
        if ((*it_g)->isVoltageRegulationOn()) {
          // We don't use dynamic models for generators with voltage regulation disabled
          nodes_[nodeid]->generators.emplace_back((*it_g)->getID(), (*it_g)->getReactiveCurvesPoints());
        }
      }
    }
  }

  // perform connections
  const auto& lines = network->getLines();
  for (auto it_l = lines.begin(); it_l != lines.end(); ++it_l) {
    auto bus1 = (*it_l)->getBusInterface1();
    auto bus2 = (*it_l)->getBusInterface2();
    if (bus1 && bus2) {
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus2->getID()]);
      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
    }
  }

  const auto& transfos = network->getTwoWTransformers();
  for (auto it_t = transfos.begin(); it_t != transfos.end(); ++it_t) {
    auto bus1 = (*it_t)->getBusInterface1();
    auto bus2 = (*it_t)->getBusInterface2();
    if (bus1 && bus2) {
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus2->getID()]);
      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
    }
  }

  const auto& transfos_three = network->getThreeWTransformers();
  for (auto it_t = transfos_three.begin(); it_t != transfos_three.end(); ++it_t) {
    auto bus1 = (*it_t)->getBusInterface1();
    auto bus2 = (*it_t)->getBusInterface2();
    auto bus3 = (*it_t)->getBusInterface3();
    if (bus1 && bus2 && bus3) {
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus2->getID()]);
      nodes_[bus1->getID()]->neighbours.push_back(nodes_[bus3->getID()]);

      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
      nodes_[bus2->getID()]->neighbours.push_back(nodes_[bus3->getID()]);

      nodes_[bus3->getID()]->neighbours.push_back(nodes_[bus1->getID()]);
      nodes_[bus3->getID()]->neighbours.push_back(nodes_[bus2->getID()]);
    }
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

boost::optional<std::shared_ptr<Node>>
NetworkManager::getSlackNode() const {
  // TODO for now, we cannot retrieve from dynawo data interface this information: we will in a future version
  return boost::none;
}

}  // namespace inputs
}  // namespace dfl
