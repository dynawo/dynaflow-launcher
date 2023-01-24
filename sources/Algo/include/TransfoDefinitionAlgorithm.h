//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  TransfoDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for transformers header file
 *
 */

#pragma once

#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief Transformers by ids definitions
 */
struct TransformersByIdDefinitions {
  std::unordered_map<inputs::Tfo::TfoId, inputs::Tfo> tfosMap;  ///< map of the input transformers by transformer id
};

/**
 * @brief Algorithm to sort transformers by ids
 */
class TransformersByIdAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param tfosByIdDefinition transformers by id definitions to update
   */
  explicit TransformersByIdAlgorithm(TransformersByIdDefinitions& tfosByIdDefinition);

  /**
   * @brief Performs the algorithm
   *
   * The algorithm extracts the transformers of the node and put them into the map
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  TransformersByIdDefinitions& tfosByIdDefinition_;  ///< transformers by id definitions to update
};

}  // namespace algo
}  // namespace dfl
