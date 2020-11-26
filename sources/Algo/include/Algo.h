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

#include "HvdcLine.h"
#include "Node.h"

#include <DYNGeneratorInterface.h>
#include <unordered_set>
#include <vector>

namespace dfl {
/**
 * @brief Namespace for algorithms to perform on network
 *
 * All nodes algorithms are based on functor taking a node in input to perform the algorithm
 * Data to update are provided to the algorithm at construction by reference
 */
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
     * performs @p lhs == @p rhs by relying on Node equality operator
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
    WITH_IMPEDANCE_SIGNALN,            ///< Use GeneratorPVWithImpedanceSignalN model
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
   * @param qmin minimum reactive power for the generator
   * @param qmax maximum reactive power for the generator
   * @param pmin minimum active power for the generator
   * @param pmax maximum active power for the generator
   */
  GeneratorDefinition(const inputs::Generator::GeneratorId& genId, ModelType type, const inputs::Node::NodeId& nodeId,
                      const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax) :
      id{genId},
      model{type},
      nodeId{nodeId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax} {}

  inputs::Generator::GeneratorId id;       ///< generator id
  ModelType model;                         ///< model
  inputs::Node::NodeId nodeId;             ///< connected node id
  std::vector<ReactiveCurvePoint> points;  ///< curve points
  double qmin;                             ///< minimum reactive power
  double qmax;                             ///< maximum reactive power
  double pmin;                             ///< minimum active power
  double pmax;                             ///< maximum active power
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
   * Add the generators of the nodes and deducing the model to use.
   * Validity of the generator is checked whenever it makes sense.
   * If the diagram is not valid, the generator is ignored by the algorithm and the default dynawo behavior is used.
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  /**
   * @brief Checks for diagram validity according to the list of points associated with the generator
   *
   * @param generator The generator with it list of points
   * @return Boolean indicating if the diagram is valid
   */
  static bool isDiagramValid(const inputs::Generator& generator);

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
   */
  void operator()(const NodePtr& node);

 private:
  Loads& loads_;            ///< the loads to update
  double dsoVoltageLevel_;  ///< Minimum voltage level of the load to be taken into account
};

/**
 * @brief Hvdc line definition for algorithms
 */
struct HvdcLineDefinition {
  using HvdcLines = std::vector<HvdcLineDefinition>;  ///< alias for list of hvdcLines

  using HvdcLineId = std::string;  ///< HvdcLine id definition

  /** @brief Enum Position that indicates how the converters of this hvdcLine are positioned.
   * 
   * enum to determine how the converters of this hvdc line are connected inside the network
   * and their position compared to the main connex component.
   */
  enum class Position {
    FIRST_IN_MAIN_COMPONENT,   ///< the first converter of this hvdc line is in the main connex component
    SECOND_IN_MAIN_COMPONENT,  ///< the second converter of this hvdc line is in the main connex component
    BOTH_IN_MAIN_COMPONENT     ///< both converters of this hvdc line are in the main connex component
  };

  /**
   * @brief Constructor
   *
   * @param id the HvdcLine id
   * @param converterType type of converter of the hvdc line
   * @param converter1 first converter connected to the hvdc line
   * @param converter2 second converter connected to the hvdc line
   * @param position position of the converters of this hvdc line compared to the main connex component
   */
  explicit HvdcLineDefinition(const HvdcLineId& id, const inputs::HvdcLine::ConverterType converterType, const inputs::ConverterInterface& converter1,
                              const inputs::ConverterInterface& converter2, Position position) :
      id{id},
      converterType{converterType},
      converter1{converter1},
      converter2{converter2},
      position{position} {}

  const HvdcLineId id;                                  ///< HvdcLine id
  const inputs::HvdcLine::ConverterType converterType;  ///< type of converter of the hvdc line
  inputs::ConverterInterface converter1;                ///< first converter
  inputs::ConverterInterface converter2;                ///< second converter
  Position position;                                    ///< position of the converters of this hvdc line compared to the main connex component
};

/**
 * @brief the controller interface definition algorithm
 */
class ControllerInterfaceDefinitionAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * @param hvdcLines the list of hvdc lines to update
   */
  explicit ControllerInterfaceDefinitionAlgorithm(std::vector<HvdcLineDefinition>& hvdcLines);

  /**
   * @brief Perform the algorithm
   *
   * Update the list with the hvdc line of the converters of the node. 
   * This function also put the position of the converters compared to the main connex component in the newly created hvdc line definition.
   * Indeed, the node that are processed through this operator() are only those that are in the main connex component, which let us deduce the position of the 
   * converters of the hvdc line
   * 
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  std::vector<HvdcLineDefinition>& hvdcLines_;  ///< the hvdc lines to update
};
}  // namespace algo
}  // namespace dfl
