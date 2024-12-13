//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

/**
 * @file Options.h
 * @brief Options header file
 */

namespace zip {
class ZipFile;
}

/// @brief Namespace for dynaflow launcher components
namespace dfl {
/// @brief Namespace for common components to dynawo-launcher
namespace common {

namespace po = boost::program_options;

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
    std::string zipArchivePath;         ///< zip archive path to unzip to get input files
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
   * @brief Check whether all required files are present in the specified archive
   *
   * This method checks if the zip archive contains all files required for execution, such as the network file,
   * configuration file, and optionally the contingencies file.
   *
   * @param vm A variables map containing the input arguments passed to Dynaflow-launcher. It is used to determine if the optional contingencies file is required.
   * @param archive A shared pointer to the zip archive to be checked.
   *
   * @returns true if one or more required files are missing, false otherwise
   */
  bool areRequiredFilesMissing(const po::variables_map& vm, const boost::shared_ptr<zip::ZipFile>& archive) const;

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
