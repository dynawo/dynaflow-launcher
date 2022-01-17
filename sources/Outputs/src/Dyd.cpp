//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  Dyd.cpp
 *
 * @brief Dynaflow launcher DYD file writer implementation file
 *
 */

#include "Dyd.h"

#include "Constants.h"
#include "Log.h"
#include "Message.hpp"

#include <DYDBlackBoxModelFactory.h>
#include <DYDDynamicModelsCollection.h>
#include <DYDDynamicModelsCollectionFactory.h>
#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRef.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>
#include <DYDStaticRef.h>
#include <DYDXmlExporter.h>
#include <DYNCommon.h>

namespace dfl {
namespace outputs {

const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> Dyd::correspondence_lib_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN, "GeneratorPVSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "GeneratorPVDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN, "GeneratorPVRemoteSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "GeneratorPVRemoteDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN, "GeneratorPQPropSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "GeneratorPQPropDiagramPQSignalN")};

const std::string Dyd::macroConnectorLoadName_("LOAD_NETWORK_CONNECTOR");
const std::string Dyd::macroConnectorGenName_("GEN_NETWORK_CONNECTOR");
const std::string Dyd::macroConnectorGenSignalNName_("GEN_SIGNALN_CONNECTOR");
const std::string Dyd::signalNModelName_("Model_Signal_N");
const std::string Dyd::macroStaticRefSignalNGeneratorName_("GeneratorStaticRef");
const std::string Dyd::macroStaticRefSVarCName_("StaticVarCompensatorStaticRef");
const std::string Dyd::macroConnectorSVarCName_("StaticVarCompensatorMacroConnector");
const std::string Dyd::macroStaticRefLoadName_("LoadRef");
const std::string Dyd::modelSignalNQprefix_("Model_Signal_NQ_");

const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> Dyd::correspondence_macro_connector_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, macroConnectorGenName_)};

const std::unordered_map<algo::HVDCDefinition::HVDCModel, std::string> Dyd::hvdcModelsNames_ = {
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhi, "HvdcPTanPhi"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, "HvdcPTanPhiDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ, "HvdcPTanPhiDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ, "HvdcPTanPhiDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQProp, "HvdcPQProp"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling, "HvdcPQPropDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ, "HvdcPQPropDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, "HvdcPQPropDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulation, "HvdcPQPropDiagramPQEmulation"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulation, "HvdcPQPropEmulation"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPV, "HvdcPV"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDangling, "HvdcPVDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ, "HvdcPVDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQ, "HvdcPVDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulation, "HvdcPVDiagramPQEmulation"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVEmulation, "HvdcPVEmulation"),
};

const std::unordered_map<algo::StaticVarCompensatorDefinition::ModelType, std::string> Dyd::svarcModelsNames_ = {
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPV, "StaticVarCompensatorPV"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING, "StaticVarCompensatorPVModeHandling"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROP, "StaticVarCompensatorPVProp"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING, "StaticVarCompensatorPVPropModeHandling"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE, "StaticVarCompensatorPVPropRemote"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING, "StaticVarCompensatorPVPropRemoteModeHandling"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE, "StaticVarCompensatorPVRemote"),
  std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING, "StaticVarCompensatorPVRemoteModeHandling")
  };

Dyd::Dyd(DydDefinition&& def) : def_{std::forward<DydDefinition>(def)} {}

