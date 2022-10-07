//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydGenerator.h"

#include "Constants.h"
#include "DydCommon.h"

#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>
#include <DYNCommon.h>

namespace dfl {
namespace outputs {

const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> DydGenerator::correspondence_lib_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "GeneratorPVSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "GeneratorPVSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "GeneratorPVDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE, "GeneratorPVTfoSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RECTANGULAR, "GeneratorPVTfoSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_SIGNALN, "GeneratorPVTfoDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "GeneratorPVRemoteSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR, "GeneratorPVRemoteSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "GeneratorPVRemoteDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "GeneratorPQPropSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "GeneratorPQPropSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "GeneratorPQPropDiagramPQSignalN")};

void
DydGenerator::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename,
                    const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel, const std::string& slackNodeId) {
  for (const auto& generator : generatorDefinitions_) {
    if (generator.isNetwork()) {
      continue;
    }
    std::string parId = getSpecificParId(generator);
    auto blackBoxModel = helper::buildBlackBoxStaticId(generator.id, generator.id, correspondence_lib_.at(generator.model), basename + ".par", parId);
    blackBoxModel->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefSignalNGeneratorName_));
    dynamicModelsToConnect->addModel(blackBoxModel);
  }
  for (const auto& keyValue : busesWithDynamicModel) {
    std::string id = constants::modelSignalNQprefix_ + keyValue.first;
    auto blackBoxModelVRRemote = helper::buildBlackBox(id, "VRRemote", basename + ".par", id);
    dynamicModelsToConnect->addModel(blackBoxModelVRRemote);
    dynamicModelsToConnect->addConnect(id, "vrremote_URegulated", constants::networkModelName, keyValue.first + "_U_value");
  }
  writeThetaRefConnect(dynamicModelsToConnect, slackNodeId);
  writeMacroConnector(dynamicModelsToConnect);
  writeMacroStaticReference(dynamicModelsToConnect);
  writeSignalNBlackBox(dynamicModelsToConnect);
  writeMacroConnect(dynamicModelsToConnect);
}

void
DydGenerator::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    auto connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenName_);
    connector->addConnect("generator_terminal", "@STATIC_ID@@NODE@_ACPIN");
    connector->addConnect("generator_switchOffSignal1", "@STATIC_ID@@NODE@_switchOff");
    dynamicModelsToConnect->addMacroConnector(connector);

    connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenSignalNName_);
    connector->addConnect("generator_N", "signalN_N");
    dynamicModelsToConnect->addMacroConnector(connector);
  }
}

void
DydGenerator::writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    auto macroStaticReference = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefSignalNGeneratorName_);
    macroStaticReference->addStaticRef("generator_PGenPu", "p");
    macroStaticReference->addStaticRef("generator_QGenPu", "q");
    macroStaticReference->addStaticRef("generator_state", "state");
    dynamicModelsToConnect->addMacroStaticReference(macroStaticReference);
  }
}

void
DydGenerator::writeSignalNBlackBox(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    auto blackBoxModelSignalN = dynamicdata::BlackBoxModelFactory::newModel(signalNModelName_);
    blackBoxModelSignalN->setLib("SignalN");
    dynamicModelsToConnect->addModel(blackBoxModelSignalN);
  }
}

void
DydGenerator::writeMacroConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  for (auto it = generatorDefinitions_.cbegin(); it != generatorDefinitions_.cend(); ++it) {
    if (it->isNetwork()) {
      continue;
    }
    if (it->model == algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE ||
        it->model == algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN ||
        it->model == algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR) {
      dynamicModelsToConnect->addConnect(it->id, "generator_URegulated", constants::networkModelName, it->regulatedBusId + "_U_value");
    } else if (it->model == algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE ||
               it->model == algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN ||
               it->model == algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR) {
      dynamicModelsToConnect->addConnect(it->id, "generator_NQ", constants::modelSignalNQprefix_ + it->regulatedBusId, "vrremote_NQ");
    }

    auto connection = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenName_, it->id, constants::networkModelName);
    auto signal = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenSignalNName_, it->id, signalNModelName_);
    signal->setIndex2(std::to_string(static_cast<unsigned int>(it - generatorDefinitions_.cbegin())));
    dynamicModelsToConnect->addMacroConnect(connection);
    dynamicModelsToConnect->addMacroConnect(signal);
  }
}

std::string
DydGenerator::getSpecificParId(const dfl::algo::GeneratorDefinition& generator) {
  std::string parId;
  switch (generator.model) {
  case algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE:
    parId = (DYN::doubleIsZero(generator.targetP)) ? constants::signalNGeneratorFixedPParId : constants::signalNGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE:
    parId = constants::signalNTfoGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE:
    parId = constants::propSignalNGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE:
    parId = constants::remoteVControlParId;
    break;
  default:
    std::size_t hashId = constants::hash(generator.id);
    std::string hashIdStr = std::to_string(hashId);
    parId = hashIdStr;
    break;
  }
  return parId;
}

void
DydGenerator::writeThetaRefConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& slackNodeId) {
  if (!generatorDefinitions_.empty()) {
    dynamicModelsToConnect->addConnect(signalNModelName_, "signalN_thetaRef", constants::networkModelName, slackNodeId + "_phi_value");
  }
}
}  // namespace outputs
}  // namespace dfl
