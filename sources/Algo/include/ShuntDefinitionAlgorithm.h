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
 * @file  ShuntDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for shunts header file
 *
 */

#pragma once

#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief Shunt counter definition
 */
struct ShuntCounterDefinitions {
  std::unordered_map<inputs::VoltageLevel::VoltageLevelId, unsigned int> nbShunts;  ///< Number of shunts by voltage level
};

/**
 * @brief Counter of shunts by voltage levels
 */
class ShuntCounterAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param shuntCounterDefs the counter definitions to update
   */
  explicit ShuntCounterAlgorithm(ShuntCounterDefinitions& shuntCounterDefs);

  /**
   * @brief Performs the algorithm
   *
   * The algorithm counts the number of shunts by voltage level
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  ShuntCounterDefinitions& shuntCounterDefs_;  ///< the counter definitions to update
};

}  // namespace algo
}  // namespace dfl
