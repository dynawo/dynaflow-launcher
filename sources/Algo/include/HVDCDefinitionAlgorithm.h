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
 * @file  HVDCDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for hvdcs header file
 *
 */

#pragma once

#include "AlgorithmsResults.h"
#include "HvdcLine.h"
#include "NetworkManager.h"
#include "Node.h"

#include <DYNCommon.h>

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

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
    return id == other.id && DYN::doubleEquals(pmax, other.pmax) && DYN::doubleEquals(pmin, other.pmin) && points.size() == other.points.size() &&
           std::equal(points.begin(), points.end(), other.points.begin(), comparePoints) && DYN::doubleEquals(qmax, other.qmax) &&
           DYN::doubleEquals(qmin, other.qmin);
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
    return DYN::doubleEquals(lhs.p, rhs.p) && DYN::doubleEquals(lhs.qmax, rhs.qmax) && DYN::doubleEquals(lhs.qmin, rhs.qmin);
  }
};

/**
 * @brief Hvdc line definition for algorithms
 */
class HVDCDefinition {
 public:
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
    HvdcPQPropDiagramPQEmulationSet,
    HvdcPQPropEmulationSet,
    HvdcPV,
    HvdcPVDangling,
    HvdcPVDanglingDiagramPQ,
    HvdcPVDiagramPQ,
    HvdcPVDiagramPQEmulationSet,
    HvdcPVEmulationSet,
  };

  /**
   * @brief Check if the HVDC definition has a diagram model
   * @returns true if HVDC definition is using a model with diagrams, false if not
   */
  bool hasDiagramModel() const {
    return model == HVDCModel::HvdcPTanPhiDanglingDiagramPQ || model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPVDanglingDiagramPQ ||
           model == HVDCModel::HvdcPTanPhiDiagramPQ || model == HVDCModel::HvdcPQPropDiagramPQ || model == HVDCModel::HvdcPQPropDiagramPQEmulationSet ||
           model == HVDCModel::HvdcPVDiagramPQ || model == HVDCModel::HvdcPVDiagramPQEmulationSet;
  }

  /**
   * @brief Check if the HVDC definition has an AC emulation model
   * @returns true if HVDC definition is using an AC emulation model, false if not
   */
  bool hasEmulationModel() const {
    return model == HVDCModel::HvdcPQPropEmulationSet || model == HVDCModel::HvdcPQPropDiagramPQEmulationSet || model == HVDCModel::HvdcPVEmulationSet ||
           model == HVDCModel::HvdcPVDiagramPQEmulationSet;
  }

  /**
   * @brief Check if the HVDC definition has a model with a proportional Q regulation
   * @returns true if HVDC definition is using a modelwith a proportional Q regulation, false if not
   */
  bool hasPQPropModel() const {
    return model == HVDCModel::HvdcPQPropDangling || model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPQProp ||
           model == HVDCModel::HvdcPQPropDiagramPQ || model == HVDCModel::HvdcPQPropEmulationSet || model == HVDCModel::HvdcPQPropDiagramPQEmulationSet;
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
   * @param p0 the active power setpoint for HVDC, if the value exists
   * @param isConverter1Rectifier whether converter 1 is rectifier
   */
  explicit HVDCDefinition(const HvdcLineId& id, const inputs::HvdcLine::ConverterType converterType, const ConverterId& converter1Id,
                          const BusId& converter1BusId, const boost::optional<bool>& converter1VoltageRegulationOn, const ConverterId& converter2Id,
                          const BusId& converter2BusId, const boost::optional<bool>& converter2VoltageRegulationOn, const Position position,
                          const HVDCModel& model, const std::array<double, 2>& powerFactors, double pMax, const boost::optional<VSCDefinition>& vscDefinition1,
                          const boost::optional<VSCDefinition>& vscDefinition2, const boost::optional<double>& droop, const boost::optional<double>& p0,
                          bool isConverter1Rectifier) :
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
      droop(droop),
      p0(p0),
      isConverter1Rectifier{isConverter1Rectifier} {}

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
  const boost::optional<double> p0;                           ///< active power setpoint value for HVDC, if it exists
  const bool isConverter1Rectifier;                           ///< whether converter 1 is rectifier
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
class HVDCDefinitionAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * @param hvdcLinesDefinitions the HVDC line definitions to update
   * @param infiniteReactiveLimits the configuration data of whether we use infinite reactive limits
   * @param vscConverters list of VSC converters
   * @param mapBusVSCConvertersBusId the mapping of buses and their number of VSC converters regulating them
   * @param serviceManager the dynawo service manager to use
   */
  HVDCDefinitionAlgorithm(HVDCLineDefinitions& hvdcLinesDefinitions, bool infiniteReactiveLimits,
                          const std::unordered_set<std::shared_ptr<inputs::Converter>>& vscConverters,
                          const inputs::NetworkManager::BusMapRegulating& mapBusVSCConvertersBusId,
                          const boost::shared_ptr<DYN::ServiceManagerInterface>& serviceManager);

  /**
   * @brief Perform the algorithm
   *
   * Update the list with the hvdc line of the converters of the node.
   * This function also put the position of the converters compared to the main connex component in the newly created hvdc line definition.
   * Pre-condition: the nodes used as parameter of this operator should be nodes of the main connex component only
   *
   * @param node the node to process
   * @param algoRes pointer to algorithms results class
   */
  void operator()(const NodePtr& node, std::shared_ptr<AlgorithmsResults>& algoRes);

 private:
  /// @brief HVDC model definition
  struct HVDCModelDefinition {
    using VSCBusPair = std::pair<HVDCDefinition::BusId, VSCDefinition::VSCId>;  ///< Alias for pair of bus id and VSC id
    /// @brief Hash structure for VSCBusPair
    struct VSCBusPairHash {
      /**
       * @brief Operator to retrieve hash value
       *
       * @param pair the VSCBusPair to hash
       * @return the computed hash
       */
      std::size_t operator()(const VSCBusPair& pair) const noexcept;
    };
    using VSCBusPairSet = std::unordered_set<VSCBusPair, VSCBusPairHash>;  ///< Alias for set of VSCBusPair

    HVDCDefinition::HVDCModel model;           ///< the model to use
    VSCBusPairSet vscBusIdsMultipleRegulated;  ///< the VSCs and their bus that are involved in multiple VSC regulations
  };

 private:
  /**
   * @brief Compute the model definition
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @param type the converter type (VSC or LCC)
   * @param node the node currently processed (one of the ends of the HVDC line)
   * @returns the model definition to use
   */
  HVDCModelDefinition computeModel(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, inputs::HvdcLine::ConverterType type,
                                   const NodePtr& node) const;

  /**
   * @brief Compute the model definition for VSC converters
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @param multipleVSCInfiniteReactive model to use in case of multiple VSC and infinite reactive limits used
   * @param multipleVSCFiniteReactive model to use in case of multiple VSC and finite reactive limits used
   * @param oneVSCInfiniteReactive model to use in case of only one VSC and infinite reactive limits used
   * @param oneVSCFiniteReactive model to use in case of only one VSC and finite reactive limits used
   * @param node the node currently processed
   * @returns the model definition to use
   */
  HVDCModelDefinition computeModelVSC(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position,
                                      HVDCDefinition::HVDCModel multipleVSCInfiniteReactive, HVDCDefinition::HVDCModel multipleVSCFiniteReactive,
                                      HVDCDefinition::HVDCModel oneVSCInfiniteReactive, HVDCDefinition::HVDCModel oneVSCFiniteReactive,
                                      const NodePtr& node) const;

  /**
   * @brief Get the VSC converters Connected By Switches
   *
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @param node the node currently processed
   *
   * @return The set, if relevant, of VSC bus pair listing the VSC converters connected by switch on current line and node
   */
  HVDCModelDefinition::VSCBusPairSet getVSCConnectedBySwitches(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, const NodePtr& node) const;

  /**
   * @brief Get the Buses By Position
   *
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @return The set of VSC bus pair according to the position
   */
  HVDCModelDefinition::VSCBusPairSet getBusesByPosition(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position) const;

  /**
   * @brief Get the list of buses regulated by multiple VSC
   * @param hvdcline the hvdc line to process
   * @param position the position of the extremities
   * @returns the list of pairs (bus, VSC) involved in multiple VSC regulation
   */
  HVDCModelDefinition::VSCBusPairSet getBusRegulatedByMultipleVSC(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position) const;

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
  boost::shared_ptr<DYN::ServiceManagerInterface> serviceManager_;            ///< Service manager to use
  std::unordered_map<inputs::Converter::ConverterId, std::shared_ptr<inputs::Converter>> vscConverters_;  ///< List of VSC converters to use
};

}  // namespace algo
}  // namespace dfl
