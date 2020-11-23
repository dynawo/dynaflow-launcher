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

namespace dfl {
namespace outputs {

const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> Dyd::correspondence_lib_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN, "GeneratorPVSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "GeneratorPVDiagramPQSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "GeneratorPVWithImpedanceSignalN"),
    std::make_pair(algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "GeneratorPVDiagramPQWithImpedanceSignalN")};

// cannot use macroConnectorGenImpedenceName_ and macroConnectorGenName_ because they are private variable
const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string> Dyd::correspondence_macro_connector_ = {
    std::make_pair(algo::GeneratorDefinition::ModelType::SIGNALN, "GEN_NETWORK_CONNECTOR"),                     // same as macroConnectorGenName_
    std::make_pair(algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "GEN_NETWORK_CONNECTOR"),          // same as macroConnectorGenName_
    std::make_pair(algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "GEN_IMP_NETWORK_CONNECTOR"),  // same as macroConnectorGenImpedenceName_
    std::make_pair(algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN,
                   "GEN_IMP_NETWORK_CONNECTOR")  // same as macroConnectorGenImpedenceName_
};

const std::string Dyd::macroConnectorLoadName_("LOAD_NETWORK_CONNECTOR");
const std::string Dyd::macroConnectorGenName_("GEN_NETWORK_CONNECTOR");
const std::string Dyd::macroConnectorGenImpedenceName_("GEN_IMP_NETWORK_CONNECTOR");
const std::string Dyd::networkModelName_("NETWORK");
const std::string Dyd::macroConnectorGenSignalNName_("GEN_SIGNALN_CONNECTOR");
const std::string Dyd::signalNModelName_("Model_Signal_N");
const std::string Dyd::macroStaticRefSignalNGeneratorName_("GeneratorStaticRef");
const std::string Dyd::macroStaticRefImpGeneratorName_("GeneratorImpStaticRef");
const std::string Dyd::macroStaticRefLoadName_("LoadRef");

Dyd::Dyd(DydDefinition&& def) : def_{std::forward<DydDefinition>(def)} {}