void
Dyd::write() const {
  dynamicdata::XmlExporter exporter;
  const auto& assemblingDoc = def_.dynamicDataBaseManager.assemblingDocument();

  auto dynamicModelsToConnect = dynamicdata::DynamicModelsCollectionFactory::newCollection();

  // macros connectors
  auto macroConnectors = writeMacroConnectors();
  for (const auto& macroConnector : macroConnectors) {
    dynamicModelsToConnect->addMacroConnector(macroConnector);
  }

  std::unordered_map<std::string, inputs::AssemblingXmlDocument::MacroConnection> macrosById;
  std::transform(assemblingDoc.macroConnections().begin(), assemblingDoc.macroConnections().end(), std::inserter(macrosById, macrosById.begin()),
                 [](const inputs::AssemblingXmlDocument::MacroConnection& macro) { return std::make_pair(macro.id, macro); });

  macroConnectors = writeDynamicModelMacroConnectors(def_.dynamicModelsDefinitions.usedMacroConnections, macrosById);
  for (const auto& macroConnector : macroConnectors) {
    dynamicModelsToConnect->addMacroConnector(macroConnector);
  }

  // macro static refs
  auto macroStaticRefs = writeMacroStaticRef();
  for (const auto& macroStaticRef : macroStaticRefs) {
    dynamicModelsToConnect->addMacroStaticReference(macroStaticRef);
  }

  // models and connections
  for (const auto& load : def_.loads) {
    dynamicModelsToConnect->addModel(writeLoad(load, def_.basename));
    dynamicModelsToConnect->addMacroConnect(writeLoadConnect(load));
  }
  auto const_models = writeConstantsModel();
  for (const auto& const_model : const_models) {
    dynamicModelsToConnect->addModel(const_model);
  }
  for (const auto& generator : def_.generators) {
    dynamicModelsToConnect->addModel(writeGenerator(generator, def_.basename));
  }
  for (const auto& keyValue : def_.hvdcDefinitions.hvdcLines) {
    dynamicModelsToConnect->addModel(writeHvdcLine(keyValue.second, def_.basename));
    writeHvdcLineConnect(dynamicModelsToConnect, keyValue.second);
  }
  for (const auto& keyValue : def_.busesWithDynamicModel) {
    dynamicModelsToConnect->addModel(writeVRRemote(keyValue.first, def_.basename));
    writeVRRemoteConnect(dynamicModelsToConnect, keyValue.first);
  }
  for (const auto& keyValue : def_.hvdcDefinitions.vscBusVSCDefinitionsMap) {
    dynamicModelsToConnect->addModel(writeVRRemote(keyValue.first, def_.basename));
    writeVRRemoteConnect(dynamicModelsToConnect, keyValue.first);
  }
  for (const auto& model : def_.dynamicModelsDefinitions.models) {
    dynamicModelsToConnect->addModel(writeDynamicModel(model.second, def_.basename));
    auto macroConnects = writeDynamicModelMacroConnect(model.second);
    for (const auto& connect : macroConnects) {
      dynamicModelsToConnect->addMacroConnect(connect);
    }
  }
  for (const auto& svarcRef : def_.svarcsDefs) {
    dynamicModelsToConnect->addModel(writeSVarC(svarcRef, def_.basename));
    dynamicModelsToConnect->addMacroConnect(writeSVarCMacroConnect(svarcRef));
  }

  dynamicModelsToConnect->addConnect(signalNModelName_, "signalN_thetaRef", constants::networkModelName, def_.slackNode->id + "_phi");

  for (auto it = def_.generators.cbegin(); it != def_.generators.cend(); ++it) {
    writeGenConnect(dynamicModelsToConnect, *it);
    auto connections = writeGenMacroConnect(*it, static_cast<unsigned int>(it - def_.generators.cbegin()));
    for (const auto& connection : connections) {
      dynamicModelsToConnect->addMacroConnect(connection);
    }
  }

  exporter.exportToFile(dynamicModelsToConnect, def_.filename, constants::xmlEncoding);
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnector>>
Dyd::writeDynamicModelMacroConnectors(const std::unordered_set<std::string>& usedMacros,
                                      const std::unordered_map<std::string, inputs::AssemblingXmlDocument::MacroConnection>& macros) {
  std::vector<boost::shared_ptr<dynamicdata::MacroConnector>> ret;
  for (const auto& macro : usedMacros) {
    auto macroConnector = dynamicdata::MacroConnectorFactory::newMacroConnector(macro);
    auto found = macros.find(macro);
#if _DEBUG_
    assert(found != macros.end());
#endif
    if (found == macros.end()) {
      // macro used in dynamic model not defined in configuration:  configuration error
      LOG(warn) << MESS(DynModelMacroNotDefined, macro) << LOG_ENDL;
      continue;
    }

    for (const auto& connection : found->second.connections) {
      macroConnector->addConnect(connection.var1, connection.var2);
    }
    ret.push_back(macroConnector);
  }

  return ret;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeDynamicModel(const algo::DynamicModelDefinition& dynModel, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(dynModel.id);
  model->setLib(dynModel.lib);
  model->setParFile(basename + ".par");
  model->setParId(dynModel.id);
  return model;
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnect>>
Dyd::writeDynamicModelMacroConnect(const algo::DynamicModelDefinition& dynModel) {
  std::vector<boost::shared_ptr<dynamicdata::MacroConnect>> ret;
  const auto& connections = dynModel.nodeConnections;

  // Here we compute the number of connections performed by macro connection, in order to generate the corresponding
  // index attributes
  std::unordered_map<std::string, std::tuple<unsigned int, unsigned int>> indexes;
  enum { INDEXES_NB_CONNECTIONS = 0, INDEXES_CURRENT_INDEX };
  for (const auto& connection : connections) {
    if (indexes.count(connection.id) > 0) {
      (std::get<INDEXES_NB_CONNECTIONS>(indexes.at(connection.id)))++;
    } else {
      indexes[connection.id] = std::make_tuple(1, 0);
    }
  }

  for (const auto& connection : connections) {
    auto macroConnect = dynamicdata::MacroConnectFactory::newMacroConnect(connection.id, dynModel.id, constants::networkModelName);
    macroConnect->setName2(connection.connectedElementId);
#if _DEBUG_
    assert(std::get<INDEXES_CURRENT_INDEX>(indexes.at(connection.id)) < std::get<INDEXES_NB_CONNECTIONS>(indexes.at(connection.id)));
#endif
    // We put index1 to 0 even in case there is only one connection, for consistency in the output file
    macroConnect->setIndex1(std::to_string(std::get<INDEXES_CURRENT_INDEX>(indexes.at(connection.id))));
    (std::get<INDEXES_CURRENT_INDEX>(indexes.at(connection.id)))++;
    ret.push_back(macroConnect);
  }

  return ret;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeVRRemote(const std::string& busId, const std::string& basename) {
  std::string id = modelSignalNQprefix_ + busId;
  auto model = dynamicdata::BlackBoxModelFactory::newModel(id);
  model->setLib("VRRemote");
  model->setParFile(basename + ".par");
  model->setParId(id);
  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeHvdcLine(const algo::HVDCDefinition& hvdcLine, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(hvdcLine.id);

  model->setStaticId(hvdcLine.id);
  model->setLib(hvdcModelsNames_.at(hvdcLine.model));
  model->setParFile(basename + ".par");
  model->setParId(hvdcLine.id);
  if (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    model->addStaticRef("hvdc_PInj1Pu", "p2");
    model->addStaticRef("hvdc_QInj1Pu", "q2");
    model->addStaticRef("hvdc_state", "state2");
    model->addStaticRef("hvdc_PInj2Pu", "p1");
    model->addStaticRef("hvdc_QInj2Pu", "q1");
    model->addStaticRef("hvdc_state", "state1");
  } else {
    // terminal 1 on 1 in case both are in main connex component
    model->addStaticRef("hvdc_PInj1Pu", "p1");
    model->addStaticRef("hvdc_QInj1Pu", "q1");
    model->addStaticRef("hvdc_state", "state1");
    model->addStaticRef("hvdc_PInj2Pu", "p2");
    model->addStaticRef("hvdc_QInj2Pu", "q2");
    model->addStaticRef("hvdc_state", "state2");
  }

  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeLoad(const algo::LoadDefinition& load, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(load.id);

  model->setStaticId(load.id);
  model->setLib("DYNModelLoadRestorativeWithLimits");
  model->setParFile(basename + ".par");
  model->setParId(constants::loadParId);
  model->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefLoadName_));

  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(def.id);
  std::string parId;
  switch (def.model) {
  case algo::GeneratorDefinition::ModelType::SIGNALN:
    parId = (DYN::doubleIsZero(def.targetP)) ? constants::signalNGeneratorFixedPParId : constants::signalNGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN:
    parId = constants::propSignalNGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN:
    parId = constants::remoteVControlParId;
    break;
  default:
    std::size_t hashId = constants::hash(def.id);
    std::string hashIdStr = std::to_string(hashId);
    parId = hashIdStr;
    break;
  }

  model->setStaticId(def.id);
  model->setLib(correspondence_lib_.at(def.model));
  model->setParFile(basename + ".par");
  model->setParId(parId);
  model->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefSignalNGeneratorName_));
  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeSVarC(const algo::StaticVarCompensatorDefinition& svarc, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(svarc.id);
  std::string parId;
  std::size_t hashId = constants::hash(svarc.id);
  std::string hashIdStr = std::to_string(hashId);
  parId = hashIdStr;
  model->setStaticId(svarc.id);
  model->setLib(svarcModelsNames_.at(svarc.model));
  model->setParFile(basename + ".par");
  model->setParId(parId);
  model->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefSVarCName_));
  switch (svarc.model) {
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING:
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING:
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING:
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING:
    model->addStaticRef("SVarC_modeHandling_mode_value", "regulatingMode");
    break;
  default:
    break;
  }

  return model;
}

std::vector<boost::shared_ptr<dynamicdata::BlackBoxModel>>
Dyd::writeConstantsModel() {
  std::vector<boost::shared_ptr<dynamicdata::BlackBoxModel>> ret;
  auto model = dynamicdata::BlackBoxModelFactory::newModel(signalNModelName_);
  model->setLib("SignalN");

  ret.push_back(model);

  return ret;
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnector>>
Dyd::writeMacroConnectors() {
  std::vector<boost::shared_ptr<dynamicdata::MacroConnector>> ret;

  auto connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenName_);
  connector->addConnect("generator_terminal", "@STATIC_ID@@NODE@_ACPIN");
  connector->addConnect("generator_switchOffSignal1", "@STATIC_ID@@NODE@_switchOff");
  ret.push_back(connector);

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenSignalNName_);
  connector->addConnect("generator_N", "signalN_N");
  ret.push_back(connector);

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorLoadName_);
  connector->addConnect("Ur_value", "@STATIC_ID@@NODE@_ACPIN_V_re");
  connector->addConnect("Ui_value", "@STATIC_ID@@NODE@_ACPIN_V_im");
  connector->addConnect("Ir_value", "@STATIC_ID@@NODE@_ACPIN_i_re");
  connector->addConnect("Ii_value", "@STATIC_ID@@NODE@_ACPIN_i_im");
  connector->addConnect("switchOff1_value", "@STATIC_ID@@NODE@_switchOff_value");
  ret.push_back(connector);

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorSVarCName_);
  connector->addConnect("SVarC_terminal", "@STATIC_ID@@NODE@_ACPIN");
  ret.push_back(connector);

  return ret;
}

