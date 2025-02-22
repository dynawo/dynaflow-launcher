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

#include "DydCommon.h"
#include "OutputsConstants.h"
#include "ParCommon.h"

#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>

namespace dfl {
namespace outputs {

const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> DydGenerator::correspondence_lib_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "GeneratorPVSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "GeneratorPVSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "GeneratorPVDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE, "GeneratorPVRpclSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_RECTANGULAR, "GeneratorPVRpclSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL_SIGNALN, "GeneratorPVDiagramPQRpclSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_INFINITE, "GeneratorPVRpcl2SignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_RECTANGULAR, "GeneratorPVRpcl2SignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL2_SIGNALN, "GeneratorPVDiagramPQRpcl2SignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE, "GeneratorPVTfoSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RECTANGULAR, "GeneratorPVTfoSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_SIGNALN, "GeneratorPVTfoDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_INFINITE, "GeneratorPVTfoRpclSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_RECTANGULAR, "GeneratorPVTfoRpclSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL_SIGNALN, "GeneratorPVTfoDiagramPQRpclSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_INFINITE, "GeneratorPVTfoRpcl2SignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR, "GeneratorPVTfoRpcl2SignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL2_SIGNALN, "GeneratorPVTfoDiagramPQRpcl2SignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "GeneratorPVRemoteSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR, "GeneratorPVRemoteSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "GeneratorPVRemoteDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "GeneratorPQPropSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "GeneratorPQPropSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "GeneratorPQPropDiagramPQSignalN")};

void DydGenerator::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const std::string &basename,
                         const std::string &slackNodeId) {
  for (const auto &generator : generatorDefinitions_) {
    if (generator.isNetwork()) {
      continue;
    }
    std::string parId = helper::getGeneratorParameterSetId(generator);
    std::unique_ptr<dynamicdata::BlackBoxModel> blackBoxModel =
        helper::buildBlackBoxStaticId(generator.id, generator.id, correspondence_lib_.at(generator.model), basename + ".par", parId);
    blackBoxModel->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefSignalNGeneratorName_));
    dynamicModelsToConnect->addModel(std::move(blackBoxModel));
  }
  writeThetaRefConnect(dynamicModelsToConnect, slackNodeId);
  writeMacroConnector(dynamicModelsToConnect);
  writeMacroStaticReference(dynamicModelsToConnect);
  writeSignalNBlackBox(dynamicModelsToConnect);
  writeMacroConnect(dynamicModelsToConnect);
}

void DydGenerator::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    std::unique_ptr<dynamicdata::MacroConnector> connector1 = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenName_);
    connector1->addConnect("generator_terminal", "@STATIC_ID@@NODE@_ACPIN");
    connector1->addConnect("generator_switchOffSignal1", "@STATIC_ID@@NODE@_switchOff");
    dynamicModelsToConnect->addMacroConnector(std::move(connector1));

    std::unique_ptr<dynamicdata::MacroConnector> connector2 = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenSignalNName_);
    connector2->addConnect("generator_N", "signalN_N");
    dynamicModelsToConnect->addMacroConnector(std::move(connector2));
  }
}

void DydGenerator::writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    std::unique_ptr<dynamicdata::MacroStaticReference> macroStaticReference =
        dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefSignalNGeneratorName_);
    macroStaticReference->addStaticRef("generator_PGenPu", "p");
    macroStaticReference->addStaticRef("generator_QGenPu", "q");
    macroStaticReference->addStaticRef("generator_state", "state");
    dynamicModelsToConnect->addMacroStaticReference(std::move(macroStaticReference));
  }
}

void DydGenerator::writeSignalNBlackBox(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect) {
  if (!generatorDefinitions_.empty()) {
    std::unique_ptr<dynamicdata::BlackBoxModel> blackBoxModelSignalN = dynamicdata::BlackBoxModelFactory::newModel(signalNModelName_);
    blackBoxModelSignalN->setLib("SignalN");
    dynamicModelsToConnect->addModel(std::move(blackBoxModelSignalN));
  }
}

void DydGenerator::writeMacroConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect) {
  std::unordered_map<std::string, unsigned int> modelNQIdGenNumber;
  for (auto it = generatorDefinitions_.cbegin(); it != generatorDefinitions_.cend(); ++it) {
    if (it->isNetwork()) {
      continue;
    }
    if (it->model == algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE ||
        it->model == algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN ||
        it->model == algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR) {
      dynamicModelsToConnect->addConnect(it->id, "generator_URegulated", constants::networkModelName, it->regulatedBusId + "_U_value");
    }

    std::unique_ptr<dynamicdata::MacroConnect> connection =
        dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenName_, it->id, constants::networkModelName);
    std::unique_ptr<dynamicdata::MacroConnect> signal =
        dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenSignalNName_, it->id, signalNModelName_);
    dynamicModelsToConnect->addMacroConnect(std::move(connection));
    dynamicModelsToConnect->addMacroConnect(std::move(signal));
  }
}

void DydGenerator::writeThetaRefConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const std::string &slackNodeId) {
  if (!generatorDefinitions_.empty()) {
    dynamicModelsToConnect->addConnect(signalNModelName_, "signalN_thetaRef", constants::networkModelName, slackNodeId + "_phi_value");
  }
}
}  // namespace outputs
}  // namespace dfl
