//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParGenerator.h"

#include "ParCommon.h"

#include <DYNCommon.h>

namespace dfl {
namespace outputs {

void
ParGenerator::write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection, ActivePowerCompensation activePowerCompensation,
                    const std::string& basename, const boost::filesystem::path& dirname,
                    const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel, StartingPointMode startingPointMode) {
  for (const auto& generator : generatorDefinitions_) {
    // if network model, nothing to do
    if (generator.isNetwork()) {
      continue;
    }
    boost::shared_ptr<parameters::ParametersSet> paramSet;
    if (!generator.isUsingDiagram()) {
      if (!paramSetCollection->hasParametersSet(getGeneratorParameterSetId(generator.model, DYN::doubleIsZero(generator.targetP)))) {
        paramSet = writeConstantGeneratorsSets(activePowerCompensation, generator.model, DYN::doubleIsZero(generator.targetP), startingPointMode);
      }
    } else {
      // we check if the macroParameterSet need by generator model is not already created. If not, we create a new one
      if (!paramSetCollection->hasMacroParametersSet(getGeneratorMacroParameterSetId(generator.model, DYN::doubleIsZero(generator.targetP)))) {
        paramSetCollection->addMacroParameterSet(
            buildGeneratorMacroParameterSet(generator.model, activePowerCompensation, DYN::doubleIsZero(generator.targetP), startingPointMode));
      }
      // if generator is not using infinite diagrams, no need to create constant sets
      paramSet = writeGenerator(generator, basename, dirname);
    }

    // common to any type of generator model, except the network one
    if (paramSet && generator.hasTransformer()) {
      updateTransfoParameters(generator, paramSet);
    }
    if (paramSet && !paramSetCollection->hasParametersSet(paramSet->getId())) {
      paramSetCollection->addParametersSet(paramSet);
    }
  }
  // adding parameters sets related to remote voltage control or multiple generator regulating same bus
  for (const auto& keyValue : busesWithDynamicModel) {
    if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    paramSetCollection->addParametersSet(helper::writeVRRemote(keyValue.first, keyValue.second));
  }
}

std::string
ParGenerator::getGeneratorMacroParameterSetId(ModelType modelType, bool fixedP) {
  std::string id;
  switch (modelType) {
  case ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? helper::getMacroParameterSetId(constants::propSignalNGeneratorFixedPParId)
                : helper::getMacroParameterSetId(constants::propSignalNGeneratorParId);
    break;
  case ModelType::PROP_SIGNALN_RECTANGULAR:
    id = fixedP ? helper::getMacroParameterSetId(constants::propSignalNGeneratorFixedPParIdRect)
                : helper::getMacroParameterSetId(constants::propSignalNGeneratorParIdRect);
    break;
  case ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? helper::getMacroParameterSetId(constants::remoteSignalNGeneratorFixedP) : helper::getMacroParameterSetId(constants::remoteVControlParId);
    break;
  case ModelType::REMOTE_SIGNALN_RECTANGULAR:
    id = fixedP ? helper::getMacroParameterSetId(constants::remoteSignalNGeneratorFixedPRect)
                : helper::getMacroParameterSetId(constants::remoteVControlParIdRect);
    break;
  case ModelType::SIGNALN_RECTANGULAR:
    id = fixedP ? helper::getMacroParameterSetId(constants::signalNGeneratorFixedPParIdRect)
                : helper::getMacroParameterSetId(constants::signalNGeneratorParIdRect);
    break;
  default:
    id = fixedP ? helper::getMacroParameterSetId(constants::signalNGeneratorFixedPParId) : helper::getMacroParameterSetId(constants::signalNGeneratorParId);
    break;
  }
  return id;
}

std::string
ParGenerator::getGeneratorParameterSetId(ModelType modelType, bool fixedP) {
  std::string id;
  switch (modelType) {
  case ModelType::PROP_SIGNALN_INFINITE:
  case ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? constants::propSignalNGeneratorFixedPParId : constants::propSignalNGeneratorParId;
    break;
  case ModelType::REMOTE_SIGNALN_INFINITE:
  case ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
    id = fixedP ? constants::remoteSignalNGeneratorFixedP : constants::remoteVControlParId;
    break;
  case ModelType::SIGNALN_TFO_INFINITE:
    id = constants::signalNTfoGeneratorParId;
    break;
  default:
    id = fixedP ? constants::signalNGeneratorFixedPParId : constants::signalNGeneratorParId;
    break;
  }
  return id;
}

boost::shared_ptr<parameters::MacroParameterSet>
ParGenerator::buildGeneratorMacroParameterSet(ModelType modelType, ActivePowerCompensation activePowerCompensation, bool fixedP,
                                              StartingPointMode startingPointMode) {
  boost::shared_ptr<parameters::MacroParameterSet> macroParameterSet =
      boost::shared_ptr<parameters::MacroParameterSet>(new parameters::MacroParameterSet(getGeneratorMacroParameterSetId(modelType, fixedP)));

  macroParameterSet->addReference(helper::buildReference("generator_PMin", "pMin", "DOUBLE"));
  macroParameterSet->addReference(helper::buildReference("generator_PMax", "pMax", "DOUBLE"));

  switch (startingPointMode) {
  case StartingPointMode::WARM:
    macroParameterSet->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));
    break;
  case StartingPointMode::FLAT:
    macroParameterSet->addReference(helper::buildReference("generator_P0Pu", "targetP_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_Q0Pu", "targetQ_pu", "DOUBLE"));
    macroParameterSet->addParameter(helper::buildParameter("generator_U0Pu", 1.0));
    macroParameterSet->addParameter(helper::buildParameter("generator_UPhase0", 0.));
    break;
  }

  macroParameterSet->addReference(helper::buildReference("generator_PRef0Pu", "targetP_pu", "DOUBLE"));
  macroParameterSet->addParameter(helper::buildParameter("generator_tFilter", 0.001));

  macroParameterSet->addParameter(helper::buildParameter("generator_KGover", getKGoverValue(fixedP)));

  switch (activePowerCompensation) {
  case ActivePowerCompensation::P:
    macroParameterSet->addReference(helper::buildReference("generator_PNom", "p_pu", "DOUBLE"));
    break;
  case ActivePowerCompensation::TARGET_P:
    macroParameterSet->addReference(helper::buildReference("generator_PNom", "targetP_pu", "DOUBLE"));
    break;
  case ActivePowerCompensation::PMAX:
    macroParameterSet->addReference(helper::buildReference("generator_PNom", "pMax_pu", "DOUBLE"));
    break;
  }

  switch (modelType) {
  case ModelType::PROP_SIGNALN_INFINITE:
  case ModelType::PROP_DIAGRAM_PQ_SIGNALN:
    macroParameterSet->addReference(helper::buildReference("generator_QRef0Pu", "targetQ_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QPercent", "qMax_pu", "DOUBLE"));
    break;
  case ModelType::REMOTE_SIGNALN_INFINITE:
  case ModelType::REMOTE_DIAGRAM_PQ_SIGNALN:
    macroParameterSet->addReference(helper::buildReference("generator_URef0", "targetV", "DOUBLE"));
    break;
  case ModelType::SIGNALN_RECTANGULAR:
    macroParameterSet->addReference(helper::buildReference("generator_QMin", "qMin", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QMax", "qMax", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
    break;
  case ModelType::PROP_SIGNALN_RECTANGULAR:
    macroParameterSet->addReference(helper::buildReference("generator_QRef0Pu", "targetQ_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QPercent", "qMax_pu", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QMin", "qMin", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QMax", "qMax", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
    break;
  case ModelType::REMOTE_SIGNALN_RECTANGULAR:
    macroParameterSet->addReference(helper::buildReference("generator_QMin", "qMin", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_QMax", "qMax", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_URef0", "targetV", "DOUBLE"));
    macroParameterSet->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
    break;
  default:
    macroParameterSet->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
    break;
  }
  return macroParameterSet;
}

boost::shared_ptr<parameters::ParametersSet>
ParGenerator::updateSignalNGenerator(const std::string& modelId, ActivePowerCompensation activePowerCompensation, bool fixedP,
                                     StartingPointMode startingPointMode) {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(modelId));
  set->addParameter(helper::buildParameter("generator_KGover", getKGoverValue(fixedP)));
  set->addParameter(helper::buildParameter("generator_QMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_QMax", constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMax", constants::powerValueMax));

  switch (activePowerCompensation) {
  case ActivePowerCompensation::P:
  case ActivePowerCompensation::PMAX:
    set->addReference(helper::buildReference("generator_PNom", "p_pu", "DOUBLE"));
    break;
  case ActivePowerCompensation::TARGET_P:
    set->addReference(helper::buildReference("generator_PNom", "targetP_pu", "DOUBLE"));
    break;
  default:  //  impossible by definition of the enum
    break;
  }

  switch (startingPointMode) {
  case StartingPointMode::WARM:
    set->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
    set->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
    set->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
    set->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));
    break;
  case StartingPointMode::FLAT:
    set->addReference(helper::buildReference("generator_P0Pu", "targetP_pu", "DOUBLE"));
    set->addReference(helper::buildReference("generator_Q0Pu", "targetQ_pu", "DOUBLE"));
    set->addParameter(helper::buildParameter("generator_U0Pu", 1.0));
    set->addParameter(helper::buildParameter("generator_UPhase0", 0.));
    break;
  }

  set->addReference(helper::buildReference("generator_PRef0Pu", "targetP_pu", "DOUBLE"));
  if (modelId == constants::remoteVControlParId) {
    set->addReference(helper::buildReference("generator_URef0", "targetV", "DOUBLE"));
  } else if (modelId != constants::propSignalNGeneratorParId) {
    set->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
  }
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
ParGenerator::writeConstantGeneratorsSets(ActivePowerCompensation activePowerCompensation, ModelType modelType, bool fixedP,
                                          StartingPointMode startingPointMode) {
  auto set = updateSignalNGenerator(getGeneratorParameterSetId(modelType, fixedP), activePowerCompensation, fixedP, startingPointMode);
  switch (modelType) {
  case ModelType::PROP_SIGNALN_INFINITE:
  case ModelType::PROP_DIAGRAM_PQ_SIGNALN:
  case ModelType::PROP_SIGNALN_RECTANGULAR:
    set->addReference(helper::buildReference("generator_QRef0Pu", "targetQ_pu", "DOUBLE"));
    set->addReference(helper::buildReference("generator_QPercent", "qMax_pu", "DOUBLE"));
    break;
  default:
    break;
  }
  return set;
}

boost::shared_ptr<parameters::ParametersSet>
ParGenerator::writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename, const boost::filesystem::path& dirname) {
  std::size_t hashId = constants::hash(def.id);
  std::string hashIdStr = std::to_string(hashId);

  //  Use the hash id in exported files to prevent use of non-ascii characters
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(hashIdStr));
  // The macroParSet is associated to a macroParameterSet via the id
  set->addMacroParSet(
      boost::shared_ptr<parameters::MacroParSet>(new parameters::MacroParSet(getGeneratorMacroParameterSetId(def.model, DYN::doubleIsZero(def.targetP)))));

  // Qmax and QMin are determined in dynawo according to reactive capabilities curves and min max
  // we need a small numerical tolerance in case the starting point of the reactive injection is exactly
  // on the limit of the reactive capability curve
  if (def.hasTransformer() && (def.isUsingRectangularDiagram())) {
    set->addParameter(helper::buildParameter("generator_QMin", def.qmin - 1));
    set->addParameter(helper::buildParameter("generator_QMax", def.qmax + 1));
  } else {
    set->addParameter(helper::buildParameter("generator_QMin0", def.qmin - 1));
    set->addParameter(helper::buildParameter("generator_QMax0", def.qmax + 1));
  }

  if (!def.isUsingRectangularDiagram()) {
    auto dirname_diagram = dirname;
    dirname_diagram.append(basename + constants::diagramDirectorySuffix).append(constants::diagramFilename(def.id));

    set->addParameter(helper::buildParameter("generator_QMaxTableFile", dirname_diagram.generic_string()));
    set->addParameter(helper::buildParameter("generator_QMaxTableName", hashIdStr + constants::diagramMaxTableSuffix));
    set->addParameter(helper::buildParameter("generator_QMinTableFile", dirname_diagram.generic_string()));
    set->addParameter(helper::buildParameter("generator_QMinTableName", hashIdStr + constants::diagramMinTableSuffix));
  }
  return set;
}

void
ParGenerator::updateTransfoParameters(const algo::GeneratorDefinition& generator, boost::shared_ptr<parameters::ParametersSet> set) {
  // TODO(rosiereflo) We assume here that QNom = QMax which might not be always true ... We should use the QMax of the diagram, not the iidm one.
  set->addReference(helper::buildReference("generator_QNomAlt", "qMax", "DOUBLE"));
  set->addParameter(helper::buildParameter("generator_SNom", sqrt(generator.pmax * generator.pmax + generator.qmax * generator.qmax)));
  set->addParameter(helper::buildParameter("generator_RTfoPu", 0.0029));
  set->addParameter(helper::buildParameter("generator_rTfoPu", 0.9535));
  set->addParameter(helper::buildParameter("generator_XTfoPu", 0.1228));
}

}  // namespace outputs
}  // namespace dfl
