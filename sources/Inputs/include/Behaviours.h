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

}  // namespace inputs
}  // namespace dfl
