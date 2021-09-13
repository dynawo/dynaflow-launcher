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
 * @file  Behaviours.h
 *
 * @brief Behaviours for nodes header file
 *
 */

#pragma once

#include <DYNGeneratorInterface.h>
#include <DYNVscConverterInterface.h>
#include <boost/optional.hpp>
#include <string>

namespace dfl {
namespace inputs {

/**
 * @brief Load behaviour
 */
struct Load {
  using LoadId = std::string;  ///< alias for id

  /**
   * @brief Constructor
   *
   * @param loadId the id of the load
   */
  explicit Load(const LoadId& loadId) : id{loadId} {}

  LoadId id;  ///< load id
};

/**
 * @brief Generator behaviour
 */
struct Generator {
  using GeneratorId = std::string;                                         ///< alias for id
  using ReactiveCurvePoint = DYN::GeneratorInterface::ReactiveCurvePoint;  ///< alias for point type
  using BusId = std::string;                                               ///< alias of BusId

  /**
   * @brief Constructor
   *
   * @param genId the id of the generator
   * @param curvePoints the list of reactive capabilities curve points
   * @param qmin minimum reactive power for the generator
   * @param qmax maximum reactive power for the generator
   * @param pmin minimum active power for the generator
   * @param pmax maximum active power for the generator
   * @param targetP target active power of the generator
   * @param regulatedBusId the Bus Id this generator is regulating
   * @param connectedBusId the Bus Id this generator is connected to
   */
  explicit Generator(const GeneratorId& genId, const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax,
                     double targetP, const BusId& regulatedBusId, const BusId& connectedBusId) :
      id{genId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax},
      targetP{targetP},
      regulatedBusId{regulatedBusId},
      connectedBusId{connectedBusId} {}

  GeneratorId id;                          ///< generator id
  std::vector<ReactiveCurvePoint> points;  ///< reactive points
  double qmin;                             ///< minimum reactive power
  double qmax;                             ///< maximum reactive power
  double pmin;                             ///< minimum active power
  double pmax;                             ///< maximum active power
  double targetP;                          ///< target active power of the generator
  const BusId regulatedBusId;              ///< regulated Bus Id
  const BusId connectedBusId;              ///< connected Bus Id
};

class HvdcLine;
/**
 * @brief Converter behaviour
 */
struct Converter {
  using ConverterId = std::string;  ///< alias for id
  using BusId = std::string;        ///< alias for bus id

  /**
   * @brief Constructor
   *
   * @param converterId the id of the converter
   * @param busId the id of the bus
   * @param hvdcLine the hvdc line this converter is contained into
   */
  Converter(const ConverterId& converterId, const BusId& busId, std::shared_ptr<HvdcLine> hvdcLine) :
      converterId{converterId},
      busId{busId},
      hvdcLine{hvdcLine} {}

  /// @brief Destructor
  virtual ~Converter() {}

  const ConverterId converterId;  ///< converter id
  const BusId busId;              ///< bus id
  // not const to allow further connection after construction
  std::shared_ptr<HvdcLine> hvdcLine;  ///< hvdc line this converter is contained into
};

/// @brief LCC converter
struct LCCConverter : public Converter {
  /**
   * @brief Constructor
   *
   * @param converterId the id of the converter
   * @param busId the id of the bus
   * @param hvdcLine the hvdc line this converter is contained into
   * @param powerFactor the power factor of the LCC converter
   */
  LCCConverter(const ConverterId& converterId, const BusId& busId, std::shared_ptr<HvdcLine> hvdcLine, double powerFactor) :
      Converter(converterId, busId, hvdcLine),
      powerFactor{powerFactor} {}

  const double powerFactor;  ///< power factor
};

/// @brief VSC converter
struct VSCConverter : public Converter {
  using ReactiveCurvePoint = DYN::VscConverterInterface::ReactiveCurvePoint;  ///< alias for point type

  /**
   * @brief Constructor
   *
   * @param converterId the id of the converter
   * @param busId the id of the bus
   * @param hvdcLine the hvdc line this converter is contained into
   * @param voltageRegulationOn optional boolean for the voltage regulation parameter, only used for VSC converters
   * @param qMax maximum reactive power of the converter
   * @param qMin minimum reactive power of the converter
   * @param points the reactive curve points
   */
  VSCConverter(const ConverterId& converterId, const BusId& busId, std::shared_ptr<HvdcLine> hvdcLine, bool voltageRegulationOn, double qMax, double qMin,
               const std::vector<ReactiveCurvePoint>& points) :
      Converter(converterId, busId, hvdcLine),
      qMax{qMax},
      qMin{qMin},
      points(points),
      voltageRegulationOn{voltageRegulationOn} {}

  const double qMax;                             ///< maximum q of the converter
  const double qMin;                             ///< minimum q of the converter
  const std::vector<ReactiveCurvePoint> points;  ///< reactive points
  const bool voltageRegulationOn;                ///< determines if voltage regulation is enabled
};

/// @brief Static var compensator (SVarC) behaviour
struct StaticVarCompensator {
  using SVarCid = std::string;  ///< alias for static var compensator id

  /**
   * @brief Constructor
   *
   * @param id the id of the SVarC
   * @param bMin the minimum susceptance value of the SVarC
   * @param bMax the maximum susceptance value of the SVarC
   * @param voltageSetPoint the voltage set point of the SVarC
   * @param VNom the nominal voltage of the bus of the SVarC
   * @param UMinActivation the low voltage activation threshold of the SVarC
   * @param UMaxActivation the high voltage activation threshold of the SVarC
   * @param USetPointMin the low voltage set point of the SVarC
   * @param USetPointMax the high voltage set point of the SVarC
   * @param b0 the initial susceptance value  of the SVarC
   * @param slope the slope (kV/MVar) of the SVarC
   */
  StaticVarCompensator(const SVarCid& id, double bMin, double bMax, double voltageSetPoint, double VNom, double UMinActivation, double UMaxActivation,
                       double USetPointMin, double USetPointMax, double b0, double slope) :
      id(id),
      bMin(bMin),
      bMax(bMax),
      voltageSetPoint(voltageSetPoint),
      VNom(VNom),
      UMinActivation(UMinActivation),
      UMaxActivation(UMaxActivation),
      USetPointMin(USetPointMin),
      USetPointMax(USetPointMax),
      b0(b0),
      slope(slope) {}

  const SVarCid id;              ///< the id of the SVarC
  const double bMin;             ///< the minimum susceptance value of the SVarC
  const double bMax;             ///< the maximum susceptance value of the SVarC
  const double voltageSetPoint;  ///< the voltage set point of the SVarC
  const double VNom;             ///< the nominal voltage of the bus of the SVarC
  const double UMinActivation;   ///< the low voltage activation threshold of the SVarC
  const double UMaxActivation;   ///< the high voltage activation threshold of the SVarC
  const double USetPointMin;     ///< the low voltage set point of the SVarC
  const double USetPointMax;     ///< the high voltage set point of the SVarC
  const double b0;               ///< the initial susceptance value of the SVarC
  const double slope;            ///< the slope (kV/MVar) of the SVarC
};

}  // namespace inputs
}  // namespace dfl
