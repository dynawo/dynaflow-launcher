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

#include "Constants.h"
#include "Log.h"
#include "ParCommon.h"

namespace dfl {
namespace outputs {

void ParGenerator::write(boost::shared_ptr<parameters::ParametersSetCollection> &paramSetCollection, ActivePowerCompensation activePowerCompensation,
                         const std::string &basename, const boost::filesystem::path &dirname, StartingPointMode startingPointMode,
                         const inputs::DynamicDataBaseManager &dynamicDataBaseManager) {
  for (const auto &generator : generatorDefinitions_) {
    // if network model, nothing to do
    if (generator.isNetwork()) {
      continue;
    }
    std::shared_ptr<parameters::ParametersSet> paramSet;
    if (helper::generatorSharesParId(generator)) {
      if (!paramSetCollection->hasParametersSet(helper::getGeneratorParameterSetId(generator))) {
        paramSet = writeConstantGeneratorsSets(activePowerCompensation, generator, startingPointMode);
      }
    } else {
      // we check if the macroParameterSet need by generator model is not already created. If not, we create a new one
      if (!paramSetCollection->hasMacroParametersSet(getGeneratorMacroParameterSetId(generator.model, DYN::doubleIsZero(generator.targetP)))) {
        paramSetCollection->addMacroParameterSet(buildGeneratorMacroParameterSet(generator, activePowerCompensation, generator.targetP, startingPointMode));
      }
      // if generator is not using infinite diagrams, no need to create constant sets
      paramSet = writeGenerator(generator, basename, dirname);
    }

    if (paramSet && generator.hasRpcl()) {
      updateRpclParameters(paramSet, generator.id,
                           dynamicDataBaseManager.setting().getSet(dynamicDataBaseManager.assembling().getSingleAssociationFromGenerator(generator.id)),
                           generator.hasRpcl2());
      if (!generator.isUsingDiagram()) {
        updateSignalNGenerator(paramSet, activePowerCompensation, generator.targetP, startingPointMode);
      }
    }
    if (paramSet && generator.hasTransformer()) {
      updateTransfoParameters(paramSet, generator.isNuclear);
    }

    // adding parameter specific to remote voltage regulation for a generator and that cannot be included in a macroParameter
    if (paramSet && generator.isRegulatingRemotely()) {
      updateRemoteRegulationParameters(generator, paramSet);
    }
    if (paramSet && !paramSetCollection->hasParametersSet(paramSet->getId())) {
      paramSetCollection->addParametersSet(paramSet);
    }
  }
}

std::string ParGenerator::getGeneratorMacroParameterSetId(ModelType modelType, bool fixedP) {
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
  case ModelType::SIGNALN_TFO_RECTANGULAR:
  case ModelType::SIGNALN_TFO_RPCL_RECTANGULAR:
  case ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR:
  case ModelType::SIGNALN_RPCL_RECTANGULAR:
  case ModelType::SIGNALN_RPCL2_RECTANGULAR:
    id = fixedP ? helper::getMacroParameterSetId(constants::signalNGeneratorFixedPParIdRect)
                : helper::getMacroParameterSetId(constants::signalNGeneratorParIdRect);
    break;
  default:
    id = fixedP ? helper::getMacroParameterSetId(constants::signalNGeneratorFixedPParId) : helper::getMacroParameterSetId(constants::signalNGeneratorParId);
    break;
  }
  return id;
}

boost::shared_ptr<parameters::MacroParameterSet> ParGenerator::buildGeneratorMacroParameterSet(const algo::GeneratorDefinition &def,
                                                                                               ActivePowerCompensation activePowerCompensation, double targetP,
                                                                                               StartingPointMode startingPointMode) {
  boost::shared_ptr<parameters::MacroParameterSet> macroParameterSet = boost::shared_ptr<parameters::MacroParameterSet>(
      new parameters::MacroParameterSet(getGeneratorMacroParameterSetId(def.model, DYN::doubleIsZero(targetP))));

  if (def.isUsingDiagram()) {
    // Otherwise was already added by updateSignalNGenerator
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
    macroParameterSet->addParameter(helper::buildParameter("generator_KGover", getKGoverValue(targetP)));

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

    if (!def.isUsingRectangularDiagram()) {
      macroParameterSet->addReference(helper::buildReference("generator_QMin0", "qMin", "DOUBLE"));
      macroParameterSet->addReference(helper::buildReference("generator_QMax0", "qMax", "DOUBLE"));
    }
  }

  if (!def.hasRpcl() || def.isUsingDiagram()) {
    macroParameterSet->addParameter(helper::buildParameter("generator_QDeadBandPu", 0.0001));
    macroParameterSet->addParameter(helper::buildParameter("generator_UDeadBandPu", 0.0001));
  }
  switch (def.model) {
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
  case ModelType::SIGNALN_TFO_RECTANGULAR:
  case ModelType::SIGNALN_TFO_RPCL_RECTANGULAR:
  case ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR:
  case ModelType::SIGNALN_RPCL_RECTANGULAR:
  case ModelType::SIGNALN_RPCL2_RECTANGULAR:
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
  case ModelType::SIGNALN_RPCL_INFINITE:
  case ModelType::SIGNALN_RPCL2_INFINITE:
  case ModelType::SIGNALN_TFO_RPCL_INFINITE:
  case ModelType::SIGNALN_TFO_RPCL2_INFINITE:
    break;
  default:
    macroParameterSet->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
    break;
  }
  return macroParameterSet;
}

void ParGenerator::updateSignalNGenerator(std::shared_ptr<parameters::ParametersSet> set, ActivePowerCompensation activePowerCompensation, double targetP,
                                          StartingPointMode startingPointMode) {
  set->addParameter(helper::buildParameter("generator_KGover", getKGoverValue(targetP)));
  set->addParameter(helper::buildParameter("generator_QMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_QMax", constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMax", constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_QDeadBandPu", 0.0001));
  set->addParameter(helper::buildParameter("generator_UDeadBandPu", 0.0001));

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
  if (set->getId() == constants::remoteVControlParId || set->getId() == constants::remoteVControlNucParId) {
    set->addReference(helper::buildReference("generator_URef0", "targetV", "DOUBLE"));
  } else if (set->getId() != constants::propSignalNGeneratorParId) {
    set->addReference(helper::buildReference("generator_URef0Pu", "targetV_pu", "DOUBLE"));
  }
}

std::shared_ptr<parameters::ParametersSet> ParGenerator::writeConstantGeneratorsSets(ActivePowerCompensation activePowerCompensation,
                                                                                     const algo::GeneratorDefinition &generator,
                                                                                     StartingPointMode startingPointMode) {
  auto set = parameters::ParametersSetFactory::newParametersSet(helper::getGeneratorParameterSetId(generator));
  updateSignalNGenerator(set, activePowerCompensation, generator.targetP, startingPointMode);
  switch (generator.model) {
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

void ParGenerator::updateRemoteRegulationParameters(const algo::GeneratorDefinition &def, std::shared_ptr<parameters::ParametersSet> set) {
  set->addReference(helper::buildReference("generator_URegulated0", "U", "DOUBLE", def.regulatedBusId));
}

std::shared_ptr<parameters::ParametersSet> ParGenerator::writeGenerator(const algo::GeneratorDefinition &def, const std::string &basename,
                                                                        const boost::filesystem::path &dirname) {
  std::string uuid = constants::uuid(def.id);

  //  Use the hash id in exported files to prevent use of non-ascii characters
  auto set = parameters::ParametersSetFactory::newParametersSet(uuid);
  // The macroParSet is associated to a macroParameterSet via the id
  set->addMacroParSet(
      boost::shared_ptr<parameters::MacroParSet>(new parameters::MacroParSet(getGeneratorMacroParameterSetId(def.model, DYN::doubleIsZero(def.targetP)))));

  if (!def.isUsingRectangularDiagram()) {
    auto dirname_diagram = dirname;
    dirname_diagram.append(basename + common::constants::diagramDirectorySuffix).append(constants::diagramFilename(def.id));

    set->addParameter(helper::buildParameter("generator_QMaxTableFile", dirname_diagram.generic_string()));
    set->addParameter(helper::buildParameter("generator_QMaxTableName", uuid + constants::diagramMaxTableSuffix));
    set->addParameter(helper::buildParameter("generator_QMinTableFile", dirname_diagram.generic_string()));
    set->addParameter(helper::buildParameter("generator_QMinTableName", uuid + constants::diagramMinTableSuffix));
  }
  return set;
}

void ParGenerator::updateTransfoParameters(std::shared_ptr<parameters::ParametersSet> set, bool isNuclear) {
  if (!set->hasParameter("generator_QNomAlt") && !set->hasReference("generator_QNomAlt"))
    set->addReference(helper::buildReference("generator_QNomAlt", "qNom", "DOUBLE"));
  if (!set->hasParameter("generator_SNom") && !set->hasReference("generator_SNom"))
    set->addReference(helper::buildReference("generator_SNom", "sNom", "DOUBLE"));
  if (isNuclear) {
    set->addParameter(helper::buildParameter("generator_XTfoPu", constants::generatorNucXPuValue));
  } else {
    set->addParameter(helper::buildParameter("generator_XTfoPu", constants::generatorXPuValue));
  }
}

void ParGenerator::updateRpclParameters(std::shared_ptr<parameters::ParametersSet> set, const std::string &genId,
                                        const inputs::SettingDataBase::Set &databaseSetting, bool Rcpl2) {
  std::vector<std::string> parameters = {"reactivePowerControlLoop_QrPu"};
  if (Rcpl2) {
    parameters.push_back("reactivePowerControlLoop_CqMaxPu");
    parameters.push_back("reactivePowerControlLoop_DeltaURefMaxPu");
    parameters.push_back("reactivePowerControlLoop_Tech");
    parameters.push_back("reactivePowerControlLoop_Ti");
  } else {
    parameters.push_back("reactivePowerControlLoop_DerURefMaxPu");
    parameters.push_back("reactivePowerControlLoop_TiQ");
  }
  for (auto parameter : parameters) {
    auto paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                                [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter; });
    if (paramIt != databaseSetting.doubleParameters.end())
      set->addParameter(helper::buildParameter(parameter, paramIt->value));
    else
      throw Error(MissingGeneratorHvdcParameterInSettings, parameter, genId);
  }
  std::unordered_map<std::string, std::string> parameterOrReference = {{"generator_QNomAlt", "qNom"}, {"generator_SNom", "sNom"}};
  for (auto parameter : parameterOrReference) {
    auto paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                                [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter.first; });
    if (paramIt != databaseSetting.doubleParameters.end())
      set->addParameter(helper::buildParameter(parameter.first, paramIt->value));
    else if (!set->hasParameter(parameter.first) && !set->hasReference(parameter.first))
      set->addReference(helper::buildReference(parameter.first, parameter.second, "DOUBLE"));
  }
}

}  // namespace outputs
}  // namespace dfl
