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

#include "DynamicDataBaseManager.h"
#include "HvdcLine.h"
#include "NetworkManager.h"
#include "Node.h"

#include <DYNGeneratorInterface.h>
#include <DYNServiceManagerInterface.h>
#include <array>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
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
    SIGNALN = 0,                ///< Use GeneratorPVSignalN model
    DIAGRAM_PQ_SIGNALN,         ///< Use GeneratorPVDiagramPQSignalN model
    REMOTE_SIGNALN,             ///< Use GeneratorPVRemoteSignalN
    REMOTE_DIAGRAM_PQ_SIGNALN,  ///< Use GeneratorPVRemoteDiagramPQSignalN
    PROP_SIGNALN,               ///< Use GeneratorPQPropSignalN
    PROP_DIAGRAM_PQ_SIGNALN     ///< Use GeneratorPQPropDiagramPQSignalN
  };
  using ReactiveCurvePoint = DYN::GeneratorInterface::ReactiveCurvePoint;  ///< Alias for reactive curve point
  using BusId = std::string;                                               ///< alias of BusId

  /**
   * @brief test if the model used is a diagram
   *
   * @return boolean indicating if the model uses a diagram
   */
  bool isUsingDiagram() const {
    return model == ModelType::DIAGRAM_PQ_SIGNALN || model == ModelType::REMOTE_DIAGRAM_PQ_SIGNALN || model == ModelType::PROP_DIAGRAM_PQ_SIGNALN;
  }

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
   * @param regulatedBusId the Bus Id this generator is regulating
   */
  GeneratorDefinition(const inputs::Generator::GeneratorId& genId, ModelType type, const inputs::Node::NodeId& nodeId,
                      const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax, double targetP,
                      const BusId& regulatedBusId) :
      id{genId},
      model{type},
      nodeId{nodeId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax},
      targetP{targetP},
      regulatedBusId{regulatedBusId} {}

  inputs::Generator::GeneratorId id;       ///< generator id
  ModelType model;                         ///< model
  inputs::Node::NodeId nodeId;             ///< connected node id
  std::vector<ReactiveCurvePoint> points;  ///< curve points
  double qmin;                             ///< minimum reactive power
  double qmax;                             ///< maximum reactive power
  double pmin;                             ///< minimum active power
  double pmax;                             ///< maximum active power
  double targetP;                          ///< target active power of the generator
  const BusId regulatedBusId;              ///< regulated Bus Id
};

/**
 * @brief Algorithm to find generators
 */
class GeneratorDefinitionAlgorithm : public NodeAlgorithm {
 public:
  using Generators = std::vector<GeneratorDefinition>;  ///< alias for list of generators
  using BusId = std::string;                            ///< alias for bus id
  using GenId = std::string;                            ///< alias for generator id
  using BusGenMap = std::unordered_map<BusId, GenId>;   ///< alias for map of bus id to generator id

  /**
   * @brief Constructor
   *
   * @param gens generators list to update
   * @param busesWithDynamicModel map of bus ids to a generator that regulates them
   * @param busMap mapping of busId and the number of generators that regulates them
   * @param infinitereactivelimits parameter to determine if infinite reactive limits are used
   * @param serviceManager dynawo service manager in order to use Dynawo extra algorithms
   */
  GeneratorDefinitionAlgorithm(Generators& gens, BusGenMap& busesWithDynamicModel, const inputs::NetworkManager::BusMapRegulating& busMap,
                               bool infinitereactivelimits, const boost::shared_ptr<DYN::ServiceManagerInterface>& serviceManager);

  /**
   * @brief Perform algorithm
   *
   * Add the generators of the nodes and deducing the model to use.
   * Validity of the generator is checked whenever it makes sense.
   * If the diagram is not valid, the generator is ignored by the algorithm and the default dynawo behavior is used.
   * For each generator, we look in the busMap_ the number of generators that are regulating the same bus as this generator,
   * If this generator is the only one regulating the bus, then we decide which model to use based on whether this generator
   * regulates the bus in local or not. Otherwise we use the prop model.
   * In order to create later on in the dyd and the par specific models based on the buses that are regulated by multiples
   * generators, we fill the busesWithDynamicModel_ map. Each time we found a bus regulated by multiples generators we add in the
   * busesWithDynamicModel_ map an element mapping the regulated bus to a generator id that regulates that bus.
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

  /**
   * @brief Determines if a node is connected to another generator node through a switch network path
   *
   * Uses the dynawo service manager
   *
   * @param node the generator node to check
   *
   * @returns @b true if another generator is connected, @b false if not
   */
  bool IsOtherGeneratorConnectedBySwitches(const NodePtr& node) const;

