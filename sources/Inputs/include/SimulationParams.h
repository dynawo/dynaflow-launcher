//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  SimulationParams.h
 *
 * @brief Manager of simulation parameters
 *
 */

#pragma once

#include "Configuration.h"
#include "Options.h"

#include <boost/filesystem.hpp>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace dfl {
namespace inputs {

/**
 * @brief Simulation parameters
 *
 * This structure aims at grouping in one place options needed by the simulation
 * to run.
 */
struct SimulationParams {
  std::string locale;                                               ///< locale
  boost::filesystem::path networkFilePath;                          ///< path to network file
  boost::filesystem::path resourcesDirPath;                         ///< path to resource directory
  dfl::inputs::Configuration::SimulationKind simulationKind;        ///< type of simulation
  std::chrono::time_point<std::chrono::steady_clock> timeStart;     ///< time of the start of the simulation
  dfl::common::Options::RuntimeConfiguration const* runtimeConfig;  ///< runtime configuration of the program
};

}  // namespace inputs
}  // namespace dfl
