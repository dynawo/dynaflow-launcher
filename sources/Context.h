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
#include "DynamicDataBaseManager.h"
#include "Contingencies.h"
#include "NetworkManager.h"

#include <JOBJobEntry.h>
#include <boost/filesystem.hpp>

namespace dfl {
/**
 * @brief Dynaflow launcher context
 *
 * This class aims to link together the different parts of DFL:
 * - inputs (network and configuration)
 * - algorithms
 * - outputs (writers)
 * - dynawo simulation
 */
class Context {
 public:
  /// @brief The kind of simulation that is requested
  enum class SimulationKind {
    STEADY_STATE_CALCULATION,  ///< A steady-state calculation
    SECURITY_ANALYSIS          ///< A security analysis for a given list of contingencies
  };

  /**
   * @brief Context definition
   */
  struct ContextDef {
    SimulationKind simulationKind;               ///< kind of simulation requested (steady-state or security analysis)
    boost::filesystem::path networkFilepath;     ///< network filepath
    boost::filesystem::path settingFilePath;     ///< setting file path for dynamic data base
    boost::filesystem::path assemblingFilePath;  ///< assembling file path for dynamic data base
    std::string contingenciesFilepath;           ///< contigencies filepath (for Security Analysis simulation)
    std::string dynawoLogLevel;                  ///< string representation of the dynawo log level
    boost::filesystem::path parFileDir;          ///< parameter file directory
    boost::filesystem::path dynawoResDir;        ///< DYNAWO resources
    std::string locale;                          ///< localization
  };

 public:
  /**
   * @brief Constructor
   *
   * @param def The context definition
   * @param config configuration to use
   */
  Context(const ContextDef& def, const inputs::Configuration& config);

  /**
   * @brief Retrieve the basename of current simulation
   *
   * @returns basename used for current simulation
   */
  const std::string& basename() const {
    return basename_;
  }

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
   *
   * This create the job entry, exports all intermediate files for dynawo simulation and create dynawo simulation
   */
  void exportOutputs();

  /**
   * @brief Execute simulation
   *
   * Constructs, initializes and execute Dynawo simulation.
   * After call of the function, Traces must be re-initialized
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
   * @returns true if compatible, false if not
   */
  bool checkConnexity() const;

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

  /**
   * @brief Filter partially connected dynamic models
   *
   * Remove models definitions with partial connectivity
   */
  void filterPartiallyConnectedDynamicModels();

 private:
  ContextDef def_;                                         ///< context definition
  inputs::NetworkManager networkManager_;                  ///< network manager
  inputs::DynamicDataBaseManager dynamicDataBaseManager_;  ///< dynamic model configuration manager
  const inputs::Configuration& config_;                    ///< configuration
  inputs::Contingencies contingencies_;                    ///< contingencies if the simulation is a Security Analysis

  std::string basename_;                                                                   ///< basename for all files
  std::vector<inputs::NetworkManager::ProcessNodeCallback> callbacksMainConnexComponent_;  ///< List of algorithms to run in main components

  std::shared_ptr<inputs::Node> slackNode_;                              ///< computed slack node
  SlackNodeOrigin slackNodeOrigin_;                                      ///< slack node origin
  std::vector<std::shared_ptr<inputs::Node>> mainConnexNodes_;           ///< main connex component
  std::vector<algo::GeneratorDefinition> generators_;                    ///< generators found
  std::vector<algo::LoadDefinition> loads_;                              ///< loads found
  algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap hvdcLines_;  ///< hvdc lines found
  algo::GeneratorDefinitionAlgorithm::BusGenMap busesWithDynamicModel_;  ///< map of bus ids to a generator that regulates them
  algo::DynamicModelDefinitions dynamicModels_;                          ///< model definitions
  algo::ShuntCounterDefinitions counters_;                               ///< shunt counters definitions
  algo::LinesByIdDefinitions linesById_;                                 ///< Lines by ids definition

  boost::shared_ptr<job::JobEntry> jobEntry_;                 ///< Dynawo job entry
  std::vector<boost::shared_ptr<job::JobEntry>> jobsEvents_;  ///< Dynawo job entries for contingencies

  /// @brief Check all elements in contingencies have corresponding dynamic models
  void checkContingencies();
  /// @brief Check if a branch is a Line
  /// @param branchId static identifier of branch
  bool isLine(const std::string& branchId);
  /// @brief Check if a branch is a two-windings transformer
  /// @param branchId static identifier of branch
  bool isTwoWTransformer(const std::string& branchId);
  /// @brief Build JOBS, DYD, PAR files for each contingency
  void exportOutputsContingencies();
  /// @brief Build JOBS, DYD, PAR files for a given contingency
  void exportOutputsContingency(const inputs::Contingencies::ContingencyDefinition& c);
  /// @brief Initialization of additional instances of network manager
  void initNetworkManager(dfl::inputs::NetworkManager& networkManager, std::vector<std::shared_ptr<inputs::Node>>& mainConnexNodes);
};
}  // namespace dfl
