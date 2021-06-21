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
 * @file  Par.cpp
 *
 * @brief Dynaflow launcher PAR file writer implementation file
 *
 */

#include "Par.h"

#include "Constants.h"

#include <DYNCommon.h>
#include <PARParameter.h>
#include <PARParameterFactory.h>
#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARReference.h>
#include <PARReferenceFactory.h>
#include <PARXmlExporter.h>
#include <boost/filesystem.hpp>

namespace dfl {
namespace outputs {

namespace helper {

static constexpr double kGoverNullValue_ = 0.;     ///< KGover null value
static constexpr double kGoverDefaultValue_ = 1.;  ///< KGover default value

template<class T>
static boost::shared_ptr<parameters::Parameter>
buildParameter(const std::string& name, const T& value) {
  return parameters::ParameterFactory::newParameter(name, value);
}

static boost::shared_ptr<parameters::Reference>
buildReference(const std::string& name, const std::string& origName, const std::string& type, const boost::optional<std::string>& componentId = {}) {
  auto ref = parameters::ReferenceFactory::newReference(name);
  ref->setOrigData("IIDM");
  ref->setOrigName(origName);
  ref->setType(type);
  if (componentId.is_initialized()) {
    ref->setComponentId(componentId.value());
  }

  return ref;
}

static std::string
getMacroParameterSetId(const std::string& modelType) {
  return "macro_" + modelType;
}

static std::string
getMacroParameterSetId(algo::GeneratorDefinition::ModelType modelType, bool fixedP) {
  std::string id;
  switch (modelType) {
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN:
  case algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? getMacroParameterSetId(constants::propSignalNGeneratorFixedPParId) : getMacroParameterSetId(constants::propSignalNGeneratorParId);
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN:
  case algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? getMacroParameterSetId(constants::remoteSignalNGeneratorFixedPParId) : getMacroParameterSetId(constants::remoteSignalNGeneratorParId);
    break;
  default:
    id = fixedP ? getMacroParameterSetId(constants::signalNGeneratorFixedPParId) : getMacroParameterSetId(constants::signalNGeneratorParId);
    break;
  }
  return id;
}

static std::string
getGeneratorParameterSetId(algo::GeneratorDefinition::ModelType modelType, bool fixedP) {
  std::string id;
  switch (modelType) {
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN:
  case algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? constants::propSignalNGeneratorFixedPParId : constants::propSignalNGeneratorParId;
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN:
  case algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? constants::remoteSignalNGeneratorFixedPParId : constants::remoteSignalNGeneratorParId;
    break;
  default:
    id = fixedP ? constants::signalNGeneratorFixedPParId : constants::signalNGeneratorParId;
    break;
  }
  return id;
}

static boost::shared_ptr<parameters::MacroParameterSet>
buildMacroParameterSet(const std::string& modelType) {
  boost::shared_ptr<parameters::MacroParameterSet> macroParameterSet =
      boost::shared_ptr<parameters::MacroParameterSet>(new parameters::MacroParameterSet(modelType));
  if (modelType == getMacroParameterSetId(constants::remoteSignalNGeneratorParId + "_vr")) {
    macroParameterSet->addParameter(helper::buildParameter("vrremote_Gain", 1.));
    macroParameterSet->addParameter(helper::buildParameter("vrremote_tIntegral", 1.));
  }
  return macroParameterSet;
}

static boost::shared_ptr<parameters::MacroParameterSet>
buildMacroParameterSet(algo::GeneratorDefinition::ModelType modelType, inputs::Configuration::ActivePowerCompensation activePowerCompensation, bool fixedP) {
  boost::shared_ptr<parameters::MacroParameterSet> macroParameterSet =
      boost::shared_ptr<parameters::MacroParameterSet>(new parameters::MacroParameterSet(getMacroParameterSetId(modelType, fixedP)));
  macroParameterSet->addReference(helper::buildReference("generator_PMin", "pMin", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_PMax", "pMax", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_PRef0Pu", "targetP_pu", "DOUBLE"));
  macroParameterSet->addParameter(helper::buildParameter("generator_tFilter", 0.001));

  double value = fixedP ? kGoverNullValue_ : kGoverDefaultValue_;
  macroParameterSet->addParameter(helper::buildParameter("generator_KGover", value));

  switch (activePowerCompensation) {
  case dfl::inputs::Configuration::ActivePowerCompensation::P:
    macroParameterSet->addReference(helper::buildReference("generator_PNom", "p_pu", "DOUBLE"));
    break;
  case dfl::inputs::Configuration::ActivePowerCompensation::TARGET_P:
    macroParameterSet->addReference(helper::buildReference("generator_PNom", "targetP_pu", "DOUBLE"));
    break;
  case dfl::inputs::Configuration::ActivePowerCompensation::PMAX:
    macroParameterSet->addReference(helper::buildReference("generator_PNom", "pMax_pu", "DOUBLE"));
    break;
  }

  switch (modelType) {
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN:
  case algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    macroParameterSet->addReference(helper::buildReference("generator_QRef0Pu", "targetQ_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QPercent", "qMax_pu", "DOUBLE"));
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN:
  case algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
    macroParameterSet->addReference(helper::buildReference("generator_URef0", "targetV", "DOUBLE"));
    break;
  default:
    macroParameterSet->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
    break;
  }
  return macroParameterSet;
}

}  // namespace helper

Par::Par(ParDefinition&& def) : def_{std::forward<ParDefinition>(def)} {}

void
Par::write() {
  parameters::XmlExporter exporter;

  auto dynamicModelsToConnect = parameters::ParametersSetCollectionFactory::newCollection();
  // adding load constant parameter set
  dynamicModelsToConnect->addParametersSet(writeConstantLoadsSet());
  // loop on generators
  for (auto it = def_.generators.begin(); it != def_.generators.end(); ++it) {
    // we check if the macroParameterSet need by generator model is not already created. If not, we create a new one
    if (!dynamicModelsToConnect->hasMacroParametersSet(helper::getMacroParameterSetId(it->model, DYN::doubleIsZero(it->targetP))) && it->isUsingDiagram()) {
      dynamicModelsToConnect->addMacroParameterSet(helper::buildMacroParameterSet(it->model, def_.activePowerCompensation, DYN::doubleIsZero(it->targetP)));
    }
    // if generator is not using infinite diagrams, no need to create constant sets
    if (it->isUsingDiagram()) {
      dynamicModelsToConnect->addParametersSet(writeGenerator(*it, def_.basename, def_.dirname.generic_string()));
    } else {
      if (!dynamicModelsToConnect->hasParametersSet(helper::getGeneratorParameterSetId(it->model, DYN::doubleIsZero(it->targetP)))) {
        dynamicModelsToConnect->addParametersSet(writeConstantGeneratorsSets(def_.activePowerCompensation, it->model, DYN::doubleIsZero(it->targetP)));
      }
    }
  }
  for (auto it = def_.hvdcLines.begin(); it != def_.hvdcLines.end(); ++it) {
    dynamicModelsToConnect->addParametersSet(writeHdvcLine(it->second));
  }
  // adding parameters sets related with remote voltage control or multiple generator regulating same bus
  for (const auto& keyValue : def_.busesWithDynamicModel) {
    if (!dynamicModelsToConnect->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteSignalNGeneratorParId + "_vr"))) {
      dynamicModelsToConnect->addMacroParameterSet(
          helper::buildMacroParameterSet(helper::getMacroParameterSetId(constants::remoteSignalNGeneratorParId + "_vr")));
    }
    dynamicModelsToConnect->addParametersSet(writeVRRemote(keyValue.first, keyValue.second));
  }

