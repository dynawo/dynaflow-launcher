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

#include "Configuration.h"
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
   * @param configFilepath configuration filepath
   */
  Context(const std::string& networkFilepath, const std::string& configFilepath);

  /**
   * @brief Process context
   *
   * This perform all algorithms on inputs and write outputs
   */
  void process();

 private:
  /// @brief Slack node origin
  enum class SlackNodeOrigin {
    FILE = 0,  ///< Slack node comes from network input file
    ALGORITHM  ///< Slack node is computed via automatic algorithm
  };

 private:
  inputs::NetworkManager networkManager_;  ///< network manager
  inputs::Configuration config_;           ///< configuration

  std::shared_ptr<inputs::Node> slackNode_;  ///< computed slack node
  SlackNodeOrigin slackNodeOrigin_;          ///< slack node origin
};
}  // namespace dfl
