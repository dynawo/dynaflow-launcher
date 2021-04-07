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
const std::string Dyd::networkModelName_("NETWORK");
const std::string Dyd::macroConnectorGenSignalNName_("GEN_SIGNALN_CONNECTOR");
const std::string Dyd::signalNModelName_("Model_Signal_N");
const std::string Dyd::macroStaticRefSignalNGeneratorName_("GeneratorStaticRef");
const std::string Dyd::macroStaticRefLoadName_("LoadRef");

const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> Dyd::correspondence_macro_connector_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_SIGNALN, macroConnectorGenName_),
    std::make_pair(algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, macroConnectorGenName_)};

Dyd::Dyd(DydDefinition&& def) : def_{std::forward<DydDefinition>(def)} {}

void
Dyd::write() {
  dynamicdata::XmlExporter exporter;

  auto dynamicModelsToConnect = dynamicdata::DynamicModelsCollectionFactory::newCollection();

  // macros connectors
  auto macro_connectors = writeMacroConnectors();
  for (auto it = macro_connectors.begin(); it != macro_connectors.end(); ++it) {
    dynamicModelsToConnect->addMacroConnector(*it);
  }

  // macro static refs
  auto macro_static_ref = writeMacroStaticRef();
  for (auto it = macro_static_ref.begin(); it != macro_static_ref.end(); ++it) {
    dynamicModelsToConnect->addMacroStaticReference(*it);
  }

  // models and connections
  for (const auto& load : def_.loads) {
    dynamicModelsToConnect->addModel(writeLoad(load, def_.basename));
    dynamicModelsToConnect->addMacroConnect(writeLoadConnect(load));
  }
  auto const_models = writeConstantsModel(def_.basename);
  for (const auto& const_model : const_models) {
    dynamicModelsToConnect->addModel(const_model);
  }
  for (const auto& generator : def_.generators) {
    dynamicModelsToConnect->addModel(writeGenerator(generator, def_.basename));
  }
  for (const auto& keyValue : def_.hvdcLines) {
    dynamicModelsToConnect->addModel(writeHvdcLine(keyValue.second, def_.basename));
    writeHvdcLineConnect(dynamicModelsToConnect, keyValue.second);
  }
  for (const auto& keyValue : def_.busesWithDynamicModel) {
    dynamicModelsToConnect->addModel(writeVRRemote(keyValue.first, def_.basename));
    writeVRRemoteConnect(dynamicModelsToConnect, keyValue.first);
  }

  dynamicModelsToConnect->addConnect(signalNModelName_, "tetaRef_0_value", "NETWORK", def_.slackNode->id + "_phi_value");

  for (auto it = def_.generators.begin(); it != def_.generators.end(); ++it) {
    writeGenConnect(dynamicModelsToConnect, *it);
    auto connections = writeGenMacroConnect(*it, static_cast<unsigned int>(it - def_.generators.begin()));
    for (auto it_c = connections.begin(); it_c != connections.end(); ++it_c) {
      dynamicModelsToConnect->addMacroConnect(*it_c);
    }
  }

  exporter.exportToFile(dynamicModelsToConnect, def_.filename, constants::xmlEncoding);
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeVRRemote(const std::string& busId, const std::string& basename) {
  std::string id = "Model_Signal_NQ_" + busId;
  auto model = dynamicdata::BlackBoxModelFactory::newModel(id);
  model->setLib("VRRemote");
  model->setParFile(basename + ".par");
  model->setParId(id);
  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeHvdcLine(const algo::HvdcLineDefinition& hvdcLine, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(hvdcLine.id);

  model->setStaticId(hvdcLine.id);
  if (hvdcLine.position == algo::HvdcLineDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
    if (hvdcLine.converterType == inputs::HvdcLine::ConverterType::LCC) {
      // TODO set lib
    } else {
      // TODO set lib
    }
  } else {
    if (hvdcLine.converterType == inputs::HvdcLine::ConverterType::LCC) {
      model->setLib("HvdcPTanPhiDangling");
    } else {
      model->setLib("HvdcPVDangling");
    }
  }
  model->setParFile(basename + ".par");
  model->setParId(hvdcLine.id);
  if (hvdcLine.position == algo::HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT) {
    model->addStaticRef("hvdc_PInj1Pu", "p1");
    model->addStaticRef("hvdc_QInj1Pu", "q1");
    model->addStaticRef("hvdc_state", "state1");
    model->addStaticRef("hvdc_PInj2Pu", "p2");
    model->addStaticRef("hvdc_QInj2Pu", "q2");
    model->addStaticRef("hvdc_state", "state2");
  } else if (hvdcLine.position == algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    model->addStaticRef("hvdc_PInj1Pu", "p2");
    model->addStaticRef("hvdc_QInj1Pu", "q2");
    model->addStaticRef("hvdc_state", "state2");
    model->addStaticRef("hvdc_PInj2Pu", "p1");
    model->addStaticRef("hvdc_QInj2Pu", "q1");
    model->addStaticRef("hvdc_state", "state1");
  } else {
    // TODO when we will handle the case were both converters are in the main connex component
  }

  return model;
}  // namespace outputs

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
    parId = constants::remoteSignalNGeneratorParId;
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

std::vector<boost::shared_ptr<dynamicdata::BlackBoxModel>>
Dyd::writeConstantsModel(const std::string& basename) {
  std::vector<boost::shared_ptr<dynamicdata::BlackBoxModel>> ret;
  auto model = dynamicdata::BlackBoxModelFactory::newModel(signalNModelName_);
  model->setLib("DYNModelSignalN");
  model->setParFile(basename + ".par");
  model->setParId(constants::signalNParId);

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
  connector->addConnect("generator_N_value", "n_grp_@INDEX@_value");
  connector->addConnect("generator_alpha_value", "alpha_grp_@INDEX@_value");
  ret.push_back(connector);

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorLoadName_);
  connector->addConnect("Ur_value", "@STATIC_ID@@NODE@_ACPIN_V_re");
  connector->addConnect("Ui_value", "@STATIC_ID@@NODE@_ACPIN_V_im");
  connector->addConnect("Ir_value", "@STATIC_ID@@NODE@_ACPIN_i_re");
  connector->addConnect("Ii_value", "@STATIC_ID@@NODE@_ACPIN_i_im");
  connector->addConnect("switchOff1_value", "@STATIC_ID@@NODE@_switchOff_value");
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

  return ret;
}

boost::shared_ptr<dynamicdata::MacroConnect>
Dyd::writeLoadConnect(const algo::LoadDefinition& loaddef) {
  return dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorLoadName_, loaddef.id, networkModelName_);
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnect>>
Dyd::writeGenMacroConnect(const algo::GeneratorDefinition& def, unsigned int index) {
  auto connection = dynamicdata::MacroConnectFactory::newMacroConnect(correspondence_macro_connector_.at(def.model), def.id, networkModelName_);
  auto signal = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenSignalNName_, def.id, signalNModelName_);
  signal->setIndex2(std::to_string(index));
  return {connection, signal};
}

