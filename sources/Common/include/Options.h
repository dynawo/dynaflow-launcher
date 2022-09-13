//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

/**
 * @file Options.h
 * @brief Options header file
 */

/// @brief Namespace for dynaflow launcher components
namespace dfl {
/// @brief Namespace for common components to dynawo-launcher
namespace common {

/**
 * @brief Manager for input programm options
 *
 * It relies on boost program options to describe all options allowed for the program
 */
class Options {
 public:
  /**
   * @brief Runtime configuration
   *
   * Representation of the options after parsing
   */
  struct RuntimeConfiguration {
    std::string programName;            ///< Name of the program
    std::string networkFilePath;        ///< Network filepath to process
    std::string contingenciesFilePath;  ///< Contingencies filepath for security analysis
    std::string configPath;             ///< Launcher configuration filepath
    std::string dynawoLogLevel;         ///< chosen log level
  };

  /**
   * @brief type of request for dynaflow launcher
   */
  enum class Request {
    ERROR = 0,          ///< request contains an error
    HELP,               ///< help display is requested
    VERSION,            ///< version display is requested
    RUN_SIMULATION_N,   ///< steady state calculation is requested
    RUN_SIMULATION_SA,  ///< security analysis is requested
    RUN_SIMULATION_NSA  ///< steady state calculation and security analysis is requested
  };

 public:
  static const std::vector<std::string> allowedLogLevels_;  ///< allowed values for log levels

 public:
  /**
  * @brief Constructor
  *
  * The constructor will initialize the program options variable to prepare the parsing of the arguments
  */
  Options();

  /**
   * @brief Get the runtime configuration
   *
   * @returns the runtime configuration
   */
  const RuntimeConfiguration& config() const {
    return config_;
  }

  /**
   * @brief Parse arguments given by main program
   *
   * @param argc number of arguments
   * @param argv arguments
   *
   * @returns the request
   */
  Request parse(int argc, char* argv[]);

  /**
   * @brief Retrieves description of options
   *
   * @returns the string representation of the option description
   */
  std::string desc() const;

 private:
  /**
   * @brief Extracts the basename of a file path
   *
   * @param filepath the filepath to process
   *
   * @returns the basename of the file
   */
  static std::string basename(const std::string& filepath);

 private:
  static const std::string defaultLogLevel_;  ///< Default log level

 private:
  boost::program_options::options_description desc_;  ///< options description
  RuntimeConfiguration config_;                       ///< current runtime configuration
};

}  // namespace common
}  // namespace dfl

/**
 * @brief Flow operator to stream options description
 *
 * @param os the stream to use
 * @param opt the options to stream
 *
 * @returns os
 */
std::ostream& operator<<(std::ostream& os, const dfl::common::Options& opt);
