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
 * @file  Configuration.cpp
 *
 * @brief Configuration implementation file
 *
 */

#include "Configuration.h"

#include "Log.h"

#include <DYNFileSystemUtils.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <limits>

#pragma GCC diagnostic warning "-Wvarargs"

namespace dfl {
namespace inputs {

namespace helper {
/**
 * @brief Helper function to update an internal parameter
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param saMode true if simulation is in SA, false otherwise
 */
template<class T>
static void
updateValue(T& value, const boost::property_tree::ptree& tree, const std::string& key, const bool saMode) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized())
    value = value_opt->get_value<T>();
}

/**
 * @brief Helper function to update an internal parameter of type boost::optional<double>
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param saMode true if simulation is in SA, false otherwise
 */
template<>
void
updateValue(boost::optional<double>& value, const boost::property_tree::ptree& tree, const std::string& key, const bool saMode) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized())
    value = value_opt->get_value<double>();
}

/**
 * @brief Helper function to update an internal parameter of type boost::filesystem::path
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param configDirectoryPath the absolute path of the directory containing the configuration file
 * @param saMode true if simulation is in SA, false otherwise
 */
void
updatePathValue(boost::filesystem::path& value, const boost::property_tree::ptree& tree, const std::string& key, const std::string& configDirectoryPath,
                const bool saMode) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    value = value_opt->get_value<boost::filesystem::path>();
    value = createAbsolutePath(value.generic_string(), configDirectoryPath);
  }
}

/**
 * @brief Helper function to update an internal parameter of type boost::optional<bool>
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param saMode true if simulation is in SA, false otherwise
 */
template<>
void
updateValue(bool& value, const boost::property_tree::ptree& tree, const std::string& key, const bool saMode) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    std::string value_str = value_opt->get_value<std::string>();
    std::transform(value_str.begin(), value_str.end(), value_str.begin(), ::tolower);
    if (value_str == "false") {
      value = false;
    } else if (value_str == "true") {
      value = true;
    } else {
      throw std::logic_error("Wrong boolean value.");
    }
  }
}

/**
 * @brief Helper function to update the active power compensation parameter, if the value of ActivePowerCompensation
 * is not one of the three available, then the function does not update the value of the active power compensation parameter.
 *
 * @param activePowerCompensation the value to update
 * @param tree the element of the boost tree
 * @param saMode true if simulation is in SA, false otherwise
 */
static void
updateActivePowerCompensationValue(Configuration::ActivePowerCompensation& activePowerCompensation, const boost::property_tree::ptree& tree,
                                   const bool saMode) {
  std::map<std::string, Configuration::ActivePowerCompensation> enumResolver = {{"P", Configuration::ActivePowerCompensation::P},
                                                                                {"targetP", Configuration::ActivePowerCompensation::TARGET_P},
                                                                                {"PMax", Configuration::ActivePowerCompensation::PMAX}};
  std::string apcString;
  helper::updateValue(apcString, tree, "ActivePowerCompensation", saMode);
  auto it = enumResolver.find(apcString);
  if (it != enumResolver.end()) {
    activePowerCompensation = it->second;
  } else if (!apcString.empty()) {
    LOG(warn, BadActivePowerCompensation, apcString);
  }
}

}  // namespace helper

