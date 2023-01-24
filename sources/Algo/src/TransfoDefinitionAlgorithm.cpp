//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "TransfoDefinitionAlgorithm.h"

namespace dfl {
namespace algo {

TransformersByIdAlgorithm::TransformersByIdAlgorithm(TransformersByIdDefinitions& tfosByIdDefinition) : tfosByIdDefinition_(tfosByIdDefinition) {}

void
TransformersByIdAlgorithm::operator()(const NodePtr& node) {
  const auto& tfos = node->tfos;
  for (const auto& tfo_ptr : tfos) {
    auto tfo = tfo_ptr.lock();
    if (tfosByIdDefinition_.tfosMap.count(tfo->id) > 0) {
      continue;
    }

    tfosByIdDefinition_.tfosMap.insert({tfo->id, *tfo});
  }
}

}  // namespace algo
}  // namespace dfl
