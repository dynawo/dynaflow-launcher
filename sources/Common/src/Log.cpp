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

namespace dfl {
namespace common {

const char* Log::dynaflowLauncherLogTag = "DYNAFLOW_LAUNCHER";

void
Log::init(const common::Options& options) {
  using DYN::Trace;
  auto& config = options.config();

  std::vector<Trace::TraceAppender> appenders;
  Trace::TraceAppender appender;

  appender.setFilePath(config.programName + ".log");
  appender.setLvlFilter(Trace::severityLevelFromString(config.dynawoLogLevel));
  appender.setTag(dynaflowLauncherLogTag);
  appender.setShowLevelTag(true);
  appender.setSeparator(" ");
  appender.setShowTimeStamp(true);
  appender.setTimeStampFormat("%Y-%m-%d %H:%M:%S");

  appenders.push_back(appender);

  appender.setFilePath("dynawo.log");
  appender.setLvlFilter(Trace::severityLevelFromString(config.dynawoLogLevel));
  appender.setTag("");
  appender.setShowLevelTag(true);
  appender.setSeparator(" ");
  appender.setShowTimeStamp(true);
  appender.setTimeStampFormat("%Y-%m-%d %H:%M:%S");

  appenders.push_back(appender);

  Trace::addAppenders(appenders);
}

}  // namespace common
}  // namespace dfl