std::vector<boost::shared_ptr<dynamicdata::MacroStaticReference>>
Dyd::writeMacroStaticRef() {
  std::vector<boost::shared_ptr<dynamicdata::MacroStaticReference>> ret;

  auto ref = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefSignalNGeneratorName_);
  ref->addStaticRef("generator_PGenPu", "p");
  ref->addStaticRef("generator_QGenPu", "q");
  ref->addStaticRef("generator_state", "state");
  ret.push_back(ref);

  ref = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefLoadName_);
  ref->addStaticRef("PPu_value", "p");
  ref->addStaticRef("QPu_value", "q");
  ref->addStaticRef("state_value", "state");
  ret.push_back(ref);

  ref = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefSVarCName_);
  ref->addStaticRef("SVarC_PInjPu", "p");
  ref->addStaticRef("SVarC_QInjPu", "q");
  ref->addStaticRef("SVarC_state", "state");
  ret.push_back(ref);

  return ret;
}

boost::shared_ptr<dynamicdata::MacroConnect>
Dyd::writeLoadConnect(const algo::LoadDefinition& loaddef) {
  return dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorLoadName_, loaddef.id, constants::networkModelName);
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnect>>
Dyd::writeGenMacroConnect(const algo::GeneratorDefinition& def, unsigned int index) {
  auto connection = dynamicdata::MacroConnectFactory::newMacroConnect(correspondence_macro_connector_.at(def.model), def.id, constants::networkModelName);
  auto signal = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenSignalNName_, def.id, signalNModelName_);
  signal->setIndex2(std::to_string(index));
  return {connection, signal};
}