Configuration::Configuration(const boost::filesystem::path& filepath, dfl::inputs::Configuration::SimulationKind simulationKind,
                             dfl::common::Options::Request request) {
  try {
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(filepath.generic_string(), tree);

    /**
     * We assume the following format for the configuration in JSON format
     * "dfl-config":  {
     *  "name": "value"
     *  ...
     *  "sa": {
     *   "name": "value"
     *   ...
     *  }
     * }
     *
     * where name is the name of the parameter and value its value. If not present, we use its hard-coded default value
     *
     * Parameters that are used only in security analysis are expected inside the block named "sa"
     */

    auto config = tree.get_child("dfl-config");

    std::string prefixConfigFile = absolute(remove_file_name(filepath.generic_string()));

    bool saMode = false;
    if (request == dfl::common::Options::Request::RUN_SIMULATION_SA)
      saMode = true;
    updateStartingPointMode(config, saMode);
    updateChosenOutput(config, simulationKind, saMode);
    helper::updateValue(useInfiniteReactiveLimits_, config, "InfiniteReactiveLimits", saMode);
    helper::updateValue(isSVarCRegulationOn_, config, "SVCRegulationOn", saMode);
    helper::updateValue(isShuntRegulationOn_, config, "ShuntRegulationOn", saMode);
    helper::updateValue(isAutomaticSlackBusOn_, config, "AutomaticSlackBusOn", saMode);
    helper::updatePathValue(outputDir_, config, "OutputDir", prefixConfigFile, saMode);
    helper::updateValue(dsoVoltageLevel_, config, "DsoVoltageLevel", saMode);
    helper::updatePathValue(settingFilePath_, config, "SettingPath", prefixConfigFile, saMode);
    helper::updatePathValue(assemblingFilePath_, config, "AssemblingPath", prefixConfigFile, saMode);
    helper::updateValue(precision_, config, "Precision", saMode);
    helper::updateValue(startTime_, config, "StartTime", saMode);
    helper::updateValue(stopTime_, config, "StopTime", saMode);
    helper::updateValue(timeStep_, config, "TimeStep", saMode);
    helper::updateValue(tfoVoltageLevel_, config, "TfoVoltageLevel", saMode);
    helper::updateActivePowerCompensationValue(activePowerCompensation_, config, saMode);
    helper::updateValue(timeOfEvent_, config, "TimeOfEvent", true);
  } catch (std::exception& e) {
    throw Error(ErrorConfigFileRead, e.what());
  }

  if (startingPointMode_ == Configuration::StartingPointMode::FLAT && activePowerCompensation_ == Configuration::ActivePowerCompensation::P) {
    throw Error(InvalidActivePowerCompensation, filepath.generic_string());
  }

  if (startingPointMode_ == Configuration::StartingPointMode::FLAT && request == dfl::common::Options::Request::RUN_SIMULATION_SA) {
    throw Error(NoFlatStartingPointModeInSA);
  }
}

void
Configuration::updateStartingPointMode(const boost::property_tree::ptree& tree, const bool saMode) {
  const std::string key = "StartingPointMode";
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  boost::optional<const boost::property_tree::ptree&> optionalStartingPointMode = tree.get_child_optional(jsonKey);
  if (!optionalStartingPointMode.is_initialized())
    optionalStartingPointMode = tree.get_child_optional(key);
  if (optionalStartingPointMode.is_initialized()) {
    std::string startingPointMode = optionalStartingPointMode->get_value<std::string>();
    std::transform(startingPointMode.begin(), startingPointMode.end(), startingPointMode.begin(), ::tolower);
    if (startingPointMode == "warm") {
      startingPointMode_ = dfl::inputs::Configuration::StartingPointMode::WARM;
    } else if (startingPointMode == "flat") {
      startingPointMode_ = dfl::inputs::Configuration::StartingPointMode::FLAT;
    } else {
      throw Error(StartingPointModeDoesntExist, startingPointMode);
    }
  }
}

void
Configuration::updateChosenOutput(const boost::property_tree::ptree& tree,
#if _DEBUG_
                                  dfl::inputs::Configuration::SimulationKind,
#else
                                  dfl::inputs::Configuration::SimulationKind simulationKind,
#endif
                                  const bool saMode) {
#if _DEBUG_
  chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE);
  chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS);
  chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE);
  chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ);
#else
  switch (simulationKind) {
  case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION:
    chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE);
    break;
  case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
    chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS);
    chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ);
    break;
  default:
    // Code should never be reached.
    throw std::logic_error("Cannot determine simulation kind.");
    break;
  }
#endif
  const std::string key = "ChosenOutputs";
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  boost::optional<const boost::property_tree::ptree&> optionalChosenOutputs = tree.get_child_optional(jsonKey);
  if (!optionalChosenOutputs.is_initialized())
    optionalChosenOutputs = tree.get_child_optional(key);
  if (optionalChosenOutputs.is_initialized()) {
    for (auto& chosenOutputElement : optionalChosenOutputs.get()) {
      std::string chosenOutputName = chosenOutputElement.second.get_value<std::string>();
      std::transform(chosenOutputName.begin(), chosenOutputName.end(), chosenOutputName.begin(), ::toupper);
      if (chosenOutputName == "STEADYSTATE") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE);
      } else if (chosenOutputName == "CONSTRAINTS") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS);
      } else if (chosenOutputName == "TIMELINE") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE);
      } else if (chosenOutputName == "LOSTEQ") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ);
      } else {
        throw Error(ChosenOutputDoesntExist, chosenOutputName);
      }
    }
  }
}

}  // namespace inputs
}  // namespace dfl
