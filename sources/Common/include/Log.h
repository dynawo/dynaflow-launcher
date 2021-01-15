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
 * @file Log.h
 * @brief Log manager header file
 */

#pragma once

#include "Options.h"

#include <DYNTrace.h>

namespace dfl {
namespace common {

/**
 * @brief Class managing the log for dynaflow-launcher
 */
class Log {
 public:
  static const char* dynaflowLauncherLogTag;  ///< Tag for dynaflow launcher

  /**
   * @brief Initialize log from runtime options
   *
   * Relies on dynawo logging system the following way:
   * - one logger with the name of the program, containing logs from dynaflow launcher
   * messages patterns and  level are given by the input
   * @param options user options
   * @param outputDir directory where the log should be written
   */
  static void init(const common::Options& options, const std::string& outputDir);
};

}  // namespace common
}  // namespace dfl

/**
 * @brief Perform a log
 *
 * This performs a log with the tag relative to dynaflow launcher
 *
 * @param level the level of the log: must be "Error", "Warn", "Info" or "Debug"
 */
#define LOG(level) Trace##level(dfl::common::Log::dynaflowLauncherLogTag)

/**
 * @brief Alias of DYN::Trace::endline
 *
 * This aims to hide the long namespace expression to help the developer and to hide the using of dynawo library
 */
#define LOG_ENDL DYN::Trace::endline
