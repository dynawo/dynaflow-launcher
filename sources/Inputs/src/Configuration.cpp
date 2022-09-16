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
 */
template<class T>
static void
updateValue(T& value, const boost::property_tree::ptree& tree, const std::string& key) {
  auto value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    value = value_opt->get_value<T>();
  }
}

/**
 * @brief Helper function to update an internal parameter of type boost::optional<double>
 *
 * @param value the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 */
template<>
void
updateValue(boost::optional<double>& value, const boost::property_tree::ptree& tree, const std::string& key) {
  auto value_opt = tree.get_child_optional(key);
  if (value_opt.is_initialized()) {
    value = value_opt->get_value<double>();
  }
}

/**
 * @brief Helper function to update the active power compensation parameter, if the value of ActivePowerCompensation
 * is not one of the three available, then the function does not update the value of the active power compensation parameter.
 *
 * @param activePowerCompensation the value to update
 * @param tree the element of the boost tree
 */
static void
updateActivePowerCompensationValue(Configuration::ActivePowerCompensation& activePowerCompensation, const boost::property_tree::ptree& tree) {
  std::map<std::string, Configuration::ActivePowerCompensation> enumResolver = {{"P", Configuration::ActivePowerCompensation::P},
                                                                                {"targetP", Configuration::ActivePowerCompensation::TARGET_P},
                                                                                {"PMax", Configuration::ActivePowerCompensation::PMAX}};
  std::string apcString;
  helper::updateValue(apcString, tree, "ActivePowerCompensation");
  auto it = enumResolver.find(apcString);
  if (it != enumResolver.end()) {
    activePowerCompensation = it->second;
  } else if (!apcString.empty()) {
    LOG(warn, BadActivePowerCompensation, apcString);
  }
}

}  // namespace helper

Configuration::Configuration(const boost::filesystem::path& filepath, dfl::inputs::Configuration::SimulationKind simulationKind) {
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

    updateStartingPointMode(config);
    updateChosenOutput(config, simulationKind);
    helper::updateValue(useInfiniteReactiveLimits_, config, "InfiniteReactiveLimits");
    helper::updateValue(isSVCRegulationOn_, config, "SVCRegulationOn");
    helper::updateValue(isShuntRegulationOn_, config, "ShuntRegulationOn");
    helper::updateValue(isAutomaticSlackBusOn_, config, "AutomaticSlackBusOn");
    helper::updateValue(outputDir_, config, "OutputDir");
    helper::updateValue(dsoVoltageLevel_, config, "DsoVoltageLevel");
    helper::updateValue(settingFilePath_, config, "SettingPath");
    helper::updateValue(assemblingFilePath_, config, "AssemblingPath");
    helper::updateValue(precision_, config, "Precision");
    helper::updateValue(startTime_, config, "StartTime");
    helper::updateValue(stopTime_, config, "StopTime");
    helper::updateValue(timeOfEvent_, config, "sa.TimeOfEvent");
    helper::updateValue(timeStep_, config, "TimeStep");
    helper::updateValue(tfoVoltageLevel_, config, "TfoVoltageLevel");
    helper::updateActivePowerCompensationValue(activePowerCompensation_, config);
  } catch (std::exception& e) {
    throw Error(ErrorConfigFileRead, e.what());
  }
}

void Configuration::updateStartingPointMode(const boost::property_tree::ptree& tree) {
  const boost::optional<const boost::property_tree::ptree &>& optionalStartingPointMode = tree.get_child_optional("StartingPointMode");
  if (optionalStartingPointMode.is_initialized()) {
    const std::string startingPointMode = optionalStartingPointMode->get_value<std::string>();
    if (startingPointMode == "WARM") {
      startingPointMode_ = dfl::inputs::Configuration::StartingPointMode::WARM;
    } else if (startingPointMode == "FLAT") {
      startingPointMode_ = dfl::inputs::Configuration::StartingPointMode::FLAT;
    } else {
      throw Error(StartingPointModeDoesntExist, startingPointMode);
    }
  }
}

void
Configuration::updateChosenOutput(const boost::property_tree::ptree& tree,
#if _DEBUG_
                                  dfl::inputs::Configuration::SimulationKind
#else
                                  dfl::inputs::Configuration::SimulationKind simulationKind
#endif
) {
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
  }
#endif
  const boost::optional<const boost::property_tree::ptree&>& optionalChosenOutputs = tree.get_child_optional("ChosenOutputs");
  if (optionalChosenOutputs) {
    for (auto& chosenOutputElement : *optionalChosenOutputs) {
      const std::string chosenOutputName = chosenOutputElement.second.get_value<std::string>();
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