  Generators& generators_;                                          ///< the generators list to update
  BusGenMap& busesWithDynamicModel_;                                ///< map of bus ids to a generator that regulates them
  const inputs::NetworkManager::BusMapRegulating& busMap_;          ///< mapping of busId and the number of generators that regulates them
  bool useInfiniteReactivelimits_;                                  ///< determine if infinite reactive limits are used
  boost::shared_ptr<DYN::ServiceManagerInterface> serviceManager_;  ///< dynawo service manager
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

/// @brief VSC definition
class VSCDefinition {
 public:
  using VSCId = std::string;                                            ///< Alias for VSC component id
  using ReactiveCurvePoint = inputs::VSCConverter::ReactiveCurvePoint;  ///< point type

  /**
   * @brief Constructor
   * @param id the id of the converter
   * @param qMax the maximum reactive power capability value of the converter
   * @param qMin the minimum reactive power capability value of the converter
   * @param pMax the maximum active power capability value of the converter
   * @param points the reactive curve points of the converter, if any
   */
  VSCDefinition(const VSCId& id, double qMax, double qMin, double pMax, const std::vector<ReactiveCurvePoint>& points) :
      id(id),
      qmax{qMax},
      qmin{qMin},
      pmax(pMax),
      pmin(-pMax),
      points(points) {}

  /**
   * @brief Equality operator for VSCDefinition
   * @param other the element to compare to
   * @returns true if current definition equals to @a other, false if not
   */
  bool operator==(const dfl::algo::VSCDefinition& other) const {
    return id == other.id && pmax == other.pmax && pmin == other.pmin && points.size() == other.points.size() &&
           std::equal(points.begin(), points.end(), other.points.begin(), comparePoints) && qmax == other.qmax && qmin == other.qmin;
  }

  VSCId id;                                ///< id of the converter
  double qmax;                             ///< maximum reactive power capability value
  double qmin;                             ///< minimum reactive power capability value
  double pmax;                             ///< maximum active power capability value
  double pmin;                             ///< minimum active power capability value, equals to -pmax
  std::vector<ReactiveCurvePoint> points;  ///< reactive curve points

 private:
  /**
   * @brief Compare function for reactive curve points
   * @param lhs first element
   * @param rhs second element
   * @returns true of lhs == rhs, false if not
   */
  static bool comparePoints(const ReactiveCurvePoint& lhs, const ReactiveCurvePoint& rhs) {
    return lhs.p == rhs.p && lhs.qmax == rhs.qmax && lhs.qmin == rhs.qmin;
  }
};

/**
 * @brief Hvdc line definition for algorithms
 */
struct HVDCDefinition {
  using ConverterId = std::string;                        ///< alias for converter id
  using BusId = std::string;                              ///< alias for bus id
  using HvdcLineId = std::string;                         ///< HvdcLine id definition
  using ConverterType = inputs::HvdcLine::ConverterType;  ///< Alias for type of converter

  /** @brief Enum Position that indicates how the converters of this hvdcLine are positioned.
   *
   * enum to determine how the converters of this hvdc line are connected inside the network
   * and their position compared to the main connex component.
   */
  enum class Position {
    FIRST_IN_MAIN_COMPONENT = 0,  ///< the first converter of this hvdc line is in the main connex component
    SECOND_IN_MAIN_COMPONENT,     ///< the second converter of this hvdc line is in the main connex component
    BOTH_IN_MAIN_COMPONENT        ///< both converters of this hvdc line are in the main connex component
  };