  exporter.exportToFile(dynamicModelsToConnect, def_.filepath.generic_string(), constants::xmlEncoding);
}

boost::shared_ptr<parameters::ParametersSet>
Par::updateSignalNGenerator(const std::string& modelId, dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation, bool fixedP) {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(modelId));
  double value = fixedP ? kGoverNullValue_ : kGoverDefaultValue_;
  set->addParameter(helper::buildParameter("generator_KGover", value));
  set->addParameter(helper::buildParameter("generator_QMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_QMax", constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMax", constants::powerValueMax));

  switch (activePowerCompensation) {
  case dfl::inputs::Configuration::ActivePowerCompensation::P:
  case dfl::inputs::Configuration::ActivePowerCompensation::PMAX:
    set->addReference(helper::buildReference("generator_PNom", "p_pu", "DOUBLE"));
    break;
  case dfl::inputs::Configuration::ActivePowerCompensation::TARGET_P:
    set->addReference(helper::buildReference("generator_PNom", "targetP_pu", "DOUBLE"));
    break;
  default:  //impossible by definition of the enum
    break;
  }

  set->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_PRef0Pu", "targetP_pu", "DOUBLE"));
  if (modelId == constants::remoteSignalNGeneratorParId) {
    set->addReference(helper::buildReference("generator_URef0", "targetV", "DOUBLE"));
  } else if (modelId != constants::propSignalNGeneratorParId) {
    set->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
  }
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeConstantGeneratorsSets(dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                                 dfl::algo::GeneratorDefinition::ModelType modelType, bool fixedP) {
  auto set = updateSignalNGenerator(helper::getGeneratorParameterSetId(modelType, fixedP), activePowerCompensation, fixedP);
  switch (modelType) {
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN:
  case algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    updatePropParameters(set);
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN:
  case algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
  case algo::GeneratorDefinition::ModelType::SIGNALN:
  case algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN:
    break;
  }
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeConstantLoadsSet() {
  // Load
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(constants::loadParId));
  set->addParameter(helper::buildParameter("load_Alpha", 1.5));
  set->addParameter(helper::buildParameter("load_Beta", 2.5));
  set->addParameter(helper::buildParameter("load_UMax0Pu", 1.15));
  set->addParameter(helper::buildParameter("load_UMin0Pu", 0.85));
  set->addParameter(helper::buildParameter("load_UDeadBandPu", 0.01));
  set->addParameter(helper::buildParameter("load_tFilter", 10.));
  set->addReference(helper::buildReference("load_P0Pu", "p0_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_Q0Pu", "q0_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_UPhase0", "angle_pu", "DOUBLE"));
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeVRRemote(const std::string& busId, const std::string& genId) {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet("Model_Signal_NQ_" + busId));
  set->addReference(helper::buildReference("vrremote_U0", "targetV", "DOUBLE", genId));
  set->addReference(helper::buildReference("vrremote_URef0", "targetV", "DOUBLE", genId));
  set->addMacroParSet(
      boost::shared_ptr<parameters::MacroParSet>(new parameters::MacroParSet(helper::getMacroParameterSetId(constants::remoteSignalNGeneratorParId + "_vr"))));
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeHdvcLine(const algo::HvdcLineDefinition& hvdcLine) {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(hvdcLine.id));
  std::string first = "1";
  std::string second = "2";
  if (hvdcLine.position == dfl::algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    first = "2";
    second = "1";
  } else if (hvdcLine.position == dfl::algo::HvdcLineDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
    // TODO when we will handle the case were both converters are in the main connex component
  }
  set->addReference(helper::buildReference("hvdc_P10Pu", "p" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_Q10Pu", "q" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_U10Pu", "v" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_UPhase10", "angle" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_P20Pu", "p" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_Q20Pu", "q" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_U20Pu", "v" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_UPhase20", "angle" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_PMaxPu", "pMax_pu", "DOUBLE"));

  set->addParameter(helper::buildParameter("hvdc_Q1MinPu", std::numeric_limits<double>::lowest()));
  set->addParameter(helper::buildParameter("hvdc_Q1MaxPu", std::numeric_limits<double>::max()));
  set->addParameter(helper::buildParameter("hvdc_Q2MinPu", std::numeric_limits<double>::lowest()));
  set->addParameter(helper::buildParameter("hvdc_Q2MaxPu", std::numeric_limits<double>::max()));
  set->addParameter(helper::buildParameter("hvdc_KLosses", 1.0));

  if (hvdcLine.converterType == dfl::inputs::HvdcLine::ConverterType::VSC) {
    if (hvdcLine.position == dfl::algo::HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT) {
      set->addParameter(helper::buildParameter("hvdc_modeU10", hvdcLine.converter1_voltageRegulationOn.value()));
      set->addParameter(helper::buildParameter("hvdc_modeU20", hvdcLine.converter2_voltageRegulationOn.value()));
    } else if (hvdcLine.position == dfl::algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
      set->addParameter(helper::buildParameter("hvdc_modeU10", hvdcLine.converter2_voltageRegulationOn.value()));
      set->addParameter(helper::buildParameter("hvdc_modeU20", hvdcLine.converter1_voltageRegulationOn.value()));
    } else {
      // TODO when we will handle the case were both converters are in the main connex component
    }
  }
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename, const std::string& dirname) {
  std::size_t hashId = constants::hash(def.id);
  std::string hashIdStr = std::to_string(hashId);

  //  Use the hash id in exported files to prevent use of non-ascii characters
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(hashIdStr));
  // The macroParSet is associated to a macroParameterSet via the id
  set->addMacroParSet(
      boost::shared_ptr<parameters::MacroParSet>(new parameters::MacroParSet(helper::getMacroParameterSetId(def.model, DYN::doubleIsZero(def.targetP)))));

  // Qmax and QMin are determined in dynawo according to reactive capabilities curves and min max
  // we need a small numerical tolerance in case the starting point of the reactive injection is exactly
  // on the limit of the reactive capability curve
  set->addParameter(helper::buildParameter("generator_QMin0", def.qmin - 1));
  set->addParameter(helper::buildParameter("generator_QMax0", def.qmax + 1));

  auto dirname_diagram = boost::filesystem::path(dirname);
  dirname_diagram.append(basename + constants::diagramDirectorySuffix).append(constants::diagramFilename(def));

  set->addParameter(helper::buildParameter("generator_QMaxTableFile", dirname_diagram.generic_string()));
  set->addParameter(helper::buildParameter("generator_QMaxTableName", hashIdStr + constants::diagramMaxTableSuffix));
  set->addParameter(helper::buildParameter("generator_QMinTableFile", dirname_diagram.generic_string()));
  set->addParameter(helper::buildParameter("generator_QMinTableName", hashIdStr + constants::diagramMinTableSuffix));

  return set;
}

void
Par::updatePropParameters(boost::shared_ptr<parameters::ParametersSet> set) {
  set->addReference(helper::buildReference("generator_QRef0Pu", "targetQ_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_QPercent", "qMax_pu", "DOUBLE"));
}
}  // namespace outputs
}  // namespace dfl
