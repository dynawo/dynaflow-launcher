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
 * @file  Algo.h
 *
 * @brief Dynaflow launcher algorithms header file
 *
 */

#pragma once

#include "Node.h"

namespace dfl {
/// @brief Namespace for algorithms to perform on netpork
namespace algo {

/**
 * @brief Algorithm to perform on nodes to find the slack node
 */
class SlackNodeAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * @param slackNode the slack node to update with the algorithm
   */
  explicit SlackNodeAlgorithm(std::shared_ptr<inputs::Node>& slackNode);

  /**
  * @brief Perform elementary step to determine the slack node
  *
  * The used criteria: the slack node is the node which has the higher voltage level then the higher number of connected nodes (in that order)
  *
  * @param node the node to process
  */
  void operator()(const std::shared_ptr<inputs::Node>& node);

 private:
  std::shared_ptr<inputs::Node>& slackNode_;  ///< The slack node to update
};
}  // namespace algo
}  // namespace dfl
