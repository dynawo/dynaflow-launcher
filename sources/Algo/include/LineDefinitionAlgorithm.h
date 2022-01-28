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
 * @file  LineDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for lines header file
 *
 */

#pragma once

#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief Lines by ids definitions
 */
struct LinesByIdDefinitions {
  std::unordered_map<inputs::Line::LineId, inputs::Line> linesMap;  ///< map of the input lines by line id
};

/**
 * @brief Algorithm to sort lines by ids
 */
class LinesByIdAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param linesByIdDefinition lines by id definitions to update
   */
  explicit LinesByIdAlgorithm(LinesByIdDefinitions& linesByIdDefinition);

  /**
   * @brief Performs the algorithm
   *
   * The algorithm extracts the lines of the node and put them into the map
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  LinesByIdDefinitions& linesByIdDefinition_;  ///< lines by id definitions to update
};

}  // namespace algo
}  // namespace dfl
