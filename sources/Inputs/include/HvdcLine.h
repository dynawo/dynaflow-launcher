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
/// @brief Namespace for inputs of Dynaflow launcher
namespace inputs {

/**
 * @brief Hvdc Line structure
 */
struct HvdcLine {
  using HvdcLineId = std::string;  ///< HvdcLine id definition

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
   * @param converter1 first converter connected to the hvdc line
   * @param converter2 second converter connected to the hvdc line
   */
  HvdcLine(const std::string& id, const ConverterType converterType, inputs::ConverterInterface converter1, inputs::ConverterInterface converter2);

  const HvdcLineId id;                    ///< HvdcLine id
  const ConverterType converterType;      ///< type of converter
  inputs::ConverterInterface converter1;  ///< first converter
  inputs::ConverterInterface converter2;  ///< second converter
};
}  // namespace inputs
}  // namespace dfl