  /// @brief HVDC available models
  enum class HVDCModel {
    HvdcPTanPhi = 0,
    HvdcPTanPhiDangling,
    HvdcPTanPhiDanglingDiagramPQ,
    HvdcPTanPhiDiagramPQ,
    HvdcPQProp,
    HvdcPQPropDangling,
    HvdcPQPropDanglingDiagramPQ,
    HvdcPQPropDiagramPQ,
    HvdcPQPropDiagramPQEmulation,
    HvdcPQPropEmulation,
    HvdcPV,
    HvdcPVDangling,
    HvdcPVDanglingDiagramPQ,
    HvdcPVDiagramPQ,
    HvdcPVDiagramPQEmulation,
    HvdcPVEmulation,
  };

  /**
   * @brief Check if the HVDC definition has a diagram model
   * @returns true if HVDC definition is using a model with diagrams, false if not
   */
  bool hasDiagramModel() const {
    return model == HVDCModel::HvdcPTanPhiDanglingDiagramPQ || model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPVDanglingDiagramPQ ||
           model == HVDCModel::HvdcPTanPhiDiagramPQ || model == HVDCModel::HvdcPQPropDiagramPQ || model == HVDCModel::HvdcPQPropDiagramPQEmulation ||
           model == HVDCModel::HvdcPVDiagramPQ || model == HVDCModel::HvdcPVDiagramPQEmulation;
  }

  /**
   * @brief Check if the HVDC definition has an AC emulation model
   * @returns true if HVDC definition is using an AC emulation model, false if not
   */
  bool hasEmulationModel() const {
    return model == HVDCModel::HvdcPQPropEmulation || model == HVDCModel::HvdcPQPropDiagramPQEmulation || model == HVDCModel::HvdcPVEmulation ||
           model == HVDCModel::HvdcPVDiagramPQEmulation;
  }

  /**
   * @brief Check if the HVDC definition has a model with a proportional Q regulation
   * @returns true if HVDC definition is using a modelwith a proportional Q regulation, false if not
   */
  bool hasPQPropModel() const {
    return model == HVDCModel::HvdcPQPropDangling || model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPQProp ||
           model == HVDCModel::HvdcPQPropDiagramPQ || model == HVDCModel::HvdcPQPropEmulation || model == HVDCModel::HvdcPQPropDiagramPQEmulation;
  }

  /**
   * @brief Check if the HVDC definition has a dangling model
   * @returns true if HVDC definition is using a dangling model, false if not
   */
  bool hasDanglingModel() const {
    return model == HVDCModel::HvdcPTanPhiDangling || model == HVDCModel::HvdcPTanPhiDanglingDiagramPQ || model == HVDCModel::HvdcPQPropDangling ||
           model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPVDangling || model == HVDCModel::HvdcPVDanglingDiagramPQ;
  }

  /**
   * @brief Constructor
   *
   * @param id the HvdcLine id
   * @param converterType type of converter of the hvdc line
   * @param converter1Id first converter id
   * @param converter1BusId first converter bus id
   * @param converter1VoltageRegulationOn first converter voltage regulation parameter, for VSC converters only
   * @param converter2Id second converter id
   * @param converter2BusId second converter bus id
   * @param converter2VoltageRegulationOn second converter voltage regulation parameter, for VSC converters only
   * @param position position of the converters of this hvdc line compared to the main connex component
   * @param model HVDC model to use for HVDC line
   * @param powerFactors the power factors for both converters, relevant only for LCC converters
   * @param pMax the maximum p
   * @param vscDefinition1 the definition of the first VSC, if present
   * @param vscDefinition2 the definition of the second VSC, if present
   * @param droop the active power droop value for HVDC, if the value exists
   */
  explicit HVDCDefinition(const HvdcLineId& id, const inputs::HvdcLine::ConverterType converterType, const ConverterId& converter1Id,
                          const BusId& converter1BusId, const boost::optional<bool>& converter1VoltageRegulationOn, const ConverterId& converter2Id,
                          const BusId& converter2BusId, const boost::optional<bool>& converter2VoltageRegulationOn, const Position position,
                          const HVDCModel& model, const std::array<double, 2>& powerFactors, double pMax, const boost::optional<VSCDefinition>& vscDefinition1,
                          const boost::optional<VSCDefinition>& vscDefinition2, const boost::optional<double>& droop) :
      id{id},
      converterType{converterType},
      converter1Id{converter1Id},
      converter1BusId{converter1BusId},
      converter1VoltageRegulationOn{converter1VoltageRegulationOn},
      converter2Id{converter2Id},
      converter2BusId{converter2BusId},
      converter2VoltageRegulationOn{converter2VoltageRegulationOn},
      position{position},
      model{model},
      powerFactors(powerFactors),
      pMax{pMax},
      vscDefinition1(vscDefinition1),
      vscDefinition2(vscDefinition2),
      droop(droop) {}

