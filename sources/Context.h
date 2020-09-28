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
 * @file  Context.h
 *
 * @brief Dynaflow launcher context header file
 *
 */

#pragma once

#include "NetworkManager.h"

namespace dfl {
/**
 * @brief Dynaflow launcher context
 *
 * This contains the algorithms that need to be applied on inputs to produce outputs.
 *
 * It also contain the inputs and outputs managers
 */
class Context {
 public:
  /**
   * @brief Constructor
   *
   * @param networkFilepath network filepath
   */
  explicit Context(const std::string& networkFilepath);

  /**
   * @brief Process context
   *
   * This perform all algorithms on inputs and write outputs
   */
  void process();

 private:
  /**
  * @brief Perform elementary step to determine the slack node
  *
  * The used criteria: the slack node is the node which has the higher voltage level then the higher number of connected nodes (in that order)
  *
  * @param node the node to process
  */
  void processSlackNode(const boost::shared_ptr<inputs::Node>& node);

 private:
  inputs::NetworkManager networkManager_;  ///< network manager

  boost::shared_ptr<inputs::Node> slack_node_;  ///< computed slack node
};
}  // namespace dfl
