//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ShuntDefinitionAlgorithm.h"

#include <DYNCommon.h>

namespace dfl {
namespace algo {

ShuntCounterAlgorithm::ShuntCounterAlgorithm(ShuntCounterDefinitions& defs) : shuntCounterDefs_(defs) {}

void
ShuntCounterAlgorithm::operator()(const NodePtr& node) {
  auto vl = node->voltageLevel.lock();
  shuntCounterDefs_.nbShunts[vl->id] += static_cast<unsigned int>(node->shunts.size());
}

}  // namespace algo
}  // namespace dfl
