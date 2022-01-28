//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "LineDefinitionAlgorithm.h"

namespace dfl {
namespace algo {

LinesByIdAlgorithm::LinesByIdAlgorithm(LinesByIdDefinitions& linesByIdDefinition) : linesByIdDefinition_(linesByIdDefinition) {}

void
LinesByIdAlgorithm::operator()(const NodePtr& node) {
  const auto& lines = node->lines;
  for (const auto& line_ptr : lines) {
    auto line = line_ptr.lock();
    if (linesByIdDefinition_.linesMap.count(line->id) > 0) {
      continue;
    }

    linesByIdDefinition_.linesMap.insert({line->id, *line});
  }
}

}  // namespace algo
}  // namespace dfl
