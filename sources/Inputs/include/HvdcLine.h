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
 * @file  HvdcLine.h
 *
 * @brief HvdcLine structure header file
 *
 */

#pragma once

#include "Behaviours.h"

#include <boost/optional.hpp>
#include <string>

namespace dfl {
namespace inputs {

/**
 * @brief Hvdc Line structure
 */
class HvdcLine {
 public:
  using HvdcLineId = std::string;   ///< HvdcLine id definition
  using ConverterId = std::string;  ///< alias for converter id
  using BusId = std::string;        ///< alias for bus id

  /// @brief Type of converter
  enum class ConverterType {
    VSC,  ///< voltage source converter
    LCC   ///< line-commutated converter
  };

  /**
   * @brief HVDC Active power control information
   */
  struct ActivePowerControl {
    /**
     * @brief Constructor
     *
     * @param droop the droop value
     * @param p0 the p0 value
     */
    ActivePowerControl(double droop, double p0) : droop(droop), p0(p0) {}

    double droop;  ///< Droop value
    double p0;     ///< p0 value
  };

 public:
  /**
   * @brief Build a HVDC line object
   *
   * the builder builds the HVDC line object and performs the connections to converters
   *
   * @param id the hvdc line id
   * @param converterType type of converter of the hvdc line
   * @param converter1 the first converter
   * @param converter2 the second converter
   * @param activePowerControl the active power control information, when present in the network
   * @param pMax the maximum p
   * @param isConverter1Rectifier whether converter 1 is rectifier
   * @param vdcNom nominal voltage of the hvdc line in kV
   * @param pSetPoint active power set-point of the hvdc line in MW
   * @param rdc dc resistance of the hvdc line in Ohm
   * @param lossFactor1 loss factor of the converter 1
   * @param lossFactor2 loss factor of the converter 2
   * @param powerFactor1 power factor of the lcc converter 1
   * @param powerFactor2 power factor of the lcc converter 2
   *
   * @return HVDC line object
   */
  static std::shared_ptr<HvdcLine> build(const std::string& id,
                                          const ConverterType converterType,
                                          const std::shared_ptr<Converter>& converter1,
                                          const std::shared_ptr<Converter>& converter2,
                                          const boost::optional<ActivePowerControl>& activePowerControl,
                                          double pMax,
                                          bool isConverter1Rectifier,
                                          const double vdcNom,
                                          const double pSetPoint,
                                          const double rdc,
                                          const double lossFactor1,
                                          const double lossFactor2,
                                          const boost::optional<double> powerFactor1 = boost::none,
                                          const boost::optional<double> powerFactor2 = boost::none);

 public:
  const HvdcLineId id;                                           ///< HvdcLine id
  const ConverterType converterType;                             ///< type of converter
  const std::shared_ptr<Converter> converter1;                   ///< first converter
  const std::shared_ptr<Converter> converter2;                   ///< second converter
  const boost::optional<ActivePowerControl> activePowerControl;  ///< active power control information
  const double pMax;                                             ///< maximum p
  const bool isConverter1Rectifier;                              ///< whether converter 1 is rectifier
  const double vdcNom;
  const double pSetPoint;
  const double rdc;
  const double lossFactor1;
  const double lossFactor2;
  const boost::optional<double> powerFactor1;
  const boost::optional<double> powerFactor2;

 private:
  /**
   * @brief Constructor
   *
   * @param id the hvdc line id
   * @param converterType type of converter of the hvdc line
   * @param converter1 the first converter
   * @param converter2 the second converter
   * @param activePowerControl the active power control information, when present in the network
   * @param pMax the maximum p
   * @param isConverter1Rectifier whether converter 1 is rectifier
   * @param vdcNom nominal voltage of the hvdc line in kV
   * @param pSetPoint active power set-point of the hvdc line in MW
   * @param rdc dc resistance of the hvdc line in Ohm
   * @param lossFactor1 loss factor of the converter 1
   * @param lossFactor2 loss factor of the converter 2
   * @param powerFactor1 power factor of the lcc converter 1
   * @param powerFactor2 power factor of the lcc converter 2
   */
  HvdcLine(const std::string& id,
            const ConverterType converterType,
            const std::shared_ptr<Converter>& converter1,
            const std::shared_ptr<Converter>& converter2,
            const boost::optional<ActivePowerControl>& activePowerControl,
            double pMax,
            bool isConverter1Rectifier,
            const double vdcNom,
            const double pSetPoint,
            const double rdc,
            const double lossFactor1,
            const double lossFactor2,
            const boost::optional<double> powerFactor1,
            const boost::optional<double> powerFactor2);
};
}  // namespace inputs
}  // namespace dfl
