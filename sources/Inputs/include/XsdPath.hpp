//
// Copyright (c) 2025, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#pragma once

#include "Log.h"
#include <boost/algorithm/string/case_conv.hpp>

namespace file = boost::filesystem;

/**
 * @brief retrieve the xsd path for a given database xml
 * @return the corresponding xsd filepath or an empty path if not found
 */
static file::path getXsdPath(std::string fileName) {
  char * useXsdEnv = getenv("DYNAFLOW_LAUNCHER_USE_XSD_VALIDATION");
  if (useXsdEnv == nullptr)
    return file::path();

  std::string useXsdStr(useXsdEnv);
  boost::to_lower(useXsdStr);

  if (useXsdStr != "true")
    return file::path();

  char * envXsdPathPrefix = getenv("DYNAFLOW_LAUNCHER_XSD");
  file::path xsdPath = (envXsdPathPrefix != nullptr) ? file::path(envXsdPathPrefix) : file::current_path();
  xsdPath.append(fileName);

  if (!file::exists(xsdPath)) {
    LOG(warn, DynModelFileXSDNotFound, xsdPath.generic_string());
    return file::path();
  }

  return xsdPath;
}