boost::shared_ptr<dynamicdata::MacroConnect>
Dyd::writeSVarCMacroConnect(const algo::StaticVarCompensatorDefinition& svarc) {
  return dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorSVarCName_, svarc.id, constants::networkModelName);
}

void
Dyd::writeGenConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::GeneratorDefinition& def) {
  if (def.model == algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN || def.model == algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN) {
    dynamicModelsToConnect->addConnect(def.id, "generator_URegulated", constants::networkModelName, def.regulatedBusId + "_U_value");
  } else if (def.model == algo::GeneratorDefinition::ModelType::PROP_SIGNALN || def.model == algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN) {
    dynamicModelsToConnect->addConnect(def.id, "generator_NQ_value", modelSignalNQprefix_ + def.regulatedBusId, "vrremote_NQ");
  }
}

void
Dyd::writeVRRemoteConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& busId) {
  dynamicModelsToConnect->addConnect(modelSignalNQprefix_ + busId, "vrremote_URegulated", constants::networkModelName, busId + "_U_value");
}

void
Dyd::writeHvdcLineConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::HVDCDefinition& hvdcDefinition) {
  const std::string vrremoteNqValue("vrremote_NQ");
  if (hvdcDefinition.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcDefinition.converter1BusId + "_ACPIN", hvdcDefinition.id, "hvdc_terminal2");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcDefinition.converter2BusId + "_ACPIN", hvdcDefinition.id, "hvdc_terminal1");
  } else {
    // case both : 1 <-> 1 and 2 <-> 2
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcDefinition.converter1BusId + "_ACPIN", hvdcDefinition.id, "hvdc_terminal1");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcDefinition.converter2BusId + "_ACPIN", hvdcDefinition.id, "hvdc_terminal2");
  }
  if (hvdcDefinition.hasPQPropModel()) {
    const auto& busId1 =
        (hvdcDefinition.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcDefinition.converter2BusId : hvdcDefinition.converter1BusId;
    dynamicModelsToConnect->addConnect(hvdcDefinition.id, "hvdc_NQ1_value", modelSignalNQprefix_ + busId1, vrremoteNqValue);
    if (hvdcDefinition.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
      dynamicModelsToConnect->addConnect(hvdcDefinition.id, "hvdc_NQ2_value", modelSignalNQprefix_ + hvdcDefinition.converter2BusId, vrremoteNqValue);
    }
  }
}  // namespace outputs
}  // namespace outputs
}  // namespace dfl
