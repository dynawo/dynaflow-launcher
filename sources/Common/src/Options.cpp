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
#include "ZipErrorMessage.h"
#include "version.h"

#include <libzip/ZipInputStream.h>
#include <libzip/ZipException.h>
#include <boost/filesystem.hpp>

#include <DYNFileSystemUtils.h>

#include <algorithm>
#include <sstream>


namespace dfl {
namespace common {

const std::vector<std::string> Options::allowedLogLevels_{"ERROR", "WARN", "INFO", "DEBUG"};

#if _DEBUG_
const std::string Options::defaultLogLevel_ = "DEBUG";
#else
const std::string Options::defaultLogLevel_ = "INFO";
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
   * @param lvl the log level string representation
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
 *
 * @param v the ParsedLogLevel corresponding to the user inputs
 * @param values values provided by the user
 */
static void
validate(boost::any& v, const std::vector<std::string>& values, ParsedLogLevel*, int) {
  // Make sure no previous assignment was made.
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
  return path.filename().replace_extension().generic_string();
}

Options::Options() : desc_{}, config_{"", "", "", "", "", defaultLogLevel_} {
  desc_.add_options()
      ("help,h", "Display help message")
      ("version,v", "Display version")
      ("log-level", po::value<ParsedLogLevel>(),
          (std::string("Dynawo logger level (allowed values are ERROR, WARN, INFO, DEBUG): default is ") + defaultLogLevel_).c_str())
      ("network", po::value<std::string>(&config_.networkFilePath)->required(), "Network file path to process (IIDM support only)")
      ("config", po::value<std::string>(&config_.configPath)->required(), "launcher Configuration file to use")
      ("contingencies", po::value<std::string>(&config_.contingenciesFilePath), "Contingencies file path to process (Security Analysis)")
      ("nsa", "Run steady state calculation followed by security analysis. Requires contingencies file to be defined.")
      ("input-archive", po::value<std::string>(&config_.zipArchivePath),
          "Path to a ZIP archive containing input files for '--network', '--config', and '--contingencies'.");
}

Options::Request
Options::parse(int argc, char* argv[]) {
  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc_).run(), vm);

    config_.programName = basename(argv[0]);

    if (vm.count("help") > 0) {
      return Request::HELP;
    }
    if (vm.count("version") > 0) {
      return Request::VERSION;
    }

    po::notify(vm);

    if (vm.count("input-archive") > 0) {
      boost::shared_ptr<zip::ZipFile> archive = zip::ZipInputStream::read(config_.zipArchivePath);
      std::string archiveParentPath = boost::filesystem::path(config_.zipArchivePath).parent_path().string();
      for (std::map<std::string, boost::shared_ptr<zip::ZipEntry> >::const_iterator archiveIt = archive->getEntries().begin();
          archiveIt != archive->getEntries().end(); ++archiveIt) {
        std::string name = archiveIt->first;
        std::string data(archiveIt->second->getData());
        std::ofstream file;
        std::string filepath = createAbsolutePath(name, archiveParentPath);
        file.open(createAbsolutePath(name, archiveParentPath).c_str(), std::ios::binary);
        file << data;
        file.close();
      }
    }

    // These are not binded automatically
    if (vm.count("log-level") > 0) {
      config_.dynawoLogLevel = vm["log-level"].as<ParsedLogLevel>().logLevelDefinition;
    }

    if (vm.count("nsa") > 0) {
      if (vm.count("contingencies") > 0) {
        return Request::RUN_SIMULATION_NSA;
      } else {
        return Request::ERROR;
      }
    } else if (vm.count("contingencies") > 0) {
      return Request::RUN_SIMULATION_SA;
    } else {
      return Request::RUN_SIMULATION_N;
    }
  } catch (const zip::ZipException& e) {
    std::string zipErrorMessage = formatZipErrorMessage(e);
    std::cerr << zipErrorMessage << std::endl;
    return Request::ERROR;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return Request::ERROR;
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
