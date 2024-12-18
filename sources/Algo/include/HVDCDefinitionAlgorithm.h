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
#include "DynamicDataBaseManager.h"
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
   * @param q the reactive power value of the converter
   * @param pMax the maximum active power capability value of the converter
   * @param points the reactive curve points of the converter, if any
   */
  VSCDefinition(const VSCId &id, double qMax, double qMin, double q, double pMax, const std::vector<ReactiveCurvePoint> &points)
      : id(id), qmax{qMax}, qmin{qMin}, q(q), pmax(pMax), pmin(-pMax), points(points) {}

  /**
   * @brief Equality operator for VSCDefinition
   * @param other the element to compare to
   * @returns true if current definition equals to @a other, false if not
   */
  bool operator==(const dfl::algo::VSCDefinition &other) const {
    return id == other.id && DYN::doubleEquals(pmax, other.pmax) && DYN::doubleEquals(pmin, other.pmin) && points.size() == other.points.size() &&
           std::equal(points.begin(), points.end(), other.points.begin(), comparePoints) && DYN::doubleEquals(qmax, other.qmax) &&
           DYN::doubleEquals(qmin, other.qmin);
  }

  VSCId id;                                ///< id of the converter
  double qmax;                             ///< maximum reactive power capability value
  double qmin;                             ///< minimum reactive power capability value
  double q;                                ///< reactive power capability value
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
  static bool comparePoints(const ReactiveCurvePoint &lhs, const ReactiveCurvePoint &rhs) {
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
    HvdcPVEmulationSetRpcl2Side1,
    HvdcPVDiagramPQEmulationSetRpcl2Side1,
    HvdcPVRpcl2Side1,
    HvdcPVDiagramPQRpcl2Side1,
    HvdcPVDanglingRpcl2Side1,
    HvdcPVDanglingDiagramPQRpcl2Side1,
    HvdcPVEmulationSetRpcl2Side2,
    HvdcPVDiagramPQEmulationSetRpcl2Side2,
    HvdcPVRpcl2Side2,
    HvdcPVDiagramPQRpcl2Side2,
    HvdcPVDanglingRpcl2Side2,
    HvdcPVDanglingDiagramPQRpcl2Side2
  };

  /**
   * @brief Check if the HVDC definition has a diagram model
   * @returns true if HVDC definition is using a model with diagrams, false if not
   */
  bool hasDiagramModel() const {
    return model == HVDCModel::HvdcPTanPhiDanglingDiagramPQ || model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPVDanglingDiagramPQ ||
           model == HVDCModel::HvdcPTanPhiDiagramPQ || model == HVDCModel::HvdcPQPropDiagramPQ || model == HVDCModel::HvdcPQPropDiagramPQEmulationSet ||
           model == HVDCModel::HvdcPVDiagramPQ || model == HVDCModel::HvdcPVDiagramPQEmulationSet ||
           model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side1 || model == HVDCModel::HvdcPVDiagramPQRpcl2Side1 ||
           model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1 || model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side2 ||
           model == HVDCModel::HvdcPVDiagramPQRpcl2Side2 || model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side2;
  }

  /**
   * @brief Check if the HVDC definition has an AC emulation model
   * @returns true if HVDC definition is using an AC emulation model, false if not
   */
  bool hasEmulationModel() const {
    return model == HVDCModel::HvdcPQPropEmulationSet || model == HVDCModel::HvdcPQPropDiagramPQEmulationSet || model == HVDCModel::HvdcPVEmulationSet ||
           model == HVDCModel::HvdcPVDiagramPQEmulationSet || model == HVDCModel::HvdcPVEmulationSetRpcl2Side1 ||
           model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side1 || model == HVDCModel::HvdcPVEmulationSetRpcl2Side2 ||
           model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side2;
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
           model == HVDCModel::HvdcPQPropDanglingDiagramPQ || model == HVDCModel::HvdcPVDangling || model == HVDCModel::HvdcPVDanglingDiagramPQ ||
           model == HVDCModel::HvdcPVDanglingRpcl2Side1 || model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1 ||
           model == HVDCModel::HvdcPVDanglingRpcl2Side2 || model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side2;
  }
  /**
   * @brief test is the  HVDC definition has a reactive power control loop 2 for connection to the secondary voltage control
   *
   * @return @b true if the  HVDC definition has a reactive power control loop 2 for connection to the secondary voltage control, @b false otherwise
   */
  bool hasRpcl2() const {
    return model == HVDCModel::HvdcPVEmulationSetRpcl2Side1 || model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side1 ||
           model == HVDCModel::HvdcPVRpcl2Side1 || model == HVDCModel::HvdcPVDiagramPQRpcl2Side1 || model == HVDCModel::HvdcPVDanglingRpcl2Side1 ||
           model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1 || model == HVDCModel::HvdcPVEmulationSetRpcl2Side2 ||
           model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side2 || model == HVDCModel::HvdcPVRpcl2Side2 || model == HVDCModel::HvdcPVDiagramPQRpcl2Side2 ||
           model == HVDCModel::HvdcPVDanglingRpcl2Side2 || model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side2;
  }
  /**
   * @brief test is the  HVDC definition has a reactive power control loop 2 for connection to the secondary voltage control
   *
   * @return @b true if the  HVDC definition has a reactive power control loop 2 for connection to the secondary voltage control, @b false otherwise
   */
  bool converterStationOnSide2() const {
    return model == HVDCModel::HvdcPVEmulationSetRpcl2Side2 || model == HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side2 ||
           model == HVDCModel::HvdcPVRpcl2Side2 || model == HVDCModel::HvdcPVDiagramPQRpcl2Side2 || model == HVDCModel::HvdcPVDanglingRpcl2Side2 ||
           model == HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side2;
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
   * @param vdcNom nominal dc voltage of the hvdc line in kV
   * @param pSetPoint active power set-point of the hvdc line in MW
   * @param rdc dc resistance of the hvdc line in Ohm
   * @param lossFactors loss factors for converters 1 and 2
   * @param ConverterStationSide1 whether the side1 of the hvdc should be connected to the side1 of the network
   */
  explicit HVDCDefinition(const HvdcLineId &id, const inputs::HvdcLine::ConverterType converterType, const ConverterId &converter1Id,
                          const BusId &converter1BusId, const boost::optional<bool> &converter1VoltageRegulationOn, const ConverterId &converter2Id,
                          const BusId &converter2BusId, const boost::optional<bool> &converter2VoltageRegulationOn, const Position position,
                          const HVDCModel &model, const std::array<double, 2> &powerFactors, double pMax, const boost::optional<VSCDefinition> &vscDefinition1,
                          const boost::optional<VSCDefinition> &vscDefinition2, const boost::optional<double> &droop, const boost::optional<double> &p0,
                          bool isConverter1Rectifier, const double vdcNom, const double pSetPoint, const double rdc, const std::array<double, 2> &lossFactors,
                          bool ConverterStationSide1)
      : id{id}, converterType{converterType}, converter1Id{converter1Id}, converter1BusId{converter1BusId},
        converter1VoltageRegulationOn{converter1VoltageRegulationOn}, converter2Id{converter2Id}, converter2BusId{converter2BusId},
        converter2VoltageRegulationOn{converter2VoltageRegulationOn}, position{position}, model{model}, powerFactors(powerFactors), pMax{pMax},
        vscDefinition1(vscDefinition1), vscDefinition2(vscDefinition2), droop(droop), p0(p0), isConverter1Rectifier{isConverter1Rectifier}, vdcNom(vdcNom),
        pSetPoint(pSetPoint), rdc(rdc), lossFactors(lossFactors), ConverterStationSide1(ConverterStationSide1) {}

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
  boost::optional<VSCDefinition> vscDefinition1;              ///< underlying VSC converter 1, irrelevant if type is not VSC
  boost::optional<VSCDefinition> vscDefinition2;              ///< underlying VSC converter 2, irrelevant if type is not VSC
  const boost::optional<double> droop;                        ///< active power droop value for HVDC, if it exists
  const boost::optional<double> p0;                           ///< active power setpoint value for HVDC, if it exists
  const bool isConverter1Rectifier;                           ///< whether converter 1 is rectifier
  const double vdcNom;                                        ///< nominal dc voltage of the hvdc line in kV
  const double pSetPoint;                                     ///< active power set-point of the hvdc line in MW
  const double rdc;                                           ///< dc resistance of the hvdc line in Ohm
  const std::array<double, 2> lossFactors;                    ///< loss factors for converters 1 and 2
  const bool ConverterStationSide1;                           ///< whether the side1 of the hvdc should be connected to the side1 of the network
};

