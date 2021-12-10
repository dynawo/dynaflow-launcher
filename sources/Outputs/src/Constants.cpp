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

RefShuntsByIdMap
computeFilteredShuntsByIds(const algo::ShuntDefinitions& shuntDefinitions) {
  RefShuntsByIdMap ret;

  for (const auto& shuntDefPair : shuntDefinitions.shunts) {
    const auto& shuntDef = shuntDefPair.second;
    if (shuntDef.dynamicModelAssociated) {
      continue;
    }
    for (const auto& shunt : shuntDef.shunts) {
      if (!shunt.voltageRegulationOn) {
        continue;
      }
      // Only the shunts with no dynamic model associated and voltage regulation on are added
      ret[shunt.busId].push_back(std::ref(shunt));
    }
  }

  return ret;
}

size_t
ShuntRefHash::operator()(const ConstShuntRef& shunt) const noexcept {
  return std::hash<inputs::Shunt::ShuntId>{}(shunt.get().id);
}

bool
ShuntRefEqual::operator()(const ConstShuntRef& lhs, const ConstShuntRef& rhs) const {
  return lhs.get() == rhs.get();
}

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
