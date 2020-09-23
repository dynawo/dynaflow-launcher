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

#include <algorithm>
#include <boost/filesystem.hpp>

const std::vector<std::string> Options::allowedLogLevels_{"ERROR", "WARN", "INFO", "DEBUG"};

const std::string Options::defaultLogLevel_ = "INFO";

namespace po = boost::program_options;

/**
 * @brief Structure encapsulate a log level value
 *
 * Encapsulate that way is required in order to overload boost program option validate
 */
struct ParsedLogLevel {
  explicit ParsedLogLevel(const std::string& lvl) : logLevelDefinition{lvl} {}

  std::string logLevelDefinition;
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

std::string
Options::make_usage_string(const std::string& program_name, const po::options_description& desc, const po::positional_options_description& p) {
  std::vector<std::string> parts;
  parts.push_back("Usage:\n");
  parts.push_back(program_name);
  for (size_t i = 0; i < p.max_total_count(); ++i) {
    parts.push_back("<" + p.name_for_position(i) + ">");
  }

  if (desc.options().size() > 0) {
    parts.push_back("\n[options]");
  }
  std::ostringstream oss;
  std::copy(parts.begin(), parts.end(), std::ostream_iterator<std::string>(oss, " "));
  oss << std::endl << desc;
  return oss.str();
}

Options::Options() : allOptions_{}, desc_{}, positional_{}, config_{"", "", defaultLogLevel_} {
  po::options_description hidden;

  desc_.add_options()("help,h", "Display help message")("log-level", po::value<ParsedLogLevel>(),
                                                        "Dynawo logger level (allowed values are ERROR, WARN, INFO, DEBUG): default is info");
  hidden.add_options()("iidmFilepath", po::value<std::string>(&config_.iidmPath)->required(), "IIDM file path to process");

  allOptions_.add(desc_);
  allOptions_.add(hidden);

  // Order of the program's arguments is the order of declaration
  positional_.add("iidmFilepath", 1);
}

bool
Options::parse(int argc, char* argv[]) {
  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(allOptions_).positional(positional_).run(), vm);

    config_.programName = argv[0];
    if (vm.count("help") > 0) {
      return false;
    }

    po::notify(vm);

    // These are not binded automatically
    if (vm.count("log-level") > 0) {
      config_.dynawoLogLevel = vm["log-level"].as<ParsedLogLevel>().logLevelDefinition;
    }
    return true;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
}

std::string
Options::desc() const {
  return make_usage_string(basename(config_.programName), desc_, positional_);
}

std::ostream&
operator<<(std::ostream& os, const Options& opt) {
  os << opt.desc();
  return os;
}
