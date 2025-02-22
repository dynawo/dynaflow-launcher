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
 * @file Log.cpp
 * @brief Log manager implementation file
 */

#include "Log.h"

#include <boost/filesystem.hpp>

namespace file = boost::filesystem;

namespace dfl {
namespace common {

const std::string &Log::getTag() {
  static const std::string &dynaflowLauncherLogTag = "DYNAFLOW_LAUNCHER";
  return dynaflowLauncherLogTag;
}

void Log::init(const common::Options &options, const std::string &outputDir) {
  using DYN::Trace;
  auto &config = options.config();

  std::vector<Trace::TraceAppender> appenders;
  Trace::TraceAppender appender;
  file::path outputPath(outputDir);
  if (!file::exists(outputPath)) {
    file::create_directories(outputPath);
  }

  appender.setFilePath(outputPath.append(config.programName + ".log").generic_string());
  appender.setLvlFilter(Trace::severityLevelFromString(config.dynawoLogLevel));
  appender.setTag(getTag());
  appender.setShowLevelTag(true);
  appender.setSeparator(" | ");
  appender.setShowTimeStamp(true);
  appender.setTimeStampFormat("%Y-%m-%d %H:%M:%S");
  appender.setPersistent(true);

  appenders.push_back(appender);

  Trace::clearAndAddAppenders(appenders);
}

void Log::addLogFileContentInMapData(const std::string &logFileRelativePath, const std::string &logFileAbsolutePath,
                                     std::unordered_map<std::string, std::string> &mapOutputFilesData) {
  std::ifstream programLogFileStream(logFileAbsolutePath);
  if (programLogFileStream.is_open()) {
    std::ostringstream strStream;
    strStream << programLogFileStream.rdbuf();
    mapOutputFilesData[logFileRelativePath] = strStream.str();
    programLogFileStream.close();
  } else {
    throw Error(FailToOpenLogFile, logFileAbsolutePath);
  }
}

}  // namespace common
}  // namespace dfl