  const HvdcLineId id;                                        ///< HvdcLine id
  const ConverterType converterType;                          ///< type of converter of the hvdc line
  const ConverterId converter1Id;                             ///< first converter id
  const BusId converter1BusId;                                ///< first converter bus id
  const boost::optional<bool> converter1VoltageRegulationOn;  ///< first converter voltage regulation parameter, for VSC converters only
  const ConverterId converter2Id;                             ///< second converter id
  const BusId converter2BusId;                                ///< second converter bus id
  const boost::optional<bool> converter2VoltageRegulationOn;  ///< second converter voltage regulation parameter, for VSC converters only
  Position position;                                          ///< position of the converters of this hvdc line compared to the main connex component
  HVDCModel model;                                            ///< HVDC model to use
  const std::array<double, 2> powerFactors;                   ///< power factors for converters 1 and 2, irrelevant if type is not LCC
  const double pMax;                                          ///< maximum p
  const boost::optional<VSCDefinition> vscDefinition1;        ///< underlying VSC converter 1, irrelevant if type is not VSC
  const boost::optional<VSCDefinition> vscDefinition2;        ///< underlying VSC converter 2, irrelevant if type is not VSC
  const boost::optional<double> droop;                        ///< active power droop value for HVDC, if it exists
};

/// @brief HVDC line definitions
struct HVDCLineDefinitions {
  using HvdcLineId = std::string;  ///< HvdcLine id definition

  using HvdcLineMap = std::unordered_map<HvdcLineId, HVDCDefinition>;  ///< Alias for map of hvdc line definition

  /**
   * @brief Alias for VSC map to their regulated bus
   *
   * We keep only one of the VSC definitions connected to a regulated bus as only one is required
   * to export the parameters
   */
  using BusVSCMap = std::unordered_map<HVDCDefinition::BusId, VSCDefinition>;

  HvdcLineMap hvdcLines;              ///< the set of hvdc lines
  BusVSCMap vscBusVSCDefinitionsMap;  ///< mapping of buses that have multiple VSC connected and one of their VSC
};

/**
 * @brief the controller interface definition algorithm
 */
class HVDCDefinitionAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * @param hvdcLinesDefinitions the HVDC line definitions to update
   * @param infiniteReactiveLimits the configuration data of whether we use infinite reactive limits
   * @param mapBusVSCConvertersBusId the mapping of buses and their number of VSC converters regulating them
   */
  HVDCDefinitionAlgorithm(HVDCLineDefinitions& hvdcLinesDefinitions, bool infiniteReactiveLimits,
                          const inputs::NetworkManager::BusMapRegulating& mapBusVSCConvertersBusId);

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
  /// @brief HVDC model definition
  struct HVDCModelDefinition {
    using VSCBusPair = std::pair<HVDCDefinition::BusId, VSCDefinition::VSCId>;  ///< Alias for pair of bus id and VSC id

    HVDCDefinition::HVDCModel model;                     ///< the model to use
    std::vector<VSCBusPair> vscBusIdsMultipleRegulated;  ///< the VSCs and their bus that are involved in multiple VSC regulations
  };

 private:
  /**
   * @brief Compute the model definition
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @param type the converter type (VSC or LCC)
   * @returns the model definition to use
   */
  HVDCModelDefinition computeModel(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, inputs::HvdcLine::ConverterType type) const;

