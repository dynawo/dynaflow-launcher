//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParDynModel.h"

#include "Constants.h"
#include "Log.h"
#include "ParCommon.h"

#include <DYNCommon.h>

namespace dfl {
namespace outputs {

ParDynModel::ParDynModel(const algo::DynamicModelDefinitions& dynamicModelsDefinitions, const std::vector<algo::GeneratorDefinition>& gens) :
    dynamicModelsDefinitions_(dynamicModelsDefinitions),
    generatorDefinitions_(gens) {
  for (size_t i = 0; i < gens.size(); ++i) {
    generatorIdToIndex_[gens[i].id] = i;
  }
}

void
ParDynModel::write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                   const algo::ShuntCounterDefinitions& shuntCounters, const algo::LinesByIdDefinitions& linesByIdDefinitions) {
  for (const auto& automaton : dynamicDataBaseManager.assembling().dynamicAutomatons()) {
    boost::shared_ptr<parameters::ParametersSet> new_set = writeDynamicModelParameterSet(
        dynamicDataBaseManager.setting().getSet(automaton.first), dynamicDataBaseManager, shuntCounters, dynamicModelsDefinitions_, linesByIdDefinitions);
    if (automaton.second.lib == common::constants::svcModelName) {
      writeAdditionalSVCParameterSet(new_set, automaton.second);
    }
    if (new_set) {
      paramSetCollection->addParametersSet(new_set);
    }
  }
}

boost::optional<std::string>
ParDynModel::getTransformerComponentId(const algo::DynamicModelDefinition& dynModelDef) {
  for (const auto& macro : dynModelDef.nodeConnections) {
    if (macro.elementType == algo::DynamicModelDefinition::MacroConnection::ElementType::TFO) {
      return macro.connectedElementId;
    }
  }

  return boost::none;
}

void
ParDynModel::writeAdditionalSVCParameterSet(boost::shared_ptr<parameters::ParametersSet> paramSet,
                                            const inputs::AssemblingDataBase::DynamicAutomaton& automaton) {
  const auto& svcDefinition = dynamicModelsDefinitions_.models.find(automaton.id);
  if (svcDefinition == dynamicModelsDefinitions_.models.end()) {
    return;
  }
  unsigned idx = 1;
  paramSet->addParameter(helper::buildParameter("secondaryVoltageControl_DerLevelMaxPu", 0.085));
  for (const auto& connection : svcDefinition->second.nodeConnections) {
    const auto& generatorIdx = generatorIdToIndex_.find(connection.connectedElementId);
    if (generatorIdx != generatorIdToIndex_.end()) {
      const auto& genDefinition = generatorDefinitions_[generatorIdx->second];
      if (!genDefinition.isNetwork()) {
        paramSet->addParameter(helper::buildParameter("secondaryVoltageControl_Participate0_" + std::to_string(idx) + "_", true));
      }
      paramSet->addReference(helper::buildReference("secondaryVoltageControl_P0Pu_" + std::to_string(idx) + "_", "p_pu", "DOUBLE", genDefinition.id));
      paramSet->addReference(helper::buildReference("secondaryVoltageControl_Q0Pu_" + std::to_string(idx) + "_", "q_pu", "DOUBLE", genDefinition.id));
      paramSet->addReference(helper::buildReference("secondaryVoltageControl_U0Pu_" + std::to_string(idx) + "_", "v_pu", "DOUBLE", genDefinition.id));
      paramSet->addReference(helper::buildReference("secondaryVoltageControl_SNom_" + std::to_string(idx) + "_", "sNom", "DOUBLE", genDefinition.id));
      if (genDefinition.hasTransformer())
        paramSet->addParameter(helper::buildParameter("secondaryVoltageControl_XTfoPu_" + std::to_string(idx) + "_", constants::generatorXPuValue));
      ++idx;
    } else {
      // We assume this is a node
      paramSet->addReference(helper::buildReference("secondaryVoltageControl_Up0Pu", "Upu", "DOUBLE", connection.connectedElementId));
      paramSet->addReference(helper::buildReference("secondaryVoltageControl_UpRef0Pu", "Upu", "DOUBLE", connection.connectedElementId));
    }
  }
}

boost::shared_ptr<parameters::ParametersSet>
ParDynModel::writeDynamicModelParameterSet(const inputs::SettingDataBase::Set& set, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                                           const algo::ShuntCounterDefinitions& counters, const algo::DynamicModelDefinitions& models,
                                           const algo::LinesByIdDefinitions& linesById) {
  if (models.models.count(set.id) == 0) {
    // model is not connected : ignore corresponding set
    return nullptr;
  }

  auto new_set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(set.id));
  for (const auto& count : set.counts) {
    const auto& multipleAssociation = dynamicDataBaseManager.assembling().getMultipleAssociation(count.id);
    if (multipleAssociation.shunt) {
      if (counters.nbShunts.count(multipleAssociation.shunt->voltageLevel) == 0) {
        // case voltage level not in network, skip
        continue;
      }
      new_set->addParameter(helper::buildParameter(count.name, static_cast<int>(counters.nbShunts.at(multipleAssociation.shunt->voltageLevel))));
    }
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
    if (componentId && *componentId == constants::componentTransformerIdTag) {
      componentId = getTransformerComponentId(models.models.at(set.id));
      if (!componentId) {
        // Configuration error : references is using a TFO element while no TFO element is connected to the dynamic model
        LOG(warn, TFOComponentNotFound, ref.name, set.id);
        continue;
      }
    }
    new_set->addReference(helper::buildReference(ref.name, ref.origName, inputs::SettingDataBase::Reference::toString(ref.dataType), componentId));
  }

  for (const auto& ref : set.refs) {
    if (ref.tag == constants::seasonTag) {
      auto seasonOpt = getActiveSeason(ref, linesById, dynamicDataBaseManager);
      if (!seasonOpt) {
        continue;
      }
      new_set->addParameter(helper::buildParameter(ref.name, *seasonOpt));
    } else {
      // Unsupported case
      LOG(warn, RefUnsupportedTag, ref.name, ref.tag);
    }
  }
  return new_set;
}

boost::optional<std::string>
ParDynModel::getActiveSeason(const inputs::SettingDataBase::Ref& ref, const algo::LinesByIdDefinitions& linesById,
                             const inputs::DynamicDataBaseManager& dynamicDataBaseManager) {
  const auto& singleAssoc = dynamicDataBaseManager.assembling().getSingleAssociation(ref.id);
  if (!singleAssoc.line) {
    LOG(warn, SingleAssociationRefNotALine, ref.name, ref.id);
    return boost::none;
  }
  const auto& lineId = singleAssoc.line->name;
  auto foundLine = linesById.linesMap.find(lineId);
  if (foundLine == linesById.linesMap.end()) {
    LOG(warn, RefLineNotFound, ref.name, ref.id, lineId);
    return boost::none;
  }

  return foundLine->second.activeSeason;
}

}  // namespace outputs
}  // namespace dfl
