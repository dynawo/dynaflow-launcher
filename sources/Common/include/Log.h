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

#include "DFLError_keys.h"
#include "DFLLog_keys.h"
#include "Options.h"

#include <DYNError.h>
#include <DYNMultiProcessingContext.h>
#include <DYNMessage.h>
#include <DYNTrace.h>

namespace dfl {
namespace common {

/**
 * @brief Class managing the log for dynaflow-launcher
 */
class Log {
 public:
  /**
   * @brief Get the tag for dynaflow launcher log
   *
   * @returns the tag for dynaflow launcher log
   */
  static const std::string& getTag();

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

  /**
   * @brief Put the content of a log file in a map
   *
   * This function reads the content of a log file specified by its absolute path and stores it as a string in the
   * provided map, using the relative path as the key.
   *
   * @param logFileRelativePath the relative path to the log file, used as the key in the map
   * @param logFileAbsolutePath the absolute path to the log file, used to locate and read its content
   * @param mapData the map where the log file content is stored, with the relative path as the key
   */
  static void addLogFileContentInMapData(const std::string& logFileRelativePath,
                                          const std::string& logFileAbsolutePath,
                                          std::unordered_map<std::string, std::string>& mapData);
};

}  // namespace common
}  // namespace dfl

/**
 * @brief Perform a log
 *
 * This performs a log with the tag relative to dynaflow launcher
 *
 * @param level the level of the log: must be "error", "warn", "info" or "debug"
 * @param key the log key from the dictionary
 */
#define LOG(level, key, ...)                                                                                                      \
  (DYNAlgorithms::multiprocessing::context().isRootProc() ? DYN::Trace::level(dfl::common::Log::getTag()) : DYN::TraceStream()) \
      << (DYN::Message("DFLLOG", dfl::KeyLog_t::names(dfl::KeyLog_t::key)), ##__VA_ARGS__) << DYN::Trace::endline

/**
 * @brief Macro description to have a shortcut.
 *  Thanks to this macro, user can only call an error with the type and the key to access
 *  the message (+ optional arguments if the message need)
 *  File error localization and line error localization are added
 *
 * @param key  key of the message description
 *
 * @return an Error
 */
#define Error(key, ...)                                                                  \
  DYN::Error(DYN::Error::GENERAL, dfl::KeyError_t::key, std::string(__FILE__), __LINE__, \
             (DYN::Message("DFLERROR", dfl::KeyError_t::names(dfl::KeyError_t::key)), ##__VA_ARGS__))
