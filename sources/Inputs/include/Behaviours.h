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

  /**
   * @brief Constructor
   *
   * @param genId the id of the generator
   * @param curvePoints the list of reactive capabilities curve points
   * @param qmin minimum reactive power for the generator
   * @param qmax maximum reactive power for the generator
   * @param pmin minimum active power for the generator
   * @param pmax maximum active power for the generator
   */
  explicit Generator(const GeneratorId& genId, const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax) :
      id{genId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax} {}

  GeneratorId id;                          ///< generator id
  std::vector<ReactiveCurvePoint> points;  ///< reactive points
  double qmin;                             ///< minimum reactive power
  double qmax;                             ///< maximum reactive power
  double pmin;                             ///< minimum active power
  double pmax;                             ///< maximum active power
};

class HvdcLine;
/**
 * @brief Converter interface behaviour
 */
struct ConverterInterface {
  using ConverterInterfaceId = std::string;  ///< alias for id
  using BusId = std::string;                 ///< alias for bus id

  /**
   * @brief Constructor
   *
   * @param converterId the id of the converter
   * @param busId the id of the bus
   * @param voltageRegulationOn optional boolean for the voltage regulation parameter, only used for VSC converters
   * @param hvdcLine the hvdc line this converter is contained into
   */
  explicit ConverterInterface(const ConverterInterfaceId& converterId, BusId busId, boost::optional<bool> voltageRegulationOn = {},
                              boost::shared_ptr<HvdcLine> hvdcLine = nullptr) :
      converterId{converterId},
      busId{busId},
      voltageRegulationOn{voltageRegulationOn},
      hvdcLine{hvdcLine} {}
  /**
   * @brief Determines if two converter interfaces are equal
   *
   * @param other the other converter to be compared against
   * 
   * @return status of the comparaison
   */
  bool operator==(const ConverterInterface& other) const {
    return converterId == other.converterId && busId == other.busId && voltageRegulationOn == other.voltageRegulationOn && hvdcLine == other.hvdcLine;
  }

  ConverterInterfaceId converterId;           ///< converter id
  BusId busId;                                ///< bus id
  boost::optional<bool> voltageRegulationOn;  ///< voltage regulation parameter, for VSC converters only
  boost::shared_ptr<HvdcLine> hvdcLine;       ///< hvdc line this converter is contained into
};
}  // namespace inputs
}  // namespace dfl