void
Dyd::writeGenConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::GeneratorDefinition& def) {
  if (def.model == algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN || def.model == algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN) {
    dynamicModelsToConnect->addConnect(def.id, "generator_URegulated", "NETWORK", def.regulatedBusId + "_U_value");
  } else if (def.model == algo::GeneratorDefinition::ModelType::PROP_SIGNALN || def.model == algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN) {
    dynamicModelsToConnect->addConnect(def.id, "generator_NQ_value", "Model_Signal_NQ_" + def.regulatedBusId, "vrremote_NQ");
  }
}

void
Dyd::writeVRRemoteConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& busId) {
  dynamicModelsToConnect->addConnect("Model_Signal_NQ_" + busId, "vrremote_URegulated", "NETWORK", busId + "_U_value");
}

void
Dyd::writeHvdcLineConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::HvdcLineDefinition& hvdcLine) {
  if (hvdcLine.position == algo::HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT) {
    dynamicModelsToConnect->addConnect("NETWORK", hvdcLine.converter1_busId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
    dynamicModelsToConnect->addConnect("NETWORK", hvdcLine.converter2_busId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
  } else if (hvdcLine.position == algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    dynamicModelsToConnect->addConnect("NETWORK", hvdcLine.converter1_busId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
    dynamicModelsToConnect->addConnect("NETWORK", hvdcLine.converter2_busId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
  } else {
    // TODO when we will handle the case were both converters are in the main connex component
  }
}  // namespace outputs
}  // namespace outputs
}  // namespace dfl