/// @brief HVDC line definitions
struct HVDCLineDefinitions {
  using HvdcLineId = std::string;  ///< HvdcLine id definition

  using HvdcLineMap = std::map<HvdcLineId, HVDCDefinition>;  ///< Alias for map of hvdc line definition

  /**
   * @brief Alias for VSC map to their regulated bus
   *
   * We keep only one of the VSC definitions connected to a regulated bus as only one is required
   * to export the parameters
   */
  using BusVSCMap = std::unordered_map<HVDCDefinition::BusId, VSCDefinition::VSCId>;

  HvdcLineMap hvdcLines;  ///< the set of hvdc lines
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
   * @param busesToNumberOfRegulationMap mapping of busId and the number of generators/VSCs that regulates them
   * @param infiniteReactiveLimits the configuration data of whether we use infinite reactive limits
   * @param vscConverters list of VSC converters
   * @param manager the dynamic data base manager to use
   */
  HVDCDefinitionAlgorithm(HVDCLineDefinitions &hvdcLinesDefinitions, const inputs::NetworkManager::BusMapRegulating &busesToNumberOfRegulationMap,
                          bool infiniteReactiveLimits, const std::unordered_set<std::shared_ptr<inputs::Converter>> &vscConverters,
                          const inputs::DynamicDataBaseManager &manager);

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
  void operator()(const NodePtr &node, std::shared_ptr<AlgorithmsResults> &algoRes);

