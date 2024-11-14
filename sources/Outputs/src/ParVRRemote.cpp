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
  for (const auto &busId2Number : busesToNumberOfRegulationMap_)
    if (busId2Number.second == dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES)
      componentToFrozen.insert({busId2Number.first, true});

  for (const auto &generator : generatorDefinitions_) {
    if (componentToFrozen.find(generator.regulatedBusId) != componentToFrozen.end() && generator.q < generator.qmax && generator.q > generator.qmin) {
      componentToFrozen[generator.regulatedBusId] = false;
    }
  }

  for (const auto &hvdc : hvdcDefinitions_.hvdcLines) {
    const auto &hvdcDefinition = hvdc.second;

    if (hvdcDefinition.vscDefinition1 && hvdcDefinition.vscDefinition1->q < hvdcDefinition.vscDefinition1->qmax &&
        hvdcDefinition.vscDefinition1->q > hvdcDefinition.vscDefinition1->qmin) {
      componentToFrozen[hvdcDefinition.converter1BusId] = false;
    }

    if (hvdcDefinition.vscDefinition2 && hvdcDefinition.vscDefinition2->q < hvdcDefinition.vscDefinition2->qmax &&
        hvdcDefinition.vscDefinition2->q > hvdcDefinition.vscDefinition2->qmin) {
      componentToFrozen[hvdcDefinition.converter2BusId] = false;
    }
  }

  std::set<std::string> handledBus;
  for (const auto &keyValue : generatorDefinitions_) {
    if (keyValue.isRegulatingLocallyWithOthers()) {
      assert(busesToNumberOfRegulationMap_.find(keyValue.regulatedBusId) != busesToNumberOfRegulationMap_.end() &&
             busesToNumberOfRegulationMap_.find(keyValue.regulatedBusId)->second == dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES);
      assert(busesToNumberOfRegulationMap_.find(keyValue.regulatedBusId) != busesToNumberOfRegulationMap_.end());
      if (handledBus.find(keyValue.regulatedBusId) != handledBus.end())
        continue;
      if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlVRParId))) {
        paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlVRParId)));
      }
      auto VRRemoteParamSet = helper::writeVRRemote(keyValue.regulatedBusId, keyValue.id);
      if (componentToFrozen[keyValue.regulatedBusId])
        VRRemoteParamSet->addParameter(helper::buildParameter("vrremote_Frozen0", true));
      paramSetCollection->addParametersSet(VRRemoteParamSet);
      handledBus.insert(keyValue.regulatedBusId);
    }
  }

  for (const auto &keyValue : hvdcDefinitions_.hvdcLines) {
    algo::HVDCDefinition hvdcLine = keyValue.second;
    if (hvdcLine.hasPQPropModel()) {
      const algo::HVDCDefinition::BusId &busId1 =
          (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcLine.converter2BusId : hvdcLine.converter1BusId;
      const auto &vscStation = (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcLine.converter2Id : hvdcLine.converter1Id;
      if (handledBus.find(busId1) == handledBus.end()) {
        if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlVRParId))) {
          paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlVRParId)));
        }
        auto VRRemoteParamSet = helper::writeVRRemote(busId1, vscStation);
        if (componentToFrozen[busId1])
          VRRemoteParamSet->addParameter(helper::buildParameter("vrremote_Frozen0", true));
        paramSetCollection->addParametersSet(VRRemoteParamSet);
        handledBus.insert(busId1);
      }

      if (hvdcLine.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT && handledBus.find(hvdcLine.converter2BusId) == handledBus.end()) {
        if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlVRParId))) {
          paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlVRParId)));
        }
        auto VRRemoteParamSet = helper::writeVRRemote(hvdcLine.converter2BusId, hvdcLine.converter2Id);
        if (componentToFrozen[hvdcLine.converter2BusId])
          VRRemoteParamSet->addParameter(helper::buildParameter("vrremote_Frozen0", true));
        paramSetCollection->addParametersSet(VRRemoteParamSet);
        handledBus.insert(hvdcLine.converter2BusId);
      }
    }
  }
}

}  // namespace outputs
}  // namespace dfl
