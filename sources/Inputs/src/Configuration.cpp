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
#include "Message.hpp"

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
    LOG(warn) << MESS(BadActivePowerCompensation, apcString) << LOG_ENDL;
  }
}

/**
 * @brief Helper function to update a std::chrono::seconds value
 *
 * @param seconds the value to update
 * @param tree the element of the boost tree
 * @param key the key of the parameter to retrieve
 */
static void
updateSeconds(std::chrono::seconds& seconds, const boost::property_tree::ptree& tree, const std::string& key) {
  unsigned int scount = std::numeric_limits<unsigned int>::max();
  helper::updateValue(scount, tree, key);
  if (scount != std::numeric_limits<unsigned int>::max()) {
    seconds = std::chrono::seconds(scount);
  }
}
}  // namespace helper

Configuration::Configuration(const boost::filesystem::path& filepath) {
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

    helper::updateValue(useInfiniteReactiveLimits_, config, "InfiniteReactiveLimits");
    helper::updateValue(isPSTRegulationOn_, config, "PSTRegulationOn");
    helper::updateValue(isSVCRegulationOn_, config, "SVCRegulationOn");
    helper::updateValue(isShuntRegulationOn_, config, "ShuntRegulationOn");
    helper::updateValue(isAutomaticSlackBusOn_, config, "AutomaticSlackBusOn");
    helper::updateValue(outputDir_, config, "OutputDir");
    helper::updateValue(dsoVoltageLevel_, config, "DsoVoltageLevel");
    helper::updateValue(settingFilePath_, config, "SettingPath");
    helper::updateValue(assemblingFilePath_, config, "AssemblyPath");
    helper::updateSeconds(startTime_, config, "StartTime");
    helper::updateSeconds(stopTime_, config, "StopTime");
    helper::updateSeconds(timeOfEvent_, config, "sa.TimeOfEvent");
    helper::updateActivePowerCompensationValue(activePowerCompensation_, config);
  } catch (std::exception& e) {
    LOG(error) << "Error while reading configuration file: " << e.what() << LOG_ENDL;
    std::exit(EXIT_FAILURE);
  }
}
}  // namespace inputs
}  // namespace dfl
