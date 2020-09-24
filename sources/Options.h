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
#include <vector>

/**
 * @file Options.h
 * @brief Options header file
 */

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
   * representation after parsing
   */
  struct RuntimeConfiguration {
    std::string iidmPath;        ///< IIDM filepath ot process
    std::string configPath;      ///< Launcher configuration filepath
    std::string dynawoLogLevel;  ///< chosen log level
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
   * @brief Parse arguments given by main programm
   *
   * @param argc number of arguments
   * @param argv arguments
   *
   * @returns false if display description/help message is required, true if not
   */
  bool parse(int argc, char* argv[]);

  /**
   * @brief Retrieves description of options
   *
   * @returns the string representation of the option description
   */
  std::string desc() const;

 private:
  static const char* defaultLogLevel_;  ///< Default log level

 private:
  friend std::ostream& operator<<(std::ostream& os, const Options& opt);

 private:
  boost::program_options::options_description desc_;  ///< options description
  RuntimeConfiguration config_;                       ///< current runtime configuration
};

/**
 * @brief Flow operator to stream options description
 *
 * @param os the stream to use
 * @param opt the options to stream
 *
 * @returns os
 */
std::ostream& operator<<(std::ostream& os, const Options& opt);
