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
 * @file  ParEvent.cpp
 *
 * @brief Dynaflow launcher PAR file writer for events in a contigency implementation file
 *
 */

#include "ParEvent.h"

#include "Contingencies.h"
#include "OutputsConstants.h"
#include "ParCommon.h"

#include <DYNCommon.h>
#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARXmlExporter.h>

namespace dfl {
namespace outputs {

ParEvent::ParEvent(ParEventDefinition &&def) : def_{std::forward<ParEventDefinition>(def)} {}

void ParEvent::write() {
  using Type = dfl::inputs::ContingencyElement::Type;
  parameters::XmlExporter exporter;

  auto parametersSets = parameters::ParametersSetCollectionFactory::newCollection();
  for (const auto &element : def_.contingency.elements) {
    if (isNetwork(element.id)) {
      parametersSets->addParametersSet(buildEventConnectedStatusDisconnection(element.id, def_.timeOfEvent));
      continue;
    }
    switch (element.type) {
    case Type::BRANCH:
    case Type::LINE:
    case Type::TWO_WINDINGS_TRANSFORMER:
      parametersSets->addParametersSet(buildBranchDisconnection(element.id, def_.timeOfEvent));
      break;
    case Type::LOAD:
    case Type::GENERATOR:
    case Type::HVDC_LINE:
    case Type::STATIC_VAR_COMPENSATOR:
      parametersSets->addParametersSet(buildEventSetPointBooleanDisconnection(element.id, def_.timeOfEvent));
      break;
    default:
      parametersSets->addParametersSet(buildEventConnectedStatusDisconnection(element.id, def_.timeOfEvent));
    }
  }

  exporter.exportToFile(parametersSets, def_.filename, constants::xmlEncoding);
}

std::shared_ptr<parameters::ParametersSet> ParEvent::buildBranchDisconnection(const std::string &branchId, const double timeOfEvent) {
  std::shared_ptr<parameters::ParametersSet> set = parameters::ParametersSetFactory::newParametersSet("Disconnect_" + branchId);
  set->addParameter(helper::buildParameter("event_tEvent", timeOfEvent));
  set->addParameter(helper::buildParameter("event_disconnectOrigin", true));
  set->addParameter(helper::buildParameter("event_disconnectExtremity", true));
  return set;
}

std::shared_ptr<parameters::ParametersSet> ParEvent::buildEventSetPointBooleanDisconnection(const std::string &elementId, const double timeOfEvent) {
  std::shared_ptr<parameters::ParametersSet> set = parameters::ParametersSetFactory::newParametersSet("Disconnect_" + elementId);
  set->addParameter(helper::buildParameter("event_tEvent", timeOfEvent));
  set->addParameter(helper::buildParameter("event_stateEvent1", true));
  return set;
}

std::shared_ptr<parameters::ParametersSet> ParEvent::buildEventConnectedStatusDisconnection(const std::string &elementId, const double timeOfEvent) {
  std::shared_ptr<parameters::ParametersSet> set = parameters::ParametersSetFactory::newParametersSet("Disconnect_" + elementId);
  set->addParameter(helper::buildParameter("event_tEvent", timeOfEvent));
  set->addParameter(helper::buildParameter("event_open", true));
  return set;
}

bool ParEvent::isNetwork(const std::string &elementId) const { return (def_.networkElements_.find(elementId) != def_.networkElements_.end()); }

}  // namespace outputs
}  // namespace dfl
