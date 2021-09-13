//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  Constants.cpp
 *
 * @brief Dynaflow launcher common to all writers impelmentation file
 *
 */

#include "Constants.h"

#include <algorithm>

namespace dfl {
namespace outputs {
namespace constants {

std::string
diagramFilename(const std::string& id) {
  // Remove '/' and '\' characters from id and replace it with '_' to avoid mistakes when parsing the path
  auto idCpy = id;
  std::replace(idCpy.begin(), idCpy.end(), '/', '_');
  std::replace(idCpy.begin(), idCpy.end(), '\\', '_');
  return idCpy + "_Diagram.txt";
}

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
