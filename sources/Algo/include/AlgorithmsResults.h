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
 * @file  AlgorithmsResults.h
 *
 * @brief Dynaflow launcher algorithms results header file
 *
 */
#pragma once

namespace dfl {

namespace algo {
/**
 * @brief Algorithms results class
 *
 */
class AlgorithmsResults {
 public:
  /**
   * @brief Construct a new Algorithms Results object
   *
   */
  AlgorithmsResults() : isAtLeastOneGeneratorRegulating(false) {}

  bool isAtLeastOneGeneratorRegulating;  ///< boolean determining if at least one generator is regulating the voltage
};

}  // namespace algo
}  // namespace dfl
