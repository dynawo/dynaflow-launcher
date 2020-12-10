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
 * @file  NetworkManager.h
 *
 * @brief Network manager header file
 *
 */

#pragma once

#include "HvdcLine.h"
#include "Node.h"

#include <DYNDataInterface.h>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <memory>
#include <unordered_map>
namespace dfl {
namespace inputs {

/**
 * @brief Network manager, handling the network input file
 *
 * Relies on DYNAWO data interface
 */
class NetworkManager {
 public:
  /**
  * @brief Enum representing the number of regulating generators for one bus
  */
  enum class NbOfRegulatingGenerators {
    ONE = 0,   ///< There is one generator regulating the bus
    MULTIPLES  ///< There are more than one generator regulating the bus
  };

  using ProcessNodeCallback = std::function<void(const std::shared_ptr<Node>&)>;  ///< Callback for node algorithm
  using BusId = std::string;                                                      ///< alias of BusId
  using BusMapRegulating = std::unordered_map<BusId, NbOfRegulatingGenerators>;   ///< alias for the bus map

 public:
  /**
  * @brief Constructor
  *
  * @param filepath network file path
  */
  explicit NetworkManager(const std::string& filepath);

  /**
   * @brief Register a callback to call at each node
   *
   * @param callback callback
   */
  void onNode(ProcessNodeCallback&& callback) {
    nodesCallbacks_.push_back(std::forward<ProcessNodeCallback>(callback));
  }

  /**
   * @brief Walk through nodes
   *
   * This will call all registered callbacks for nodes
   */
  void walkNodes();

  /**
   * @brief Retrieve the slack node if it is given in the network file
   *
   * @returns the information of the slack node, if present
   */
  boost::optional<std::shared_ptr<Node>> getSlackNode() const {
    return (slackNode_) ? boost::make_optional(slackNode_) : boost::none;
  }

  /**
   * @brief Retrieve data interface
   *
   * @returns data interface
   */
  boost::shared_ptr<DYN::DataInterface> dataInterface() const {
    return interface_;
  }

  /**
   * @brief Retrieve the hvdc lines of the network
   *
   * @returns hvdc line
   */
  const std::vector<std::shared_ptr<HvdcLine>>& getHvdcLine() {
    return hvdcLines_;
  }

  /**
   * @brief Retrieve the mapping of busId and the number of generators that regulates them 
   *
   * @returns map of bus id to nbOfRegulatingGenerators
   */
  const BusMapRegulating& getMapBusId() {
    return mapBusId_;
  }

 private:
  /**
   * @brief Build node tree from data interface
   */
  void buildTree();

 private:
  boost::shared_ptr<DYN::DataInterface> interface_;           ///< data interface
  std::shared_ptr<Node> slackNode_;                           ///< Slack node defined in network, if any
  std::map<Node::NodeId, std::shared_ptr<Node>> nodes_;       ///< nodes representing the node tree
  std::vector<ProcessNodeCallback> nodesCallbacks_;           ///< list of callback or nodes
  std::vector<std::shared_ptr<HvdcLine>> hvdcLines_;          ///< hvdc Lines
  std::vector<std::shared_ptr<VoltageLevel>> voltagelevels_;  ///< Voltage levels elements
  BusMapRegulating mapBusId_;                                 ///< mapping of busId and the number of generators that regulates them
};

}  // namespace inputs
}  // namespace dfl