  /**
   * @brief Compute the model definition for VSC converters
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @param multipleVSCInfiniteReactive model to use in case of multiple VSC and infinite reactive limits used
   * @param multipleVSCFiniteReactive model to use in case of multiple VSC and finite reactive limits used
   * @param oneVSCInfiniteReactive model to use in case of only one VSC and infinite reactive limits used
   * @param oneVSCFiniteReactive model to use in case of only one VSC and finite reactive limits used
   * @returns the model definition to use
   */
  HVDCModelDefinition computeModelVSC(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position,
                                      HVDCDefinition::HVDCModel multipleVSCInfiniteReactive, HVDCDefinition::HVDCModel multipleVSCFiniteReactive,
                                      HVDCDefinition::HVDCModel oneVSCInfiniteReactive, HVDCDefinition::HVDCModel oneVSCFiniteReactive) const;

  /**
   * @brief Get the list of buses regulated by multiple VSC
   * @param hvdcline the hvdc line to process
   * @param position the position of the extremities
   * @returns the list of pairs (bus, VSC) involved in multiple VSC regulation
   */
  std::vector<HVDCModelDefinition::VSCBusPair> getBusRegulatedByMultipleVSC(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position) const;

  /**
   * @brief Get or create HVDC line definition
   *
   * Creates the element if not already existing
   * @param hvdcLine the HVDC line to process
   * @returns the pair (element, status), where element is the inserted or got element and status true if element was already inserted before or false if not
   */
  std::pair<std::reference_wrapper<HVDCDefinition>, bool> getOrCreateHvdcLineDefinition(const inputs::HvdcLine& hvdcLine);

 private:
  HVDCLineDefinitions& hvdcLinesDefinitions_;                                 ///< The HVDC lines definitions to update
  const bool infiniteReactiveLimits_;                                         ///< whether we use infinite reactive limits
  const inputs::NetworkManager::BusMapRegulating& mapBusVSCConvertersBusId_;  ///< the map of buses and the number of VSC converters regulating them
};

/**
 * @brief DynModel definition
 *
 * Structure containing minimum information to define a dynamic model with its connections
 */
struct DynamicModelDefinition {
  using DynModelId = std::string;  ///< alias for dynamic model id

  /**
   * @brief Macro connection definition
   */
  struct MacroConnection {
    using MacroId = std::string;    ///< alias for macro connector id
    using ElementId = std::string;  ///< alias for connected element id

    /// @brief Connected element type
    enum class ElementType {
      NODE = 0,  ///< Node type
      LINE,      ///< Line type
      TFO,       ///< Transformer type
      SHUNT      ///< Shunt type
    };

    /**
     * @brief Constructor
     *
     * @param macroid the macro connector id to use
     * @param type type of the element to connect
     * @param id the element id to connect
     */
    MacroConnection(const MacroId& macroid, const ElementType& type, const ElementId& id) : id(macroid), elementType(type), connectedElementId(id) {}

    /**
     * @brief Equality operator
     * @param other the macro connection to compare to
     * @returns true if they are equal, false if not
     */
    bool operator==(const MacroConnection& other) const;

    /**
     * @brief Non-equality operator
     * @param other the macro connection to compare to
     * @returns true if they are not equal, false if not
     */
    bool operator!=(const MacroConnection& other) const;

    /**
     * @brief Less than operator
     * @param other the macro connection to compare to
     * @returns true if inferior than other, false if not
     */
    bool operator<(const MacroConnection& other) const;

    /**
     * @brief Less or equal than operator
     * @param other the macro connection to compare to
     * @returns true if inferior or equal than other, false if not
     */
    bool operator<=(const MacroConnection& other) const;

    /**
     * @brief Greater than operator
     * @param other the macro connection to compare to
     * @returns true if superior than other, false if not
     */
    bool operator>(const MacroConnection& other) const;

    /**
     * @brief Greater or equal than operator
     * @param other the macro connection to compare to
     * @returns true if superior or equal than other, false if not
     */
    bool operator>=(const MacroConnection& other) const;

    MacroId id;                    ///< Macro connector id
    ElementType elementType;       ///< Connected element type
    ElementId connectedElementId;  ///< Element id connected throught the macro connection (can be node, line or tfo)
  };

  /**
   * @brief Constructor
   *
   * @param dynModelId the dynamic model id
   * @param dynModelLib the library name
   */
  DynamicModelDefinition(const DynModelId& dynModelId, const std::string& dynModelLib) : id(dynModelId), lib(dynModelLib) {}

  DynModelId id;                              ///< dynamic model id
  std::string lib;                            ///< library name
  std::set<MacroConnection> nodeConnections;  ///< set of macro connections for the dynamic model
};

