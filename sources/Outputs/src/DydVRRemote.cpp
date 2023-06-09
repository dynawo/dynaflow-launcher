//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydVRRemote.h"

#include "DydCommon.h"
#include "OutputsConstants.h"

#include <DYDMacroConnectorFactory.h>
#include <DYDMacroConnectFactory.h>

namespace dfl {
namespace outputs {

void
DydVRRemote::writeVRRemotes(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
                            const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesRegulatedBySeveralGenerators,
                            const std::string& basename) {
  using BusId = std::string;
  std::set<BusId> regulatedBusIds;
  for (const auto& busToGenerator : busesRegulatedBySeveralGenerators)
    regulatedBusIds.insert(busToGenerator.first);
  for (const auto& busToVSC : hvdcDefinitions_.vscBusVSCDefinitionsMap)
    regulatedBusIds.insert(busToVSC.first);

  for (const BusId& regulatedBusId : regulatedBusIds) {
    std::string id = constants::modelSignalNQprefix_ + regulatedBusId;
    boost::shared_ptr<dynamicdata::BlackBoxModel> blackBoxModelVRRemote = helper::buildBlackBox(id, "VRRemote", basename + ".par", id);
    dynamicModelsToConnect->addModel(blackBoxModelVRRemote);
    dynamicModelsToConnect->addConnect(id, "vrremote_URegulated", constants::networkModelName, regulatedBusId + "_U_value");
  }

  writeMacroConnector(dynamicModelsToConnect);
  writeConnections(dynamicModelsToConnect);
}

void
DydVRRemote::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    auto connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenVRRemoteName_);
    connector->addConnect("generator_NQ", "vrremote_NQ");
    connector->addConnect("generator_limUQUp", "vrremote_limUQUp_@INDEX@_");
    connector->addConnect("generator_limUQDown", "vrremote_limUQDown_@INDEX@_");
    dynamicModelsToConnect->addMacroConnector(connector);
  }
}

void
DydVRRemote::writeConnections(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  std::unordered_map<std::string, unsigned int> modelNQIdGenNumber;
  for (auto it = generatorDefinitions_.cbegin(); it != generatorDefinitions_.cend(); ++it) {
    if (it->isNetwork()) {
      continue;
    }
    if (it->model == algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE ||
               it->model == algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN ||
               it->model == algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR) {
      std::string modelNQId = constants::modelSignalNQprefix_ + it->regulatedBusId;
      ++modelNQIdGenNumber[modelNQId];
      auto connection = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenVRRemoteName_, it->id, modelNQId);
      connection->setIndex2(std::to_string(modelNQIdGenNumber[modelNQId]));
      dynamicModelsToConnect->addMacroConnect(connection);
    }
  }

  const std::string vrremoteNqValue("vrremote_NQ");
  for (const auto& keyValue : hvdcDefinitions_.hvdcLines) {
    algo::HVDCDefinition hvdcLine = keyValue.second;
    if (hvdcLine.hasPQPropModel()) {
      const algo::HVDCDefinition::BusId& busId1 =
        (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcLine.converter2BusId : hvdcLine.converter1BusId;
      dynamicModelsToConnect->addConnect(hvdcLine.id, "hvdc_NQ1", constants::modelSignalNQprefix_ + busId1, vrremoteNqValue);
      if (hvdcLine.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
        dynamicModelsToConnect->addConnect(hvdcLine.id, "hvdc_NQ2", constants::modelSignalNQprefix_ + hvdcLine.converter2BusId, vrremoteNqValue);
      }
    }
  }
}

}  // namespace outputs
}  // namespace dfl
