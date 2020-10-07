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
 * @file  Algo.h
 *
 * @brief Dynaflow launcher algorithms header file
 *
 */

#pragma once

#include "Node.h"

#include <DYNGeneratorInterface.h>
#include <unordered_set>
#include <vector>

namespace dfl {
/// @brief Namespace for algorithms to perform on netpork
namespace algo {

/**
 * @brief Base definition for node algorithm
 */
struct NodeAlgorithm {
  using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
};

/**
 * @brief Algorithm to perform on nodes to find the slack node
 */
class SlackNodeAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * @param slackNode the slack node to update with the algorithm
   */
  explicit SlackNodeAlgorithm(NodePtr& slackNode);

  /**
  * @brief Perform elementary step to determine the slack node
  *
  * The used criteria: the slack node is the node which has the higher voltage level then the higher number of connected nodes (in that order)
  *
  * @param node the node to process
  */
  void operator()(const NodePtr& node);

 private:
  NodePtr& slackNode_;  ///< The slack node to update
};

/**
 * @brief Algorithm to determine largest connex component
 */
class MainConnexComponentAlgorithm : public NodeAlgorithm {
 public:
  using ConnexGroup = std::vector<NodePtr>;  ///< Alias for group of nodes

 public:
  /**
   * @brief Constructor
   *
   * @param mainConnexity main connex component to update
   */
  explicit MainConnexComponentAlgorithm(ConnexGroup& mainConnexity);

  /**
   * @brief Perform algorithm
   *
   * For each node, we determine, by going through its neighbours, which other nodes are connexs
   * and we mark the ones we already processed to avoid processing them again.
   *
   *  @brief node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  /**
  * @brief Equality Comparator for nodes
  */
  struct EqualCompareNode {
    /**
     * @brief invocable operator
     *
     * performs @p lhs == @p rhs
     *
     * @param lhs first node
     * @param rhs second node
     *
     * @returns comparaison status
     */
    bool operator()(const NodePtr& lhs, const NodePtr& rhs) const {
      return (*lhs) == (*rhs);
    }
  };

 private:
  /**
  * @brief Update connexity group recursively
  *
  * Update group with all nodes in inputs and their neighbours
  *
  * @param group the group to update
  * @param nodes the nodes to add to the group
  */
  void updateConnexGroup(ConnexGroup& group, const std::vector<NodePtr>& nodes);

 private:
  std::unordered_set<NodePtr, std::hash<NodePtr>, EqualCompareNode> markedNodes_;  ///< the set of marked nodes for the algorithm
  ConnexGroup& mainConnexity_;                                                     ///< the main connex component to update
};

/**
 * @brief Generation definition for algorithm
 */
struct GeneratorDefinition {
  /**
   * @brief Generator model type
   */
  enum class ModelType {
    SIGNALN = 0,                       ///< Use GeneratorPVSignalN model
    DIAGRAM_PQ_SIGNALN,                ///< Use GeneratorPVDiagramPQSignalN model
    WITH_IMPENDANCE_SIGNALN,           ///< Use GeneratorPVWithImpedanceSignalN model
    WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN  ///< Use GeneratorPVWithImpedanceDiagramPQSignalN
  };

  using ReactiveCurvePoint = DYN::GeneratorInterface::ReactiveCurvePoint;  ///< Alias for reactive curve point

  /**
   * @brief Constructor
   *
   * @param genId generator id
   * @param type the model to use
   * @param nodeId the node id connected to the generator
   * @param curvePoints the list of reactive capabilities curve points
   */
  GeneratorDefinition(const inputs::Generator::GeneratorId& genId, ModelType type, const inputs::Node::NodeId& nodeId,
                      const std::vector<ReactiveCurvePoint>& curvePoints) :
      id{genId},
      model{type},
      nodeId{nodeId},
      points(curvePoints) {}

  inputs::Generator::GeneratorId id;       ///< generator id
  ModelType model;                         ///< model
  inputs::Node::NodeId nodeId;             ///< connected node id
  std::vector<ReactiveCurvePoint> points;  ///< curve points
};

/**
 * @brief Algorithm to find generators
 */
class GeneratorDefinitionAlgorithm : public NodeAlgorithm {
 public:
  using Generators = std::vector<GeneratorDefinition>;  ///< alias for list of generators

  /**
   * @brief Constructor
   *
   * @param gens generators list to update
   * @param infinitereactivelimits parameter to determine if infinite reactive limits are used
   */
  GeneratorDefinitionAlgorithm(Generators& gens, bool infinitereactivelimits);

  /**
   * @brief Perform algorithm
   *
   * Add the generators of the nodes and deducing the model to use
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  Generators& generators_;          ///< the generators list to update
  bool useInfiniteReactivelimits_;  ///< determine if infinite reactive limits are used
};

/**
 * @brief Load definition for algorithms
 */
struct LoadDefinition {
  /**
   * @brief Constructor
   *
   * @param loadId the load id
   * @param nodeId the node id connected to the load
   */
  explicit LoadDefinition(const inputs::Load::LoadId& loadId, const inputs::Node::NodeId& nodeId) : id{loadId}, nodeId{nodeId} {}

  inputs::Load::LoadId id;      ///< load id
  inputs::Node::NodeId nodeId;  ///< connected node id
};

/**
 * @brief The load definition algorithm
 */
class LoadDefinitionAlgorithm : public NodeAlgorithm {
 public:
  using Loads = std::vector<LoadDefinition>;  ///< alias for list of loads

  /**
   * @brief Constructor
   *
   * @param loads the list of loads to update
   */
  explicit LoadDefinitionAlgorithm(Loads& loads);

  /**
   * @brief Perform the algorithm
   *
   * Update the list with the loads of the node
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  Loads& loads_;  ///< the loads to update
};

}  // namespace algo
}  // namespace dfl
