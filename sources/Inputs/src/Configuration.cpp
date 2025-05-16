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
 * @param parameterValueModified a parameter key is added in this if the value
 * was redefined in the configuration file
 */
template <class T>
static void updateValue(T &value, const boost::property_tree::ptree &tree, const std::string &key, const bool saMode,
                        std::unordered_set<std::string> &parameterValueModified) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    parameterValueModified.insert(key);
    value = value_opt->get_value<T>();
  }
}

/**
 * @brief Helper function to update an internal parameter of type
 * boost::optional<double>
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param saMode true if simulation is in SA, false otherwise
 * @param parameterValueModified a parameter key is added in this if the value
 */
template <>
void updateValue(boost::optional<double> &value, const boost::property_tree::ptree &tree, const std::string &key, const bool saMode,
                 std::unordered_set<std::string> &parameterValueModified) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    parameterValueModified.insert(key);
    value = value_opt->get_value<double>();
  }
}

/**
 * @brief Helper function to update an internal parameter of type
 * boost::filesystem::path
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param configDirectoryPath the absolute path of the directory containing the
 * configuration file
 * @param saMode true if simulation is in SA, false otherwise
 */
void updatePathValue(boost::filesystem::path &value, const boost::property_tree::ptree &tree, const std::string &key, const std::string &configDirectoryPath,
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
 * @brief Helper function to update an internal parameter of type
 * boost::optional<bool>
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 * @param saMode true if simulation is in SA, false otherwise
 * @param parameterValueModified a parameter key is added in this if the value
 */
template <>
void updateValue(bool &value, const boost::property_tree::ptree &tree, const std::string &key, const bool saMode,
                 std::unordered_set<std::string> &parameterValueModified) {
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  auto value_opt = tree.get_child_optional(jsonKey);
  if (!value_opt.is_initialized())
    value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    parameterValueModified.insert(key);
    std::string value_str = value_opt->get_value<std::string>();
    std::transform(value_str.begin(), value_str.end(), value_str.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
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
 * @brief Helper function to update the active power compensation parameter, if
 * the value of ActivePowerCompensation is not one of the three available, then
 * the function does not update the value of the active power compensation
 * parameter.
 *
 * @param activePowerCompensation the value to update
 * @param tree the element of the boost tree
 * @param saMode true if simulation is in SA, false otherwise
 * @param parameterValueModified a parameter key is added in this if the value
 */
static void updateActivePowerCompensationValue(Configuration::ActivePowerCompensation &activePowerCompensation, const boost::property_tree::ptree &tree,
                                               const bool saMode, std::unordered_set<std::string> &parameterValueModified) {
  std::map<std::string, Configuration::ActivePowerCompensation> enumResolver = {{"P", Configuration::ActivePowerCompensation::P},
                                                                                {"targetP", Configuration::ActivePowerCompensation::TARGET_P},
                                                                                {"PMax", Configuration::ActivePowerCompensation::PMAX}};
  std::string apcString;
  helper::updateValue(apcString, tree, "ActivePowerCompensation", saMode, parameterValueModified);
  auto it = enumResolver.find(apcString);
  if (it != enumResolver.end()) {
    activePowerCompensation = it->second;
  } else if (!apcString.empty()) {
    LOG(warn, BadActivePowerCompensation, apcString);
  }
}

}  // namespace helper

Configuration::Configuration(const boost::filesystem::path &filepath, SimulationKind simulationKind) : filepath_(filepath), simulationKind_(simulationKind) {
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
     * where name is the name of the parameter and value its value. If not
     * present, we use its hard-coded default value
     *
     * Parameters that are used only in security analysis are expected inside
     * the block named "sa"
     */

    auto config = tree.get_child("dfl-config");

    std::string prefixConfigFile = absolute(remove_file_name(filepath_.generic_string()));

    bool saMode = false;
    if (simulationKind_ == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS)
      saMode = true;
    updateStartingPointMode(config, saMode);
    updateChosenOutput(config, simulationKind_, saMode);
    helper::updateValue(useInfiniteReactiveLimits_, config, "InfiniteReactiveLimits", saMode, parameterValueModified_);
    helper::updateValue(isSVarCRegulationOn_, config, "SVCRegulationOn", saMode, parameterValueModified_);
    helper::updateValue(isShuntRegulationOn_, config, "ShuntRegulationOn", saMode, parameterValueModified_);
    helper::updateValue(isAutomaticSlackBusOn_, config, "AutomaticSlackBusOn", saMode, parameterValueModified_);
    helper::updatePathValue(outputDir_, config, "OutputDir", prefixConfigFile, false);  // Not possible to override outputDir in SA
    helper::updateValue(dsoVoltageLevel_, config, "DsoVoltageLevel", saMode, parameterValueModified_);
    helper::updatePathValue(settingFilePath_, config, "SettingPath", prefixConfigFile, saMode);
    helper::updatePathValue(assemblingFilePath_, config, "AssemblingPath", prefixConfigFile, saMode);
    helper::updateValue(precision_, config, "Precision", saMode, parameterValueModified_);
    helper::updateValue(startTime_, config, "StartTime", saMode, parameterValueModified_);
    helper::updateValue(stopTime_, config, "StopTime", saMode, parameterValueModified_);
    helper::updateValue(timeStep_, config, "TimeStep", saMode, parameterValueModified_);
    helper::updateValue(minTimeStep_, config, "MinTimeStep", saMode, parameterValueModified_);
    helper::updateValue(tfoVoltageLevel_, config, "TfoVoltageLevel", saMode, parameterValueModified_);
    helper::updateActivePowerCompensationValue(activePowerCompensation_, config, saMode, parameterValueModified_);
    helper::updatePathValue(startingDumpFilePath_, config, "StartingDumpFile", prefixConfigFile, true);
    if (simulationKind_ == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS) {
      helper::updateValue(timeOfEvent_, config, "TimeOfEvent", true, parameterValueModified_);
    }
  } catch (std::exception &e) {
    throw Error(ErrorConfigFileRead, e.what());
  }
}

void Configuration::sanityCheck() const {
  if (simulationKind_ == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS && !startingDumpFilePath_.empty() && !exists(startingDumpFilePath_)) {
    throw Error(StartingDumpFileNotFound, startingDumpFilePath_.generic_string());
  }

  if (!settingFilePath_.empty() && assemblingFilePath_.empty()) {
    throw Error(SettingFileWithoutAssemblingFile, filepath_.generic_string());
  }

  if (!assemblingFilePath_.empty() && settingFilePath_.empty()) {
    throw Error(AssemblingFileWithoutSettingFile, filepath_.generic_string());
  }

  if (startingPointMode_ == Configuration::StartingPointMode::FLAT && activePowerCompensation_ == Configuration::ActivePowerCompensation::P) {
    throw Error(InvalidActivePowerCompensation, filepath_.generic_string());
  }
}

void Configuration::updateStartingPointMode(const boost::property_tree::ptree &tree, const bool saMode) {
  const std::string key = "StartingPointMode";
  std::string jsonKey = key;
  if (saMode)
    jsonKey = "sa." + key;
  boost::optional<const boost::property_tree::ptree &> optionalStartingPointMode = tree.get_child_optional(jsonKey);
  if (!optionalStartingPointMode.is_initialized())
    optionalStartingPointMode = tree.get_child_optional(key);
  if (optionalStartingPointMode.is_initialized()) {
    std::string startingPointMode = optionalStartingPointMode->get_value<std::string>();
    std::transform(startingPointMode.begin(), startingPointMode.end(), startingPointMode.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (startingPointMode == "warm") {
      startingPointMode_ = dfl::inputs::Configuration::StartingPointMode::WARM;
    } else if (startingPointMode == "flat") {
      startingPointMode_ = dfl::inputs::Configuration::StartingPointMode::FLAT;
    } else {
      throw Error(StartingPointModeDoesntExist, startingPointMode);
    }
  }
}

void Configuration::updateChosenOutput(const boost::property_tree::ptree &tree,
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
  boost::optional<const boost::property_tree::ptree &> optionalChosenOutputs = tree.get_child_optional(jsonKey);
  if (!optionalChosenOutputs.is_initialized())
    optionalChosenOutputs = tree.get_child_optional(key);
  if (optionalChosenOutputs.is_initialized()) {
    for (auto &chosenOutputElement : optionalChosenOutputs.get()) {
      std::string chosenOutputName = chosenOutputElement.second.get_value<std::string>();
      std::transform(chosenOutputName.begin(), chosenOutputName.end(), chosenOutputName.begin(),
                     [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
      if (chosenOutputName == "STEADYSTATE") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE);
      } else if (chosenOutputName == "CONSTRAINTS") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS);
      } else if (chosenOutputName == "TIMELINE") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE);
      } else if (chosenOutputName == "LOSTEQ") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ);
      } else if (chosenOutputName == "DUMPSTATE") {
        chosenOutputs_.insert(dfl::inputs::Configuration::ChosenOutputEnum::DUMPSTATE);
      } else {
        throw Error(ChosenOutputDoesntExist, chosenOutputName);
      }
    }
  }
}

}  // namespace inputs
}  // namespace dfl
