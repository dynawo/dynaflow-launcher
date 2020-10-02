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
#include "Job.h"
#include "NetworkManager.h"

#include <boost/filesystem.hpp>

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
   * @param dynawLogLevel string representation of the dynawo log level
   * @param parFileDir parameter file directory
   */
  Context(const std::string& networkFilepath, const std::string& configFilepath, const std::string& dynawLogLevel, const std::string& parFileDir);

  /**
   * @brief Process context
   *
   * This perform all algorithms on inputs
   *
   * @returns status of the process
   */
  bool process();

  /**
   * @brief Export output files
   */
  void exportOutputs();

 private:
  /// @brief Slack node origin
  enum class SlackNodeOrigin {
    FILE = 0,  ///< Slack node comes from network input file
    ALGORITHM  ///< Slack node is computed via automatic algorithm
  };

 private:
  /**
   * @brief Check connexity
   *
   * Checks that the found slack node is compatible with the main connex component
   *
   * @returns true if comaptible, false if not
   */
  bool checkConnexity() const;

 private:
  inputs::NetworkManager networkManager_;  ///< network manager
  inputs::Configuration config_;           ///< configuration

  std::string basename_;                      ///< basename for all files
  const std::string dynawLogLevel_;           ///< Dynawo log level for simulation to export
  const boost::filesystem::path parFileDir_;  ///< constant parameter files directory

  std::shared_ptr<inputs::Node> slackNode_;                     ///< computed slack node
  SlackNodeOrigin slackNodeOrigin_;                             ///< slack node origin
  std::vector<std::shared_ptr<inputs::Node>> mainConnexNodes_;  ///< main connex component

  std::unique_ptr<outputs::Job> jobWriter_;  ///< Job writer
};
}  // namespace dfl