 private:
  /// @brief HVDC model definition
  struct HVDCModelDefinition {
    HVDCDefinition::HVDCModel model;  ///< the model to use
  };

 private:
  /**
   * @brief Compute the model definition
   * @param hvdcline the HVDC line to process
   * @param position the position of the extremities
   * @param type the converter type (VSC or LCC)
   * @returns the model definition to use
   */
  HVDCModelDefinition computeModel(const inputs::HvdcLine &hvdcline, HVDCDefinition::Position position, inputs::HvdcLine::ConverterType type) const;

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
  HVDCModelDefinition computeModelVSC(const inputs::HvdcLine &hvdcline, HVDCDefinition::Position position,
                                      HVDCDefinition::HVDCModel multipleVSCInfiniteReactive, HVDCDefinition::HVDCModel multipleVSCFiniteReactive,
                                      HVDCDefinition::HVDCModel oneVSCInfiniteReactive, HVDCDefinition::HVDCModel oneVSCFiniteReactive) const;

  /**
   * @brief Get or create HVDC line definition
   *
   * Creates the element if not already existing
   * @param hvdcLine the HVDC line to process
   * @returns the pair (element, status), where element is the inserted or got element and status true if element was already inserted before or false if not
   */
  std::pair<std::reference_wrapper<HVDCDefinition>, bool> getOrCreateHvdcLineDefinition(const inputs::HvdcLine &hvdcLine);

 private:
  HVDCLineDefinitions &hvdcLinesDefinitions_;                                     ///< The HVDC lines definitions to update
  const inputs::NetworkManager::BusMapRegulating &busesToNumberOfRegulationMap_;  ///< mapping of busId and the number of generators that regulates them
  const bool infiniteReactiveLimits_;                                             ///< whether we use infinite reactive limits
  std::unordered_map<inputs::Converter::ConverterId, std::shared_ptr<inputs::Converter>> vscConverters_;  ///< List of VSC converters to use
  std::unordered_map<std::string, inputs::AssemblingDataBase::HvdcLineConverterSide>
      hvdcLinesInSVC_;  ///< If a hvdc line id is in this map then it belongs to a secondary voltage control area
};

}  // namespace algo
}  // namespace dfl