/**
 * @brief Definition of models
 */
struct DynamicModelDefinitions {
  std::unordered_map<DynamicModelDefinition::DynModelId, DynamicModelDefinition> models;      ///< models by dynamic model id
  std::unordered_set<DynamicModelDefinition::MacroConnection::MacroId> usedMacroConnections;  ///< list of macro connectors used for current set of models
};

/**
 * @brief Algorithm to find dynamic models
 */
class DynModelAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * The constructor will pre-process some data so that relevant information are retrieved with efficiency during main algorithm
   *
   * @param models the models to update
   * @param manager the dynamic data base manager to use
   */
  DynModelAlgorithm(DynamicModelDefinitions& models, const inputs::DynamicDataBaseManager& manager);

  /**
   * @brief Perform the algorithm
   *
   * Depending on whether the node is concerned in a macro connection, dispatch the node to update the macro connections information in the models definition.
   * Macro connections not connected to a network node will be discarded and not put in the dynamic models definitions.
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  /**
   * @brief DynModel macro connect definition
   */
  struct MacroConnect {
    /// @brief Default Constructor
    MacroConnect() = default;

    /**
     * @brief Constructor
     *
     * @param modelId dynamic model id
     * @param macroConnection macro connector id
     */
    MacroConnect(const DynamicModelDefinition::DynModelId& modelId, const DynamicModelDefinition::MacroConnection::MacroId& macroConnection) :
        dynModelId(modelId),
        macroConnectionId(macroConnection) {}

    /**
     * @brief Equality operator
     * @param other the other macro connect to compare to
     * @returns true if they are equal, false if not
     */
    bool operator==(const MacroConnect& other) const;

    /**
     * @brief Non-equality operator
     * @param other the other macro connect to compare to
     * @returns true if they are not equal, false if not
     */
    bool operator!=(const MacroConnect& other) const;

    DynamicModelDefinition::DynModelId dynModelId;                       ///< dynamic model id
    DynamicModelDefinition::MacroConnection::MacroId macroConnectionId;  ///< macro connection id
  };

  /**
   * @brief Hash structure for macro connect
   */
  struct MacroConnectHash {
    /**
     * @brief Operator to hash a macro connect
     * @returns unique hash
     */
    std::size_t operator()(const MacroConnect& connect) const noexcept;
  };

 private:
  /**
   * @brief Determines if a library is loadable in current environement
   * @param lib library name
   * @returns true if library could be loaded, false if not
   */
  static bool libraryExists(const std::string& lib);

  /**
   * @brief Computes library path for library name
   *
   * Search for the library in dynawo environment and then in DFL environment
   *
   * @param lib the library name
   * @returns the filepath of the library, or nullopt if not found
   */
  static boost::optional<boost::filesystem::path> findLibraryPath(const std::string& lib);

 private:
  /// @brief Extract models from configuration before processing the nodes
  void extractDynModels();

  /**
   * @brief Process single association from configuration
   *
   * @param automaton the dynamic automaton
   * @param macro the macro connection connected to the singleassociation
   * @param singleassoc the single association config element to process
   */
  void extractSingleAssociationInfo(const inputs::AssemblingXmlDocument::DynamicAutomaton& automaton, const inputs::AssemblingXmlDocument::MacroConnect& macro,
                                    const inputs::AssemblingXmlDocument::SingleAssociation& singleassoc);

  /**
   * @brief Process multi association from configuration
   *
   * @param automaton the dynamic automaton
   * @param macro the macro connection connected to the singleassociation
   * @param multiassoc the multi association config element to process
   */
  void extractMultiAssociationInfo(const inputs::AssemblingXmlDocument::DynamicAutomaton& automaton, const inputs::AssemblingXmlDocument::MacroConnect& macro,
                                   const inputs::AssemblingXmlDocument::MultipleAssociation& multiassoc);

  /**
   * @brief Process node in case of dynamic automaton bus connection
   * @param node node to process
   */
  void connectMacroConnectionForBus(const NodePtr& node);

  /**
   * @brief Process node in case of dynamic automaton shunt connection
   * @param node node to process
   */
  void connectMacroConnectionForShunt(const NodePtr& node);

  /**
   * @brief Process node in case of dynamic automaton line connection
   * @param line line to process
   */
  void connectMacroConnectionForLine(const std::shared_ptr<inputs::Line>& line);

  /**
   * @brief Process node in case of dynamic automaton transformer connection
   * @param tfo transformer to process
   */
  void connectMacroConnectionForTfo(const std::shared_ptr<inputs::Tfo>& tfo);

  /**
   * @brief Add macro connection to the dynamic model definition
   *
   * Creates the dynamic model definition if not already existing
   * Update the dynamic model definitions member
   *
   * @param automaton the dynamic automaton
   * @param macroConnection the macro connection to add
   */
  void addMacroConnectionToModelDefinitions(const dfl::inputs::AssemblingXmlDocument::DynamicAutomaton& automaton,
                                            const DynamicModelDefinition::MacroConnection& macroConnection);

 private:
  DynamicModelDefinitions& dynamicModels_;  ///< Dynamic model definitions to update

  std::unordered_map<std::string, inputs::AssemblingXmlDocument::DynamicAutomaton> dynamicAutomatonsById_;  ///< dynamic automatons by model id
  std::unordered_map<inputs::VoltageLevel::VoltageLevelId, std::unordered_set<MacroConnect, MacroConnectHash>>
      macroConnectByVlForBusesId_;  ///< macro connections for buses, by voltage level
  std::unordered_map<inputs::VoltageLevel::VoltageLevelId, std::vector<MacroConnect>>
      macroConnectByVlForShuntsId_;                                                             ///< macro connections for shunts, by voltage level
  std::unordered_map<inputs::Line::LineId, std::vector<MacroConnect>> macroConnectByLineName_;  ///< macro connections for lines, by line id
  std::unordered_map<inputs::Tfo::TfoId, std::vector<MacroConnect>> macroConnectByTfoName_;     ///< macro connections for transformer, by transformer id

  const inputs::DynamicDataBaseManager& manager_;  ///< dynamic database config manager
};

