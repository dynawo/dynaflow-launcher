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

#include "Constants.h"

#include <DYNCommon.h>
#include <PARParameter.h>
#include <PARParameterFactory.h>
#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARReference.h>
#include <PARReferenceFactory.h>
#include <PARXmlExporter.h>
#include <boost/filesystem.hpp>

namespace dfl {
namespace outputs {

namespace helper {

template<class T>
static boost::shared_ptr<parameters::Parameter>
buildParameter(const std::string& name, const T& value) {
  return parameters::ParameterFactory::newParameter(name, value);
}
} // namespace helper

ParEvent::ParEvent(ParEventDefinition&& def) : def_{std::forward<ParEventDefinition>(def)} {}

void
ParEvent::write() {
  parameters::XmlExporter exporter;

  auto parametersSets = parameters::ParametersSetCollectionFactory::newCollection();
  // TODO(Luma) loop on pairs (elementId, elementType) of current contingency
  if (def_.elementType == "BRANCH") {
    parametersSets->addParametersSet(writeBranchDisconnection(def_.elementId, def_.timeEvent));
  }

  exporter.exportToFile(parametersSets, def_.filename, constants::xmlEncoding);
}

boost::shared_ptr<parameters::ParametersSet>
ParEvent::writeBranchDisconnection(const std::string& branchId, double timeEvent) {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet("Disconnect_" + branchId));
  set->addParameter(helper::buildParameter("event_tEvent", timeEvent));
  set->addParameter(helper::buildParameter("event_disconnectOrigin", true));
  set->addParameter(helper::buildParameter("event_disconnectExtremity", true));
  return set;
}

}  // namespace outputs
}  // namespace dfl
