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
   * @param targetP target active power of the generator
   */
  GeneratorDefinition(const inputs::Generator::GeneratorId& genId, ModelType type, const inputs::Node::NodeId& nodeId,
                      const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax, double targetP) :
      id{genId},
      model{type},
      nodeId{nodeId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax},
      targetP{targetP} {}

  inputs::Generator::GeneratorId id;       ///< generator id
  ModelType model;                         ///< model
  inputs::Node::NodeId nodeId;             ///< connected node id
  std::vector<ReactiveCurvePoint> points;  ///< curve points
  double qmin;                             ///< minimum reactive power
  double qmax;                             ///< maximum reactive power
  double pmin;                             ///< minimum active power
  double pmax;                             ///< maximum active power
  double targetP;                          ///< target active power of the generator
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
  using ConverterId = std::string;                    ///< alias for converter id
  using BusId = std::string;                          ///< alias for bus id
  using HvdcLineId = std::string;                     ///< HvdcLine id definition

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
   * @param converter1_id first converter id 
   * @param converter1_busId first converter bus id
   * @param converter1_voltageRegulationOn firt converter voltage regulation parameter, for VSC converters only
   * @param converter2_id second converter id 
   * @param converter2_busId second converter bus id
   * @param converter2_voltageRegulationOn second converter voltage regulation parameter, for VSC converters only
   * @param position position of the converters of this hvdc line compared to the main connex component
   */
  explicit HvdcLineDefinition(const HvdcLineId& id, const inputs::HvdcLine::ConverterType converterType, const ConverterId& converter1_id,
                              const BusId& converter1_busId, const boost::optional<bool>& converter1_voltageRegulationOn, const ConverterId& converter2_id,
                              const BusId& converter2_busId, const boost::optional<bool>& converter2_voltageRegulationOn, const Position position) :
      id{id},
      converterType{converterType},
      converter1_id{converter1_id},
      converter1_busId{converter1_busId},
      converter1_voltageRegulationOn{converter1_voltageRegulationOn},
      converter2_id{converter2_id},
      converter2_busId{converter2_busId},
      converter2_voltageRegulationOn{converter2_voltageRegulationOn},
      position{position} {}

  const HvdcLineId id;                                         ///< HvdcLine id
  const inputs::HvdcLine::ConverterType converterType;         ///< type of converter of the hvdc line
  const ConverterId converter1_id;                             ///< first converter id
  const BusId converter1_busId;                                ///< first converter bus id
  const boost::optional<bool> converter1_voltageRegulationOn;  ///< firt converter voltage regulation parameter, for VSC converters only
  const ConverterId converter2_id;                             ///< second converter id
  const BusId converter2_busId;                                ///< second converter bus id
  const boost::optional<bool> converter2_voltageRegulationOn;  ///< second converter voltage regulation parameter, for VSC converters only
  Position position;                                           ///< position of the converters of this hvdc line compared to the main connex component
};

/**
 * @brief the controller interface definition algorithm
 */
class ControllerInterfaceDefinitionAlgorithm : public NodeAlgorithm {
 public:
  using HvdcLineId = std::string;                                ///< HvdcLine id definition
  using HvdcLineMap = std::map<HvdcLineId, HvdcLineDefinition>;  ///< Alias for map of hvdc line definition

  /**
   * @brief Constructor
   *
   * @param hvdcLines the list of hvdc lines to update
   */
  explicit ControllerInterfaceDefinitionAlgorithm(HvdcLineMap& hvdcLines);

  /**
   * @brief Perform the algorithm
   *
   * Update the list with the hvdc line of the converters of the node. 
   * This function also put the position of the converters compared to the main connex component in the newly created hvdc line definition.
   * Pre-condition: the nodes used as parameter of this operator should be nodes of the main connex component only
   * 
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  HvdcLineMap& hvdcLines_;  ///< the set of hvdc line, to know if we have already encountered this hvdc line
};
}  // namespace algo
}  // namespace dfl
