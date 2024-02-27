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
#include <DYNComponentInterface.h>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>
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
  * @brief Enum representing the number of regulating elements for one bus
  */
  enum class NbOfRegulating {
    ONE = 0,   ///< There is one element regulating the bus
    MULTIPLES  ///< There are more than one element regulating the bus
  };

  using ProcessNodeCallback = std::function<void(const std::shared_ptr<Node>&)>;  ///< Callback for node algorithm
  using BusId = std::string;                                                      ///< alias of BusId
  using BusMapRegulating = std::unordered_map<BusId, NbOfRegulating>;             ///< alias for the bus map

 public:
  /**
  * @brief Constructor
  *
  * @param filepath network file path
  */
  explicit NetworkManager(const boost::filesystem::path& filepath);

  /**
   * @brief Register a callback to call at each node
   *
   * @param callback callback
   */
  void onNode(ProcessNodeCallback&& callback) {
    nodesCallbacks_.push_back(std::move(callback));
  }

  /**
   * @brief Walk through nodes
   *
   * This will call all registered callbacks for nodes
   */
  void walkNodes() const;

  /**
   * @brief Retrieve the slack node if it is given in the network file
   *
   * @returns the information of the slack node, if present
   */
  boost::optional<std::shared_ptr<Node>> getSlackNode() const {
    return slackNode_ ? boost::make_optional(slackNode_) : boost::none;
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
  const std::vector<std::shared_ptr<HvdcLine>>& getHvdcLine() const {
    return hvdcLines_;
  }

  /**
   * @brief Computes total list of VSC converters accross all HVDClines
   *
   * @return list of the converters
   */
  std::unordered_set<std::shared_ptr<Converter>> computeVSCConverters() const;

  /**
   * @brief Retrieve the mapping of busId and the number of generators that regulate them
   *
   * @returns map of bus id to nbOfRegulatingGenerators
   */
  const BusMapRegulating& getMapBusGeneratorsBusId() const {
    return mapBusGeneratorsBusId_;
  }

  /**
   * @brief Rerieve the mapping of busId and the number of VSC converters that regulate it
   * @returns map of bus id to nbOfRegulatingGenerators
   */
  const BusMapRegulating& getMapBusVSCConvertersBusId() const {
    return mapBusVSCConvertersBusId_;
  }

  /**
   * @brief determines if the network is at least partially conditioned
   * @returns true if at least one component has initial conditions set, false otherwise
   */
  bool isPartiallyConditioned() const { return isPartiallyConditioned_; }

  /**
   * @brief determines if the network is fully conditioned
   * @returns true if all components have initial conditions set, false otherwise
   */
  bool isFullyConditioned() const { return isFullyConditioned_; }

 private:
  /**
   * @brief Build node tree from data interface
   */
  void buildTree();

  /**
   * @brief Update a bus regulating map according to internal interface
   * @param map the mapping to update
   * @param elementId the element id to add to the map
   * @param dataInterface the data interface to use
   */
  static void updateMapRegulatingBuses(BusMapRegulating& map, const std::string& elementId, const boost::shared_ptr<DYN::DataInterface>& dataInterface);

  /**
   * @brief Update the network conditioning status based on a component conditioning status
   * @param componentInterface the data interface to use
   */
  void updateConditioningStatus(const boost::shared_ptr<DYN::ComponentInterface>& componentInterface);

 private:
  boost::shared_ptr<DYN::DataInterface> interface_;           ///< data interface
  std::shared_ptr<Node> slackNode_;                           ///< Slack node defined in network, if any
  std::map<Node::NodeId, std::shared_ptr<Node>> nodes_;       ///< nodes representing the node tree
  std::vector<ProcessNodeCallback> nodesCallbacks_;           ///< list of callback or nodes
  std::vector<std::shared_ptr<HvdcLine>> hvdcLines_;          ///< hvdc Lines
  std::vector<std::shared_ptr<VoltageLevel>> voltagelevels_;  ///< Voltage levels elements
  std::vector<std::shared_ptr<Line>> lines_;                  ///< List of the lines
  std::vector<std::shared_ptr<Tfo>> tfos_;                    ///< List of transformers
  BusMapRegulating mapBusGeneratorsBusId_;                    ///< mapping of busId and the number of generators that regulate them
  BusMapRegulating mapBusVSCConvertersBusId_;                 ///< mapping of busId and the number of VSC converters that regulate them
  bool   isPartiallyConditioned_;                             ///< true if the network is at last prtially conditioned, false otherwise
  bool   isFullyConditioned_;                                 ///< true if the network is fully conditioned, false otherwise
};

}  // namespace inputs
}  // namespace dfl
