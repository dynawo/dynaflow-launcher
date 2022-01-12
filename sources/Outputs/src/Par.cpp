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
#include "Log.h"
#include "Message.hpp"
#include "ParCommon.hpp"

#include <DYNCommon.h>
#include <DYNDataInterface.h>
#include <DYNNetworkInterface.h>
#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARReference.h>
#include <PARReferenceFactory.h>
#include <PARXmlExporter.h>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <type_traits>

namespace dfl {
namespace outputs {

namespace helper {

static constexpr double kGoverNullValue_ = 0.;     ///< KGover null value
static constexpr double kGoverDefaultValue_ = 1.;  ///< KGover default value

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
    id = fixedP ? getMacroParameterSetId(constants::remoteSignalNGeneratorFixedP) : getMacroParameterSetId(constants::remoteVControlParId);
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
    id = fixedP ? constants::remoteSignalNGeneratorFixedP : constants::remoteVControlParId;
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
  if (modelType == getMacroParameterSetId(constants::remoteVControlParId + "_vr")) {
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

const std::string Par::componentTransformerIdTag_("@TFO@");
const std::string Par::seasonTag_("@SAISON@");
const std::string Par::macroParameterSetStaticCompensator_("MacroParameterSetStaticCompensator");

Par::Par(ParDefinition&& def) : def_{std::forward<ParDefinition>(def)} {}

void
Par::write() const {
  parameters::XmlExporter exporter;

  auto dynamicModelsToConnect = parameters::ParametersSetCollectionFactory::newCollection();
  // adding load constant parameter set
  dynamicModelsToConnect->addParametersSet(writeConstantLoadsSet());
  // loop on generators
  for (const auto& generator : def_.generators) {
    // we check if the macroParameterSet need by generator model is not already created. If not, we create a new one
    if (!dynamicModelsToConnect->hasMacroParametersSet(helper::getMacroParameterSetId(generator.model, DYN::doubleIsZero(generator.targetP))) &&
        generator.isUsingDiagram()) {
      dynamicModelsToConnect->addMacroParameterSet(
          helper::buildMacroParameterSet(generator.model, def_.activePowerCompensation, DYN::doubleIsZero(generator.targetP)));
    }
    // if generator is not using infinite diagrams, no need to create constant sets
    if (generator.isUsingDiagram()) {
      dynamicModelsToConnect->addParametersSet(writeGenerator(generator, def_.basename, def_.dirname));
    } else {
      if (!dynamicModelsToConnect->hasParametersSet(helper::getGeneratorParameterSetId(generator.model, DYN::doubleIsZero(generator.targetP)))) {
        dynamicModelsToConnect->addParametersSet(
            writeConstantGeneratorsSets(def_.activePowerCompensation, generator.model, DYN::doubleIsZero(generator.targetP)));
      }
    }
  }

  if (!def_.svarcsDefinitions.empty()) {
    dynamicModelsToConnect->addMacroParameterSet(writeMacroParameterSetStaticVarCompensators());
  }
  for (const auto& svarc : def_.svarcsDefinitions) {
    dynamicModelsToConnect->addParametersSet(writeStaticVarCompensator(svarc));
  }

  for (const auto& hvdcLine : def_.hvdcDefinitions.hvdcLines) {
    dynamicModelsToConnect->addParametersSet(writeHdvcLine(hvdcLine.second, def_.basename, def_.dirname));
  }
  // adding parameters sets related to remote voltage control or multiple generator regulating same bus
  for (const auto& keyValue : def_.busesWithDynamicModel) {
    if (!dynamicModelsToConnect->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      dynamicModelsToConnect->addMacroParameterSet(helper::buildMacroParameterSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    dynamicModelsToConnect->addParametersSet(writeVRRemote(keyValue.first, keyValue.second));
  }
  // adding parameters sets related to remote voltage control or multiple VSC regulating same bus
  for (const auto& keyValue : def_.hvdcDefinitions.vscBusVSCDefinitionsMap) {
    if (!dynamicModelsToConnect->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      dynamicModelsToConnect->addMacroParameterSet(helper::buildMacroParameterSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    dynamicModelsToConnect->addParametersSet(writeVRRemote(keyValue.first, keyValue.second.id));
  }

  const auto& sets = def_.dynamicDataBaseManager.settingDocument().sets();
  for (const auto& set : sets) {
    auto new_set = writeDynamicModelParameterSet(set, def_.dynamicDataBaseManager.assemblingDocument(), def_.shuntCounters, def_.dynamicModelsDefinitions,
                                                 def_.linesByIdDefinitions);
    if (new_set) {
      dynamicModelsToConnect->addParametersSet(new_set);
    }
  }

  exporter.exportToFile(dynamicModelsToConnect, def_.filepath.generic_string(), constants::xmlEncoding);
}

boost::optional<std::string>
Par::getTransformerComponentId(const algo::DynamicModelDefinition& dynModelDef) {
  for (const auto& macro : dynModelDef.nodeConnections) {
    if (macro.elementType == algo::DynamicModelDefinition::MacroConnection::ElementType::TFO) {
      return macro.connectedElementId;
    }
  }

  return boost::none;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeDynamicModelParameterSet(const inputs::SettingXmlDocument::Set& set, const inputs::AssemblingXmlDocument& assemblingDoc,
                                   const algo::ShuntCounterDefinitions& counters, const algo::DynamicModelDefinitions& models,
                                   const algo::LinesByIdDefinitions& linesById) {
  if (models.models.count(set.id) == 0) {
    // model is not connected : ignore corresponding set
    return nullptr;
  }

  auto new_set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(set.id));
  const auto& multipleAssociations = assemblingDoc.multipleAssociations();
  std::unordered_map<std::string, inputs::AssemblingXmlDocument::MultipleAssociation> associationsById;
  std::transform(multipleAssociations.begin(), multipleAssociations.end(), std::inserter(associationsById, associationsById.begin()),
                 [](const inputs::AssemblingXmlDocument::MultipleAssociation& association) { return std::make_pair(association.id, association); });
  for (const auto& count : set.counts) {
    auto found = associationsById.find(count.id);
    if (found == associationsById.end()) {
      LOG(debug) << "Count id " << count.id << " not found as a multiple association in assembling: Configuration error" << LOG_ENDL;
      continue;
    }
    if (counters.nbShunts.count(found->second.shunt.voltageLevel) == 0) {
      // case voltage level not in network, skip
      continue;
    }
    new_set->addParameter(helper::buildParameter(count.name, static_cast<int>(counters.nbShunts.at(found->second.shunt.voltageLevel))));
  }

  for (const auto& param : set.boolParameters) {
    new_set->addParameter(helper::buildParameter(param.name, param.value));
  }
  for (const auto& param : set.doubleParameters) {
    new_set->addParameter(helper::buildParameter(param.name, param.value));
  }
  for (const auto& param : set.integerParameters) {
    new_set->addParameter(helper::buildParameter(param.name, param.value));
  }

  for (const auto& param : set.stringParameters) {
    new_set->addParameter(helper::buildParameter(param.name, param.value));
  }

  for (const auto& ref : set.references) {
    auto componentId = ref.componentId;

    // special case @TFO@ reference
    if (componentId && *componentId == componentTransformerIdTag_) {
      componentId = getTransformerComponentId(models.models.at(set.id));
      if (!componentId) {
        // Configuration error : references is using a TFO element while no TFO element is connected to the dynamic model
        LOG(warn) << MESS(TFOComponentNotFound, ref.name, set.id) << LOG_ENDL;
        continue;
      }
    }
    new_set->addReference(helper::buildReference(ref.name, ref.origName, inputs::SettingXmlDocument::Reference::toString(ref.dataType), componentId));
  }

  for (const auto& ref : set.refs) {
    if (ref.tag == seasonTag_) {
      auto seasonOpt = getActiveSeason(ref, linesById, assemblingDoc);
      if (!seasonOpt) {
        continue;
      }
      new_set->addParameter(helper::buildParameter(ref.name, *seasonOpt));
    } else {
      // Unsupported case
      LOG(warn) << MESS(RefUnsupportedTag, ref.name, ref.tag) << LOG_ENDL;
    }
  }

  return new_set;
}

boost::optional<std::string>
Par::getActiveSeason(const inputs::SettingXmlDocument::Ref& ref, const algo::LinesByIdDefinitions& linesById,
                     const inputs::AssemblingXmlDocument& assemblingDoc) {
  const auto& singleAssocations = assemblingDoc.singleAssociations();

  // the ref element references the single association to use
  auto foundAsso = std::find_if(singleAssocations.begin(), singleAssocations.end(),
                                [&ref](const inputs::AssemblingXmlDocument::SingleAssociation& asso) { return asso.id == ref.id; });
  if (foundAsso == singleAssocations.end()) {
    LOG(warn) << MESS(SingleAssociationRefNotFound, ref.name, ref.id) << LOG_ENDL;
    return boost::none;
  }
  if (!foundAsso->line) {
    LOG(warn) << MESS(SingleAssociationRefNotALine, ref.name, ref.id) << LOG_ENDL;
    return boost::none;
  }
  const auto& lineId = foundAsso->line->name;
  auto foundLine = linesById.linesMap.find(lineId);
  if (foundLine == linesById.linesMap.end()) {
    LOG(warn) << MESS(RefLineNotFound, ref.name, ref.id, lineId) << LOG_ENDL;
    return boost::none;
  }

  return foundLine->second.activeSeason;
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
  default:  //  impossible by definition of the enum
    break;
  }

  set->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_PRef0Pu", "targetP_pu", "DOUBLE"));
  if (modelId == constants::remoteVControlParId) {
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
Par::writeVRRemote(const std::string& busId, const std::string& elementId) {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet("Model_Signal_NQ_" + busId));
  set->addReference(helper::buildReference("vrremote_U0", "targetV", "DOUBLE", elementId));
  set->addReference(helper::buildReference("vrremote_URef0", "targetV", "DOUBLE", elementId));
  set->addMacroParSet(
      boost::shared_ptr<parameters::MacroParSet>(new parameters::MacroParSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))));
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeHdvcLine(const algo::HVDCDefinition& hvdcDefinition, const std::string& basename, const boost::filesystem::path& dirname) {
  auto dirnameDiagram = dirname;
  dirnameDiagram.append(basename + constants::diagramDirectorySuffix);

  // Define this function as a lambda instead of a class function to avoid too much arguments that would make it less readable
  auto updateHVDCParams = [&hvdcDefinition, &dirnameDiagram](boost::shared_ptr<parameters::ParametersSet> set,
                                                             const algo::HVDCDefinition::ConverterId& converterId, unsigned int converterNumber,
                                                             unsigned int parameterNumber) {
    constexpr double factorPU = 100;
    std::size_t hashId = constants::hash(converterId);
    std::string hashIdStr = std::to_string(hashId);
    auto dirnameDiagramLocal = dirnameDiagram;
    dirnameDiagramLocal.append(constants::diagramFilename(converterId));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MinTableFile", dirnameDiagramLocal.generic_string()));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MinTableName", hashIdStr + constants::diagramMinTableSuffix));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MaxTableFile", dirnameDiagramLocal.generic_string()));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MaxTableName", hashIdStr + constants::diagramMaxTableSuffix));
    if (hvdcDefinition.converterType == algo::HVDCDefinition::ConverterType::VSC) {
      const auto& vscDefinition = (converterId == hvdcDefinition.converter1Id) ? *hvdcDefinition.vscDefinition1 : *hvdcDefinition.vscDefinition2;
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Min0Pu", (vscDefinition.qmin - 1) / factorPU));
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Max0Pu", (vscDefinition.qmax + 1) / factorPU));
    } else {
      // assuming that converterNumber is 1 or 2 (pre-condition)
      auto qMax = constants::computeQmax(hvdcDefinition.powerFactors.at(converterNumber - 1), hvdcDefinition.pMax);
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Min0Pu", (-qMax - 1) / factorPU));
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Max0Pu", (qMax + 1) / factorPU));
    }
  };

  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(hvdcDefinition.id));
  std::string first = "1";
  std::string second = "2";
  if (hvdcDefinition.position == dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    first = "2";
    second = "1";
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
  set->addParameter(helper::buildParameter("hvdc_KLosses", 1.0));

  if (!hvdcDefinition.hasDiagramModel()) {
    set->addParameter(helper::buildParameter("hvdc_Q1MinPu", std::numeric_limits<double>::lowest()));
    set->addParameter(helper::buildParameter("hvdc_Q1MaxPu", std::numeric_limits<double>::max()));
    set->addParameter(helper::buildParameter("hvdc_Q2MinPu", std::numeric_limits<double>::lowest()));
    set->addParameter(helper::buildParameter("hvdc_Q2MaxPu", std::numeric_limits<double>::max()));
  } else {
    const auto& hvdcConverterIdMain =
        (hvdcDefinition.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcDefinition.converter2Id : hvdcDefinition.converter1Id;
    size_t converterNumber = (hvdcDefinition.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? 2 : 1;
    constexpr size_t parameterMainNumber = 1;
    updateHVDCParams(set, hvdcConverterIdMain, converterNumber, parameterMainNumber);

    if (hvdcDefinition.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
      // always converter 2 on parameter number 2 to add in that case
      updateHVDCParams(set, hvdcDefinition.converter2Id, 2, 2);
    }
  }

  if (hvdcDefinition.converterType == dfl::inputs::HvdcLine::ConverterType::VSC) {
    if (hvdcDefinition.position == dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
      set->addParameter(helper::buildParameter("hvdc_modeU10", hvdcDefinition.converter2VoltageRegulationOn.value()));
      set->addParameter(helper::buildParameter("hvdc_modeU20", hvdcDefinition.converter1VoltageRegulationOn.value()));
      set->addReference(helper::buildReference("hvdc_Q1Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_Q2Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_U1Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_U2Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter1Id));
    } else {
      set->addParameter(helper::buildParameter("hvdc_modeU10", hvdcDefinition.converter1VoltageRegulationOn.value()));
      set->addParameter(helper::buildParameter("hvdc_modeU20", hvdcDefinition.converter2VoltageRegulationOn.value()));
      set->addReference(helper::buildReference("hvdc_Q1Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_Q2Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_U1Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_U2Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter2Id));
    }
  }
  if (hvdcDefinition.hasPQPropModel()) {
    switch (hvdcDefinition.position) {
    case dfl::algo::HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT:
      set->addReference(helper::buildReference("hvdc_QPercent1", "qMax_pu", "DOUBLE", hvdcDefinition.converter1Id));
      break;
    case dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT:
      set->addReference(helper::buildReference("hvdc_QPercent1", "qMax_pu", "DOUBLE", hvdcDefinition.converter2Id));
      break;
    case dfl::algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT:
      set->addReference(helper::buildReference("hvdc_QPercent1", "qMax_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_QPercent2", "qMax_pu", "DOUBLE", hvdcDefinition.converter2Id));
      break;
    }
  }
  if (!hvdcDefinition.hasDanglingModel()) {
    set->addReference(helper::buildReference("P1Ref_ValueIn", "p1_pu", "DOUBLE"));
  }
  if (hvdcDefinition.hasDiagramModel()) {
    set->addParameter(helper::buildParameter("hvdc_tFilter", 0.001));
  }
  if (hvdcDefinition.hasEmulationModel()) {
    set->addParameter(helper::buildParameter("acemulation_tFilter", 50.));
    auto kac = computeKAC(*hvdcDefinition.droop);  // since the model is an emulation one, the extension is defined (see algo)
    set->addParameter(helper::buildParameter("acemulation_KACEmulation", kac));
  }
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename, const boost::filesystem::path& dirname) {
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

  auto dirname_diagram = dirname;
  dirname_diagram.append(basename + constants::diagramDirectorySuffix).append(constants::diagramFilename(def.id));

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

boost::shared_ptr<parameters::ParametersSet>
Par::writeStaticVarCompensator(const algo::StaticVarCompensatorDefinition& svarc) {
  std::size_t hashId = constants::hash(svarc.id);
  std::string hashIdStr = std::to_string(hashId);
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(hashIdStr));

  set->addMacroParSet(boost::make_shared<parameters::MacroParSet>(macroParameterSetStaticCompensator_));
  double value;

  value = svarc.voltageSetPoint / svarc.VNom;
  set->addParameter(helper::buildParameter("SVarC_URef0Pu", value));
  set->addParameter(helper::buildParameter("SVarC_UNom", svarc.VNom));
  value = computeBPU(svarc.b0, svarc.VNom);
  set->addParameter(helper::buildParameter("SVarC_BShuntPu", value));
  value = computeBPU(svarc.bMax, svarc.VNom);
  set->addParameter(helper::buildParameter("SVarC_BMaxPu", value));
  value = computeBPU(svarc.bMin, svarc.VNom);
  set->addParameter(helper::buildParameter("SVarC_BMinPu", value));
  switch (svarc.model) {
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING:
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE:
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING:
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROP:
    value = svarc.slope * Sb_ / svarc.VNom;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING:
    value = svarc.slope * Sb_ / svarc.VNom;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE:
    value = svarc.slope * Sb_ / svarc.VNom;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING:
    value = svarc.slope * Sb_ / svarc.VNom;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  default:
    break;
  }

  return set;
}

boost::shared_ptr<parameters::MacroParameterSet>
Par::writeMacroParameterSetStaticVarCompensators() {
  auto macro = boost::make_shared<parameters::MacroParameterSet>(macroParameterSetStaticCompensator_);

  macro->addReference(helper::buildReference("SVarC_P0Pu", "p_pu", "DOUBLE"));
  macro->addReference(helper::buildReference("SVarC_Q0Pu", "q_pu", "DOUBLE"));
  macro->addReference(helper::buildReference("SVarC_U0Pu", "v_pu", "DOUBLE"));
  macro->addReference(helper::buildReference("SVarC_UPhase0", "angle_pu", "DOUBLE"));

  return macro;
}

}  // namespace outputs
}  // namespace dfl