/**
 * @brief Shunt counter definition
 */
struct ShuntCounterDefinitions {
  std::unordered_map<inputs::VoltageLevel::VoltageLevelId, unsigned int> nbShunts;  ///< Number of shunts by voltage level
};

/**
 * @brief Counter of shunts by voltage levels
 */
class ShuntCounterAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param shuntCounterDefs the counter definitions to update
   */
  explicit ShuntCounterAlgorithm(ShuntCounterDefinitions& shuntCounterDefs);

  /**
   * @brief Performs the algorithm
   *
   * The algorithm counts the number of shunts by voltage level
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  ShuntCounterDefinitions& shuntCounterDefs_;  ///< the counter definitions to update
};

/**
 * @brief Lines by ids definitions
 */
struct LinesByIdDefinitions {
  std::unordered_map<inputs::Line::LineId, inputs::Line> linesMap;  ///< map of the input lines by line id
};

/**
 * @brief Algorithm to sort lines by ids
 */
class LinesByIdAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param linesByIdDefinition lines by id definitions to update
   */
  explicit LinesByIdAlgorithm(LinesByIdDefinitions& linesByIdDefinition);

  /**
   * @brief Performs the algorithm
   *
   * The algorithm extracts the lines of the node and put them into the map
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  LinesByIdDefinitions& linesByIdDefinition_;  ///< lines by id definitions to update
};

/// @brief Static var compensator definitions
struct StaticVarCompensatorDefinitions {
  std::vector<std::reference_wrapper<const inputs::StaticVarCompensator>> svarcs;  ///< List of static var compensators
};

/// @brief Static var compensator algorithm
class StaticVarCompensatorAlgorithm : public NodeAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param svarcsDefinitions the static var compensator definitions to update
   */
  explicit StaticVarCompensatorAlgorithm(StaticVarCompensatorDefinitions& svarcsDefinitions);

  /**
   * @brief Performs the algorithm
   *
   * the algorithm extracts all static var compensators connected to the node and add them to the definitions
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  StaticVarCompensatorDefinitions& svarcsDefinitions_;  ///< the static var compensator definitions to update
};

}  // namespace algo
}  // namespace dfl
