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
struct HvdcLine {
  using HvdcLineId = std::string;   ///< HvdcLine id definition
  using ConverterId = std::string;  ///< alias for converter id
  using BusId = std::string;        ///< alias for bus id

  /// @brief Type of converter
  enum class ConverterType {
    VSC,  ///< voltage source converter
    LCC   ///< line-commutated converter
  };

  /**
   * @brief Constructor
   *
   * @param id the hvdc line id
   * @param converterType type of converter of the hvdc line  
   * @param converter1_id first converter id 
   * @param converter1_busId first converter bus id
   * @param converter1_voltageRegulationOn firt converter voltage regulation parameter, for VSC converters only
   * @param converter2_id second converter id 
   * @param converter2_busId second converter bus id
   * @param converter2_voltageRegulationOn second converter voltage regulation parameter, for VSC converters only
   */
  HvdcLine(const std::string& id, const ConverterType converterType, const ConverterId& converter1_id, const BusId& converter1_busId,
           const boost::optional<bool>& converter1_voltageRegulationOn, const ConverterId& converter2_id, const BusId& converter2_busId,
           const boost::optional<bool>& converter2_voltageRegulationOn);

  const HvdcLineId id;                                         ///< HvdcLine id
  const ConverterType converterType;                           ///< type of converter
  const ConverterId converter1_id;                             ///< first converter id
  const BusId converter1_busId;                                ///< first converter bus id
  const boost::optional<bool> converter1_voltageRegulationOn;  ///< firt converter voltage regulation parameter, for VSC converters only
  const ConverterId converter2_id;                             ///< second converter id
  const BusId converter2_busId;                                ///< second converter bus id
  const boost::optional<bool> converter2_voltageRegulationOn;  ///< second converter voltage regulation parameter, for VSC converters only
};
}  // namespace inputs
}  // namespace dfl
