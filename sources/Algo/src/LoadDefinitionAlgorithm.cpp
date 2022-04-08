//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "LoadDefinitionAlgorithm.h"

#include <DYNCommon.h>

namespace dfl {
namespace algo {
LoadDefinitionAlgorithm::LoadDefinitionAlgorithm(Loads& loads, double dsoVoltageLevel) : loads_(loads), dsoVoltageLevel_(dsoVoltageLevel) {}

void
LoadDefinitionAlgorithm::operator()(const NodePtr& node, std::shared_ptr<AlgorithmsResults>&) {
  LoadDefinition::ModelType model = LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS;
  bool isUnderDsoVoltage = false;
  if (DYN::doubleNotEquals(node->nominalVoltage, dsoVoltageLevel_) && node->nominalVoltage < dsoVoltageLevel_) {
    isUnderDsoVoltage = true;
  }

  for (const auto& load : node->loads) {
    model = load.isFictitious || isUnderDsoVoltage || load.isNotInjecting ? LoadDefinition::ModelType::NETWORK
                                                                          : LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS;
    loads_.emplace_back(load.id, model, node->id);
  }
}

}  // namespace algo
}  // namespace dfl