void
Dyd::write() {
  dynamicdata::XmlExporter exporter;

  auto collection = dynamicdata::DynamicModelsCollectionFactory::newCollection();

  // macros connectors
  auto macro_connectors = writeMacroConnectors();
  for (auto it = macro_connectors.begin(); it != macro_connectors.end(); ++it) {
    collection->addMacroConnector(*it);
  }

  // macro static refs
  auto macro_static_ref = writeMacroStaticRef();
  for (auto it = macro_static_ref.begin(); it != macro_static_ref.end(); ++it) {
    collection->addMacroStaticReference(*it);
  }

  // models
  for (auto it = def_.loads.begin(); it != def_.loads.end(); ++it) {
    collection->addModel(writeLoad(*it, def_.basename));
  }
  auto const_models = writeConstantsModel(def_.basename);
  for (auto it = const_models.begin(); it != const_models.end(); ++it) {
    collection->addModel(*it);
  }
  for (auto it = def_.generators.begin(); it != def_.generators.end(); ++it) {
    collection->addModel(writeGenerator(*it, def_.basename));
  }
  for (const auto& hvdcLine : def_.hvdcLines) {
    collection->addModel(writeHvdcLine(hvdcLine, def_.basename));
  }
  // connections
  for (auto it = def_.loads.begin(); it != def_.loads.end(); ++it) {
    collection->addMacroConnect(writeLoadConnect(*it));
  }

  collection->addConnect(signalNModelName_, "tetaRef_0_value", "NETWORK", def_.slackNode->id + "_phi_value");

  for (auto it = def_.generators.begin(); it != def_.generators.end(); ++it) {
    auto connections = writeGenConnect(*it, static_cast<unsigned int>(it - def_.generators.begin()));
    for (auto it_c = connections.begin(); it_c != connections.end(); ++it_c) {
      collection->addMacroConnect(*it_c);
    }
  }

  for (const auto& hvdcLine : def_.hvdcLines) {
    writeHvdcLineConnect(collection, hvdcLine);
  }
  exporter.exportToFile(collection, def_.filename, constants::xmlEncoding);
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeHvdcLine(const algo::HvdcLineDefinition& hvdcLine, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(hvdcLine.id);

  model->setStaticId(hvdcLine.id);
  if (hvdcLine.converterType == inputs::HvdcLine::ConverterType::LCC) {
    model->setLib("HvdcPTanPhiDangling");
  } else {
    model->setLib("HvdcPVDangling");
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
  }

  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeLoad(const algo::LoadDefinition& load, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(load.id);

  model->setStaticId(load.id);
  model->setLib("LoadAlphaBetaRestorativeLimitsRecalc");
  model->setParFile(basename + ".par");
  model->setParId(constants::loadParId);
  model->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefLoadName_));

  return model;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
Dyd::writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(def.id);

  const std::string& static_ref_macro =
      (def.model == algo::GeneratorDefinition::ModelType::SIGNALN || def.model == algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN)
          ? macroStaticRefSignalNGeneratorName_
          : macroStaticRefImpGeneratorName_;
  std::string parId;
  switch (def.model) {
  case algo::GeneratorDefinition::ModelType::SIGNALN:
    parId = constants::signalNGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN:
    parId = constants::impSignalNGeneratorParId;
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
  model->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(static_ref_macro));

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

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenImpedenceName_);
  connector->addConnect("coupling_terminal1", "@STATIC_ID@@NODE@_ACPIN");
  connector->addConnect("generator_switchOffSignal1", "@STATIC_ID@@NODE@_switchOff");
  ret.push_back(connector);

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorGenSignalNName_);
  connector->addConnect("generator_N_value", "n_grp_@INDEX@_value");
  connector->addConnect("generator_alphaSum_value", "alphaSum_grp_@INDEX@_value");
  connector->addConnect("generator_alpha_value", "alpha_grp_@INDEX@_value");
  ret.push_back(connector);

  connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorLoadName_);
  connector->addConnect("load_terminal", "@STATIC_ID@@NODE@_ACPIN");
  connector->addConnect("load_switchOffSignal1", "@STATIC_ID@@NODE@_switchOff");
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

  ref = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefImpGeneratorName_);
  ref->addStaticRef("coupling_P1GenPu", "p");
  ref->addStaticRef("coupling_Q1GenPu", "q");
  ref->addStaticRef("generator_state", "state");
  ret.push_back(ref);

  ref = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefLoadName_);
  ref->addStaticRef("load_PPu", "p");
  ref->addStaticRef("load_QPu", "q");
  ref->addStaticRef("load_state", "state");
  ret.push_back(ref);

  return ret;
}

boost::shared_ptr<dynamicdata::MacroConnect>
Dyd::writeLoadConnect(const algo::LoadDefinition& loaddef) {
  return dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorLoadName_, loaddef.id, networkModelName_);
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnect>>
Dyd::writeGenConnect(const algo::GeneratorDefinition& def, unsigned int index) {
  auto connection = dynamicdata::MacroConnectFactory::newMacroConnect(correspondence_macro_connector_.at(def.model), def.id, networkModelName_);
  auto signal = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorGenSignalNName_, def.id, signalNModelName_);
  signal->setIndex2(std::to_string(index));
  return {connection, signal};
}

void
Dyd::writeHvdcLineConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& collection, const algo::HvdcLineDefinition& hvdcLine) {
  if (hvdcLine.position == algo::HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT) {
    collection->addConnect("NETWORK", hvdcLine.converter1.busId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
    collection->addConnect("NETWORK", hvdcLine.converter2.busId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
  } else if (hvdcLine.position == algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    collection->addConnect("NETWORK", hvdcLine.converter1.busId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
    collection->addConnect("NETWORK", hvdcLine.converter2.busId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
  }
}  // namespace outputs
}  // namespace outputs
}  // namespace dfl
