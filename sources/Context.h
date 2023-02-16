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

#include "AlgorithmsResults.h"
#include "Configuration.h"
#include "ContingenciesManager.h"
#include "ContingencyValidationAlgorithm.h"
#include "DynModelDefinitionAlgorithm.h"
#include "DynamicDataBaseManager.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "LoadDefinitionAlgorithm.h"
#include "MainConnexComponentAlgorithm.h"
#include "NetworkManager.h"
#include "SVarCDefinitionAlgorithm.h"
#include "ShuntDefinitionAlgorithm.h"
#include "SlackNodeAlgorithm.h"

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
  using ProcessNodeCallBackMainComponent =
      std::function<void(const std::shared_ptr<inputs::Node>&,
                         std::shared_ptr<dfl::algo::AlgorithmsResults>&)>;  ///< Callback for node algorithm on main topological island
                                                                            /**
   * @brief Context definition
   */
  struct ContextDef {
    dfl::inputs::Configuration::StartingPointMode startingPointMode;  ///< starting point mode of the simulation
    dfl::inputs::Configuration::SimulationKind simulationKind;        ///< kind of simulation requested (steady-state or security analysis)
    boost::filesystem::path networkFilepath;                          ///< network filepath
    boost::filesystem::path settingFilePath;                          ///< setting file path for dynamic data base
    boost::filesystem::path assemblingFilePath;                       ///< assembling file path for dynamic data base
    boost::filesystem::path contingenciesFilePath;                    ///< contigencies file path for Security Analysis simulation
    std::string dynawoLogLevel;                                       ///< string representation of the dynawo log level
    boost::filesystem::path parFileDir;                               ///< parameter file directory
    boost::filesystem::path dynawoResDir;                             ///< DYNAWO resources
    std::string locale;                                               ///< localization
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
   * @brief Export results file
   *
   * @param simulationOk the simulation was ok
   *
   * This create a json output file of the results
   */
  void exportResults(bool simulationOk);

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
   * @brief Register a callback to call at each node of the main topological island
   *
   * @param cbk the callback to register
   */
  void onNodeOnMainConnexComponent(ProcessNodeCallBackMainComponent&& cbk) {
    callbacksMainConnexComponent_.push_back(std::forward<ProcessNodeCallBackMainComponent>(cbk));
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

  /// @brief Execute security analysis by running simulations for the base case and all the valid contingencies
  void executeSecurityAnalysis();

  /// @brief Prepare the job file
  void exportOutputJob();

  /// @brief Prepare the output files required to simulate the valid contingencies
  void exportOutputsContingencies();

  /// @brief Prepare the output files required to simulate a given contingency
  /// @param contingency the contingency
  /// @param elementsNetworkType ids of network elements with a network type
  void exportOutputsContingency(const inputs::Contingency& contingency, const std::unordered_set<std::string>& elementsNetworkType);

 private:
  ContextDef def_;                                         ///< context definition
  inputs::NetworkManager networkManager_;                  ///< network manager
  inputs::DynamicDataBaseManager dynamicDataBaseManager_;  ///< dynamic model configuration manager
  inputs::ContingenciesManager contingenciesManager_;      ///< contingencies manager in a Security Analysis
  const inputs::Configuration config_;                     ///< configuration

  std::string basename_;                                                        ///< basename for all files
  std::vector<ProcessNodeCallBackMainComponent> callbacksMainConnexComponent_;  ///< List of algorithms to run in main components

  std::shared_ptr<inputs::Node> slackNode_;                                          ///< computed slack node
  SlackNodeOrigin slackNodeOrigin_;                                                  ///< slack node origin
  std::vector<std::shared_ptr<inputs::Node>> mainConnexNodes_;                       ///< main connex component
  std::vector<algo::GeneratorDefinition> generators_;                                ///< generators found
  std::vector<algo::LoadDefinition> loads_;                                          ///< loads found
  std::vector<algo::StaticVarCompensatorDefinition> staticVarCompensators_;          ///< svarcs definitions
  algo::HVDCLineDefinitions hvdcLineDefinitions_;                                    ///< hvdc definitions
  algo::GeneratorDefinitionAlgorithm::BusGenMap busesRegulatedBySeveralGenerators_;  ///< map of bus ids to a generator that regulates them
  algo::DynamicModelDefinitions dynamicModels_;                                      ///< model definitions
  algo::ShuntCounterDefinitions counters_;                                           ///< shunt counters definitions
  algo::LinesByIdDefinitions linesById_;                                             ///< Lines by ids definition
  boost::optional<algo::ValidContingencies> validContingencies_;                     ///< contingencies accepted for simulation in a Security Analyasis
  std::shared_ptr<algo::AlgorithmsResults> algoResults_;                             ///< reference to algorithms results class

  boost::shared_ptr<job::JobEntry> jobEntry_;                 ///< Dynawo job entry
  std::vector<boost::shared_ptr<job::JobEntry>> jobsEvents_;  ///< Dynawo job entries for contingencies
};
}  // namespace dfl
