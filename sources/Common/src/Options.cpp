//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

/**
 * @file Options.cpp
 * @brief Options implementation file
 */

#include "Options.h"

#include "version.h"

#include <algorithm>
#include <boost/filesystem.hpp>
#include <sstream>

namespace dfl {
namespace common {

const std::vector<std::string> Options::allowedLogLevels_{"ERROR", "WARN", "INFO", "DEBUG"};

#if _DEBUG_
const char* Options::defaultLogLevel_ = "DEBUG";
#else
const char* Options::defaultLogLevel_ = "INFO";
#endif

namespace po = boost::program_options;

/**
 * @brief Structure encapsulate a log level value
 *
 * Encapsulate that way is required in order to overload boost program option validate
 */
struct ParsedLogLevel {
  /**
   * @brief Constructor
   *
   * @brief lvl the log level string representation
   */
  explicit ParsedLogLevel(const std::string& lvl) : logLevelDefinition{lvl} {}

  std::string logLevelDefinition;  ///< Log level definition
};

/**
 * @brief overloaded validate function
 *
 * Function to validate the case for log level
 *
 * see boost documentation for program options, "how to" chapter, "custom validator" section
 */
static void
validate(boost::any& v, const std::vector<std::string>& values, ParsedLogLevel*, int) {
  // Make sure no previous assignment to 'a' was made.
  po::validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  const std::string& value = po::validators::get_single_string(values);

  if (std::find(Options::allowedLogLevels_.begin(), Options::allowedLogLevels_.end(), value) == Options::allowedLogLevels_.end()) {
    throw po::validation_error(po::validation_error::invalid_option_value);
  } else {
    v = boost::any(ParsedLogLevel(value));
  }
}

std::string
Options::basename(const std::string& filepath) {
  boost::filesystem::path path(filepath);
  return path.filename().generic_string();
}

Options::Options() : desc_{}, config_{"", "", "", defaultLogLevel_} {
  desc_.add_options()("help,h", "Display help message")("log-level", po::value<ParsedLogLevel>(),
                                                        "Dynawo logger level (allowed values are ERROR, WARN, INFO, DEBUG): default is info")(
      "iidm", po::value<std::string>(&config_.networkFilePath)->required(), "Network file path to process (IIDM support only)")(
      "config", po::value<std::string>(&config_.configPath)->required(), "launcher Configuration file to use")("version,v", "Display version");
}

auto
Options::parse(int argc, char* argv[]) -> std::tuple<bool, Request> {
  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc_).run(), vm);

    config_.programName = basename(argv[0]);
    if (vm.count("help") > 0) {
      return std::forward_as_tuple(true, Request::HELP);
    }
    if (vm.count("version") > 0) {
      return std::forward_as_tuple(true, Request::VERSION);
    }

    po::notify(vm);

    // These are not binded automatically
    if (vm.count("log-level") > 0) {
      config_.dynawoLogLevel = vm["log-level"].as<ParsedLogLevel>().logLevelDefinition;
    }
    return std::forward_as_tuple(true, Request::NORMAL);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return std::forward_as_tuple(false, Request::NORMAL);
  }
}

std::string
Options::desc() const {
  std::stringstream ss;
  ss << config_.programName << " v" << DYNAFLOW_LAUNCHER_VERSION_STRING << std::endl;
  ss << desc_;
  return ss.str();
}

}  // namespace common
}  // namespace dfl

std::ostream&
operator<<(std::ostream& os, const dfl::common::Options& opt) {
  os << opt.desc();
  return os;
}
