//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParVRRemote.h"

#include "ParCommon.h"

namespace dfl {
namespace outputs {

void
ParVRRemote::writeVRRemotes(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection) {
  std::unordered_map<algo::GeneratorDefinitionAlgorithm::BusId, bool> busesRegulatedBySeveralGeneratorsToFrozen;
  for (const auto &keyValue : busesRegulatedBySeveralGenerators_)
    busesRegulatedBySeveralGeneratorsToFrozen.insert({keyValue.first, true});
  for (const auto &generator : generatorDefinitions_) {
    // if network model, nothing to do
    if (generator.isNetwork()) {
      continue;
    }
    if (busesRegulatedBySeveralGeneratorsToFrozen.find(generator.regulatedBusId) != busesRegulatedBySeveralGeneratorsToFrozen.end() &&
        generator.q < generator.qmax && generator.q > generator.qmin) {
      busesRegulatedBySeveralGeneratorsToFrozen[generator.regulatedBusId] = false;
    }
  }

  // adding parameters sets related to remote voltage control or multiple generator regulating same bus
  for (const auto &keyValue : busesRegulatedBySeveralGenerators_) {
    if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    auto VRRemoteParamSet = helper::writeVRRemote(keyValue.first, keyValue.second);
    if (busesRegulatedBySeveralGeneratorsToFrozen[keyValue.first])
      VRRemoteParamSet->addParameter(helper::buildParameter("vrremote_frozen0", true));
    paramSetCollection->addParametersSet(VRRemoteParamSet);
  }

  // adding parameters sets related to remote voltage control or multiple VSC regulating same bus
  for (const auto &keyValue : vscBusVSCDefinitionsMap_) {
    if (busesRegulatedBySeveralGenerators_.find(keyValue.first) != busesRegulatedBySeveralGenerators_.end()) {
      continue;
    }
    if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    paramSetCollection->addParametersSet(helper::writeVRRemote(keyValue.first, keyValue.second));
  }
}

}  // namespace outputs
}  // namespace dfl
