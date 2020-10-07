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

#include "Algo.h"
#include "Configuration.h"
#include "Job.h"
#include "NetworkManager.h"

#include <DYNSimulation.h>
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
   * @brief Context definition
   */
  struct ContextDef {
    std::string networkFilepath;  ///< network filepath
    std::string configFilepath;   ///< configuration filepath
    std::string dynawLogLevel;    ///< string representation of the dynawo log level
    std::string parFileDir;       ///< parameter file directory
    std::string dynawoResDir;     ///< DYNAWO resources
    std::string locale;           ///< localization
  };

 public:
  /**
   * @brief Constructor
   *
   * @param def The context definition
   */
  explicit Context(const ContextDef& def);

  /**
   * @brief Process context
   *
   * This perform all algorithms on nodes inputs of the network manager then perform all specific algorithm on the main connex component
   *
   * @returns status of the process
   */
  bool process();

  /**
   * @brief Export output files
   */
  void exportOutputs();

  /**
   * @brief Execute simulation
   */
  void execute();

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

  /**
   * @brief Create dynawo simulation
   *
   * @param in the job entry to use
   */
  void createSimulation(boost::shared_ptr<job::JobEntry>& job);

  /**
   * @brief Register a callback to call at each node
   *
   * @param cbk the callback to register
   */
  void onNodeOnMainConnexComponent(inputs::NetworkManager::ProcessNodeCallback&& cbk) {
    callbacksMainConnexComponent_.push_back(std::forward<inputs::NetworkManager::ProcessNodeCallback>(cbk));
  }

  /**
   * @brief Walk through all nodes in main connex components and apply callbacks
   */
  void walkNodesMain();

 private:
  ContextDef def_;                         ///< context definition
  inputs::NetworkManager networkManager_;  ///< network manager
  inputs::Configuration config_;           ///< configuration

  std::string basename_;                                                                   ///< basename for all files
  std::vector<inputs::NetworkManager::ProcessNodeCallback> callbacksMainConnexComponent_;  ///< List of algorithms to run in main components

  std::shared_ptr<inputs::Node> slackNode_;                     ///< computed slack node
  SlackNodeOrigin slackNodeOrigin_;                             ///< slack node origin
  std::vector<std::shared_ptr<inputs::Node>> mainConnexNodes_;  ///< main connex component
  std::vector<algo::GeneratorDefinition> generators_;           ///< generators found
  std::vector<algo::LoadDefinition> loads_;                     ///< loads found

  boost::shared_ptr<DYN::Simulation> simu_;  ///< Dynawo simulation
};
}  // namespace dfl
