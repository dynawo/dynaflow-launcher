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
   */
  explicit Generator(const GeneratorId& genId, const std::vector<ReactiveCurvePoint>& curvePoints) : id{genId}, points(curvePoints) {}

  GeneratorId id;                          ///< generator id
  std::vector<ReactiveCurvePoint> points;  ///< reactive points
};

}  // namespace inputs
}  // namespace dfl
