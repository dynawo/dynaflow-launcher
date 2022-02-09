//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  LoadDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for loads header file
 *
 */

#pragma once

#include "AlgorithmsResults.h"
#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief Load definition for algorithms
 */
class LoadDefinition {
 public:
  /**
   * @brief model type
   *
   */
  enum class ModelType {
    NETWORK = 0,               ///< Use network model
    LOADRESTORATIVEWITHLIMITS  ///< Use load restorative model
  };
  /**
   * @brief Construct a new Load Definition object
   *
   * @param loadId the load id
   * @param modelType type of model used
   * @param nodeId the node id connected to the load
   */
  explicit LoadDefinition(const inputs::Load::LoadId& loadId, ModelType modelType, const inputs::Node::NodeId& nodeId) :
      id{loadId},
      modelType{modelType},
      nodeId{nodeId} {}

  /**
   * @brief determines if the load model type is network
   *
   * @return true when model type is network
   */
  bool isNetwork() const {
    return modelType == ModelType::NETWORK;
  }

  inputs::Load::LoadId id;      ///< load id
  ModelType modelType;          ///< model type
  inputs::Node::NodeId nodeId;  ///< connected node id
};

/**
 * @brief The load definition algorithm
 */
class LoadDefinitionAlgorithm {
 public:
  using Loads = std::vector<LoadDefinition>;  ///< alias for list of loads

  /**
   * @brief Constructor
   *
   * @param loads the list of loads to update
   * @param dsoVoltageLevel Minimum voltage level of the load to be taken into account
   */
  explicit LoadDefinitionAlgorithm(Loads& loads, double dsoVoltageLevel);

  /**
   * @brief Perform the algorithm
   *
   * Update the list with the loads of the node. Only add the loads for which the nominal voltage of the node is
   * superior to dsoVoltageLevel, otherwise the load is ignored and is handled by the default dynawo behavior
   *
   * @param node the node to process
   * @param algoRes pointer to algorithms results class
   */
  void operator()(const NodePtr& node, std::shared_ptr<AlgorithmsResults>& algoRes);

 private:
  Loads& loads_;            ///< the loads to update
  double dsoVoltageLevel_;  ///< Minimum voltage level of the load to be taken into account
};
}  // namespace algo
}  // namespace dfl
