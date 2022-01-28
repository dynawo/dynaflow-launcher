//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ContingencyValidationAlgorithm.h"
#include "Log.h"
#include "Message.hpp"
#include <DYNCommon.h>

namespace dfl {
namespace algo {
void
ContingencyValidationAlgorithm::operator()(const NodePtr& node) {
  using Type = inputs::ContingencyElement::Type;

  for (const auto& line : node->lines) {
    validContingencies_.markElementValid(line.lock()->id, Type::LINE);
  }
  for (const auto& tfoPtr : node->tfos) {
    const auto& tfo = tfoPtr.lock();
    switch (tfo->nodes.size()) {
    case 2:
      validContingencies_.markElementValid(tfo->id, Type::TWO_WINDINGS_TRANSFORMER);
      break;
    case 3:
      validContingencies_.markElementValid(tfo->id, Type::THREE_WINDINGS_TRANSFORMER);
      break;
    }
  }
  for (const auto& converter : node->converters) {
    validContingencies_.markElementValid(converter.lock()->hvdcLine->id, Type::HVDC_LINE);
  }
  for (const auto& load : node->loads) {
    validContingencies_.markElementValid(load.id, Type::LOAD);
  }
  for (const auto& generator : node->generators) {
    validContingencies_.markElementValid(generator.id, Type::GENERATOR);
  }
  for (const auto& shunt : node->shunts) {
    validContingencies_.markElementValid(shunt.id, Type::SHUNT_COMPENSATOR);
  }
  for (const auto& danglingLine : node->danglingLines) {
    validContingencies_.markElementValid(danglingLine.id, Type::DANGLING_LINE);
  }
  for (const auto& svarc : node->svarcs) {
    validContingencies_.markElementValid(svarc.id, Type::STATIC_VAR_COMPENSATOR);
  }
  for (const auto& busBarSection : node->busBarSections) {
    validContingencies_.markElementValid(busBarSection.id, Type::BUSBAR_SECTION);
  }
}

ValidContingencies::ValidContingencies(const std::vector<inputs::Contingency>& contingencies) : contingencies_(std::ref(contingencies)) {
  for (const auto& contingency : contingencies_.get()) {
    for (const auto& element : contingency.elements) {
      elementContingencies_[element.id].push_back(std::ref(contingency));
    }
  }
}

void
ValidContingencies::markElementValid(const ElementId& elementId, inputs::ContingencyElement::Type elementType) {
  const auto& elementContingencies = elementContingencies_.find(elementId);
  if (elementContingencies != elementContingencies_.end()) {
    // For all contingencies where the element is referred ...
    for (const auto& contingencyRef : elementContingencies->second) {
      const auto& contingency = contingencyRef.get();
      // Find the element inside the input contingency ...
      auto contingencyElement = std::find_if(contingency.elements.begin(), contingency.elements.end(),
                                             [&elementId](const inputs::ContingencyElement& contingencyElement) { return contingencyElement.id == elementId; });
      if (contingencyElement != contingency.elements.end()) {
        // And check it has been given with a valid type,
        // according to the reference type found in the network
        if (inputs::ContingencyElement::isCompatible((*contingencyElement).type, elementType)) {
          // If type is compatible, add the element to the list of valid elements found for the contingency
          validatingContingencies_[contingency.id].insert(elementId);
        }
      }
    }
  }
}

void
ValidContingencies::keepContingenciesWithAllElementsValid() {
  // A contingency is valid for simulation if all its elements have been marked as valid
  for (const auto& contingency : contingencies_.get()) {
    auto validatingContingency = validatingContingencies_.find(contingency.id);
    if (validatingContingency == validatingContingencies_.end()) {
      // For this contingency we have not found any valid element
      LOG(warn) << MESS(ContingencyInvalidForSimulationNoValidElements, contingency.id) << LOG_ENDL;
    } else {
      bool valid = true;
      // Iterate over all the elements in the input contingency
      for (const auto& element : contingency.elements) {
        // Check that the element has been marked as valid
        if ((*validatingContingency).second.find(element.id) == (*validatingContingency).second.end()) {
          LOG(warn) << MESS(ContingencyInvalidForSimulation, contingency.id, element.id) << LOG_ENDL;
          valid = false;
        }
      }
      if (valid) {
        validContingencies_.push_back(contingency);
      }
    }
  }
}

}  // namespace algo
}  // namespace dfl
