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

void ParVRRemote::writeVRRemotes(boost::shared_ptr<parameters::ParametersSetCollection> &paramSetCollection) {
  std::unordered_map<algo::GeneratorDefinitionAlgorithm::BusId, bool> componentToFrozen;
  for (const auto &keyValue : busesRegulatedBySeveralGenerators_)
    componentToFrozen.insert({keyValue.first, true});
  for (const auto &keyValue : hvdcDefinitions_.vscBusVSCDefinitionsMap)
    componentToFrozen.insert({keyValue.first, true});

  for (const auto &generator : generatorDefinitions_) {
    // if network model, nothing to do
    if (generator.isNetwork()) {
      continue;
    }
    if (componentToFrozen.find(generator.regulatedBusId) != componentToFrozen.end() && generator.q < generator.qmax && generator.q > generator.qmin) {
      componentToFrozen[generator.regulatedBusId] = false;
    }
  }

  for (const auto &hvdc : hvdcDefinitions_.hvdcLines) {
    const auto &hvdcDefinition = hvdc.second;

    if (hvdcDefinition.vscDefinition1 && hvdcDefinition.vscDefinition1->q < hvdcDefinition.vscDefinition1->qmax &&
        hvdcDefinition.vscDefinition1->q > hvdcDefinition.vscDefinition1->qmin) {
      componentToFrozen[hvdcDefinition.converter1BusId] = false;
      std::cout << "BUBU?? " << hvdcDefinition.vscDefinition1->q << " " << hvdcDefinition.vscDefinition1->qmin << " " << hvdcDefinition.vscDefinition1->qmax
                << std::endl;
    }

    if (hvdcDefinition.vscDefinition2 && hvdcDefinition.vscDefinition2->q < hvdcDefinition.vscDefinition2->qmax &&
        hvdcDefinition.vscDefinition2->q > hvdcDefinition.vscDefinition2->qmin) {
      componentToFrozen[hvdcDefinition.converter2BusId] = false;
      std::cout << "BUBU2?? " << hvdcDefinition.vscDefinition1->q << " " << hvdcDefinition.vscDefinition1->qmin << " " << hvdcDefinition.vscDefinition1->qmax
                << std::endl;
    }
  }

  // adding parameters sets related to remote voltage control or multiple generator regulating same bus
  for (const auto &keyValue : busesRegulatedBySeveralGenerators_) {
    if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    auto VRRemoteParamSet = helper::writeVRRemote(keyValue.first, keyValue.second);
    if (componentToFrozen[keyValue.first])
      VRRemoteParamSet->addParameter(helper::buildParameter("vrremote_Frozen0", true));
    paramSetCollection->addParametersSet(VRRemoteParamSet);
  }

  // adding parameters sets related to remote voltage control or multiple VSC regulating same bus
  for (const auto &keyValue : hvdcDefinitions_.vscBusVSCDefinitionsMap) {
    if (busesRegulatedBySeveralGenerators_.find(keyValue.first) != busesRegulatedBySeveralGenerators_.end()) {
      continue;
    }
    if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    auto VRRemoteParamSet = helper::writeVRRemote(keyValue.first, keyValue.second);
    if (componentToFrozen[keyValue.first])
      VRRemoteParamSet->addParameter(helper::buildParameter("vrremote_Frozen0", true));
    paramSetCollection->addParametersSet(VRRemoteParamSet);
  }
}

}  // namespace outputs
}  // namespace dfl
