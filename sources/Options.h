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
    std::string programName;     ///< Name of the programm launched
    std::string iidmPath;        ///< IIDM filepath ot process
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
  /**
   * @brief Extracts the basename of a file path
   *
   * @param filepath the filepath to process
   *
   * @returns the basename of the file
   */
  static std::string basename(const std::string& filepath);

  /**
   * @brief Format description from arguments
   *
   * @param program_name the name of the current running programm
   * @param desc the internal description of the options ot use
   * @param p the positionnal description of the options to use
   *
   * @returns the string representation of the options described by arguments
   */
  static std::string make_usage_string(const std::string& program_name, const boost::program_options::options_description& desc,
                                       const boost::program_options::positional_options_description& p);

 private:
  static const std::string defaultLogLevel_;  ///< Default log level

 private:
  friend std::ostream& operator<<(std::ostream& os, const Options& opt);

 private:
  boost::program_options::options_description allOptions_;             ///< all parsed options
  boost::program_options::options_description desc_;                   ///< options relative to description
  boost::program_options::positional_options_description positional_;  ///< positional options
  RuntimeConfiguration config_;                                        ///< current runtime configuration
};

/**
 * @brief Flow operator to stream options description
 */
std::ostream& operator<<(std::ostream& os, const Options& opt);
