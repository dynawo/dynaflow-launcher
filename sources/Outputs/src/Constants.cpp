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
diagramFilename(const dfl::algo::GeneratorDefinition& generator) {
  // Remove '/' and '\' characters from id and replace it with '%'
  auto id = generator.id;
  std::replace(id.begin(), id.end(), '/', '%');
  std::replace(id.begin(), id.end(), '\\', '%');
  return id + "_Diagram.txt";
}

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
