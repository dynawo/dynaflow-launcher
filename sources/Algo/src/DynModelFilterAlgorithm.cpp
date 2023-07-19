//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DynModelFilterAlgorithm.h"
#include "Constants.h"
#include "Log.h"

namespace dfl {
namespace algo {

void
DynModelFilterAlgorithm::filter() {
  removeRpclInGeneratorsAndSvcIfMissingConnexionToSvc();
  filterPartiallyConnectedDynamicModels();
}

void
DynModelFilterAlgorithm::removeRpclInGeneratorsAndSvcIfMissingConnexionToSvc() {
  std::vector<DynamicModelDefinition::DynModelId> svcToRemove;
  for (const auto& dynamicModel : dynamicModelsToFilter_) {
    if (dynamicModel.second.lib == dfl::common::constants::svcModelName) {
      bool isSVCconnectedToUMeasurement = checkIfSVCconnectedToUMeasurement(dynamicModel.second);
      if (!isSVCconnectedToUMeasurement) {
        svcToRemove.push_back(dynamicModel.second.id);
        for (const DynamicModelDefinition::MacroConnection& generatorConnectedToSVC : dynamicModel.second.nodeConnections) {
          const DynamicModelDefinition::MacroConnection::ElementId& genId = generatorConnectedToSVC.connectedElementId;
          auto foundGen = std::find_if(generators_.begin(), generators_.end(),
                                        [&genId](const algo::GeneratorDefinition& genDefinition) {
                                          return genDefinition.id == genId;
                                        });
          if (foundGen != generators_.end()) {
            foundGen->removeRpclFromModel();
          }
        }
      }
    }
  }
  for (const DynamicModelDefinition::DynModelId& svcId : svcToRemove) {
    LOG(debug, SVCNotConnectectedToBus, svcId);
    dynamicModelsToFilter_.erase(svcId);
  }
}

bool
DynModelFilterAlgorithm::checkIfSVCconnectedToUMeasurement(const algo::DynamicModelDefinition& svcModel) const {
  bool isSVCconnectedToUMeasurement = false;
  const std::map<std::string, inputs::AssemblingDataBase::DynamicAutomaton>& assemblingAutomatons = assembling_.dynamicAutomatons();
  auto assemblingAutomatonIt = assemblingAutomatons.find(svcModel.id);
  assert(assemblingAutomatonIt != assemblingAutomatons.end());
  const std::vector<inputs::AssemblingDataBase::MacroConnect>& macroConnects = assemblingAutomatonIt->second.macroConnects;
  for (inputs::AssemblingDataBase::MacroConnect macroConnect : macroConnects) {
    const inputs::AssemblingDataBase::SingleAssociation& singleAssociation = assembling_.getSingleAssociation(macroConnect.id);
    if (singleAssociation.bus.is_initialized()) {
      auto macroConnectIt = std::find_if(svcModel.nodeConnections.begin(), svcModel.nodeConnections.end(),
                                          [&macroConnect](const DynamicModelDefinition::MacroConnection& nodeConnection) {
                                            return nodeConnection.id == macroConnect.macroConnection;
                                          });
      if (macroConnectIt != svcModel.nodeConnections.end()) {
        isSVCconnectedToUMeasurement = true;
        break;
      }
    }
  }
  return isSVCconnectedToUMeasurement;
}

void
DynModelFilterAlgorithm::filterPartiallyConnectedDynamicModels() {
  const std::map<std::string, inputs::AssemblingDataBase::DynamicAutomaton>& automatonsConfig = assembling_.dynamicAutomatons();
  for (const auto& automaton : automatonsConfig) {
    if (dynamicModelsToFilter_.count(automaton.second.id) == 0) {
      continue;
    }

    DynamicModelDefinition& modelDef = dynamicModelsToFilter_.at(automaton.second.id);

    // Filtering generators with default model or regulating a node regulated by several generators from SVCs
    if (automaton.second.lib == dfl::common::constants::svcModelName) {
      std::vector<algo::DynamicModelDefinition::MacroConnection> toRemove;
      for (const DynamicModelDefinition::MacroConnection& connection : modelDef.nodeConnections) {
        DynamicModelDefinition::MacroConnection::ElementId compId = connection.connectedElementId;
        auto found = std::find_if(generators_.begin(), generators_.end(),
                                  [&compId](const algo::GeneratorDefinition& genDefinition) { return genDefinition.id == compId; });
        if (found != generators_.end() && found->isNetwork()) {
          LOG(debug, SVCConnectedToDefaultGen, connection.connectedElementId, automaton.second.id);
          toRemove.push_back(connection);
        } else if (found != generators_.end() && !found->hasRpcl()) {
          LOG(debug, SVCConnectedToGenRegulatingNode, connection.connectedElementId, automaton.second.id);
          toRemove.push_back(connection);
        }
      }
      for (const DynamicModelDefinition::MacroConnection& connection : toRemove) {
        modelDef.nodeConnections.erase(connection);
      }
      if (modelDef.nodeConnections.size() <= 1) {
        LOG(debug, EmptySVC, automaton.second.id);
        dynamicModelsToFilter_.erase(automaton.second.id);
      }
      continue;
    }

    if (dynamicModelsToFilter_.find(automaton.second.id) == dynamicModelsToFilter_.end())
      continue;

    for (const inputs::AssemblingDataBase::MacroConnect& macroConnect : automaton.second.macroConnects) {
      auto found = std::find_if(
          modelDef.nodeConnections.begin(), modelDef.nodeConnections.end(),
          [&macroConnect](const algo::DynamicModelDefinition::MacroConnection &macroConnection) { return macroConnection.id == macroConnect.macroConnection; });
      if (found == modelDef.nodeConnections.end()) {
        LOG(debug, ModelPartiallyConnected, automaton.second.id);
        dynamicModelsToFilter_.erase(automaton.second.id);
        break;  // element doesn't exist any more, go to next automaton
      }
    }
  }
}

}  // namespace algo
}  // namespace dfl
