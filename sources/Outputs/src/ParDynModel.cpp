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
                   const algo::ShuntCounterDefinitions& shuntCounters, const algo::LinesByIdDefinitions& linesByIdDefinitions,
                   const algo::TransformersByIdDefinitions& transformersById) {
  for (const auto& dynModel : dynamicModelsDefinitions_.models) {
    boost::shared_ptr<parameters::ParametersSet> new_set;

    if (dynModel.second.lib == common::constants::svcModelName) {
      new_set = writeSVCParameterSet(dynamicDataBaseManager.setting().getSet(dynModel.first), dynamicDataBaseManager, dynModel.second);
    } else {
      new_set = writeDynamicModelParameterSet(dynamicDataBaseManager.setting().getSet(dynModel.first), dynamicDataBaseManager, shuntCounters,
                                              dynamicModelsDefinitions_, linesByIdDefinitions, transformersById);
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

boost::shared_ptr<parameters::ParametersSet>
ParDynModel::writeSVCParameterSet(const inputs::SettingDataBase::Set& set, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                                  const algo::DynamicModelDefinition& automaton) {
  auto new_set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(set.id));

  std::unordered_map<std::string, unsigned> genIdToInitialIndex;
  auto itAutomaton = dynamicDataBaseManager.assembling().dynamicAutomatons().find(automaton.id);
  assert(itAutomaton != dynamicDataBaseManager.assembling().dynamicAutomatons().end());
  unsigned genIndex = 1;
  for (const auto& macroConn : itAutomaton->second.macroConnects) {
    if (dynamicDataBaseManager.assembling().isSingleAssociation(macroConn.id)) {
      for (const auto& gen : dynamicDataBaseManager.assembling().getSingleAssociation(macroConn.id).generators) {
        genIdToInitialIndex[gen.name] = genIndex;
      }
      if (!dynamicDataBaseManager.assembling().getSingleAssociation(macroConn.id).generators.empty())
        ++genIndex;
    }
  }

  std::unordered_map<std::string, double> genInitialParamToValues;
  for (const auto& param : set.doubleParameters) {
    if (param.name == "secondaryVoltageControl_Alpha" || param.name == "secondaryVoltageControl_Beta") {
      new_set->addParameter(helper::buildParameter(param.name, param.value));
    } else if (param.name.find("secondaryVoltageControl_Qr_") != std::string::npos) {
      genInitialParamToValues[param.name] = param.value;
    } else if (param.name.find("secondaryVoltageControl_SNom_") != std::string::npos) {
      genInitialParamToValues[param.name] = param.value;
    }
  }

  unsigned idx = 1;
  new_set->addParameter(helper::buildParameter("secondaryVoltageControl_DerLevelMaxPu", 0.085));
  new_set->addParameter(helper::buildParameter("secondaryVoltageControl_FreezingActivated", true));
  bool frozen = true;
  for (const auto& connection : automaton.nodeConnections) {
    const auto& generatorIdx = generatorIdToIndex_.find(connection.connectedElementId);
    if (generatorIdx != generatorIdToIndex_.end()) {
      const auto& genDefinition = generatorDefinitions_[generatorIdx->second];
      if (!genDefinition.isNetwork()) {
        new_set->addParameter(helper::buildParameter("secondaryVoltageControl_Participate0_" + std::to_string(idx) + "_", true));
      }

      auto it = genIdToInitialIndex.find(genDefinition.id);
      if (it != genIdToInitialIndex.end()) {
        new_set->addParameter(helper::buildParameter("secondaryVoltageControl_Qr_" + std::to_string(idx) + "_",
                                                     genInitialParamToValues["secondaryVoltageControl_Qr_" + std::to_string(it->second) + "_"]));
        auto sNomIt = genInitialParamToValues.find("secondaryVoltageControl_SNom_" + std::to_string(it->second) + "_");
        if (sNomIt != genInitialParamToValues.end()) {
          new_set->addParameter(helper::buildParameter("secondaryVoltageControl_SNom_" + std::to_string(idx) + "_", sNomIt->second));
        } else {
          new_set->addReference(helper::buildReference("secondaryVoltageControl_SNom_" + std::to_string(idx) + "_", "sNom", "DOUBLE", genDefinition.id));
        }
      }

      new_set->addReference(helper::buildReference("secondaryVoltageControl_P0Pu_" + std::to_string(idx) + "_", "p_pu", "DOUBLE", genDefinition.id));
      new_set->addReference(helper::buildReference("secondaryVoltageControl_Q0Pu_" + std::to_string(idx) + "_", "q_pu", "DOUBLE", genDefinition.id));
      new_set->addReference(helper::buildReference("secondaryVoltageControl_U0Pu_" + std::to_string(idx) + "_", "v_pu", "DOUBLE", genDefinition.id));
      if (genDefinition.hasTransformer())
        new_set->addParameter(helper::buildParameter("secondaryVoltageControl_XTfoPu_" + std::to_string(idx) + "_",
                                                     (genDefinition.isNuclear) ? constants::generatorNucXPuValue : constants::generatorXPuValue));

      if (genDefinition.q < genDefinition.qmax && genDefinition.q > genDefinition.qmin) {
        frozen = false;
      }
      ++idx;
    } else {
      // We assume this is a node
      new_set->addReference(helper::buildReference("secondaryVoltageControl_Up0Pu", "Upu", "DOUBLE", connection.connectedElementId));
      new_set->addReference(helper::buildReference("secondaryVoltageControl_UpRef0Pu", "Upu", "DOUBLE", connection.connectedElementId));
    }
  }
  if (frozen)
    new_set->addParameter(helper::buildParameter("secondaryVoltageControl_Frozen0", true));
  return new_set;
}

boost::shared_ptr<parameters::ParametersSet>
ParDynModel::writeDynamicModelParameterSet(const inputs::SettingDataBase::Set& set, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                                           const algo::ShuntCounterDefinitions& counters, const algo::DynamicModelDefinitions& models,
                                           const algo::LinesByIdDefinitions& linesById, const algo::TransformersByIdDefinitions& tfosById) {
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
      auto seasonOpt = getActiveSeason(ref, linesById, tfosById, dynamicDataBaseManager);
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
                             const algo::TransformersByIdDefinitions& tfosById, const inputs::DynamicDataBaseManager& dynamicDataBaseManager) {
  const auto& singleAssoc = dynamicDataBaseManager.assembling().getSingleAssociation(ref.id);
  if (!singleAssoc.line && !singleAssoc.tfo) {
    LOG(warn, SingleAssociationRefIncorrectType, ref.name, ref.id);
    return boost::none;
  }
  if (singleAssoc.line) {
    const auto& lineId = singleAssoc.line->name;
    auto foundLine = linesById.linesMap.find(lineId);
    if (foundLine == linesById.linesMap.end()) {
      LOG(warn, RefDeviceNotFound, ref.name, ref.id, lineId);
      return boost::none;
    }

    return foundLine->second.activeSeason;
  } else {
    const auto& tfoId = singleAssoc.tfo->name;
    auto found = tfosById.tfosMap.find(tfoId);
    if (found == tfosById.tfosMap.end()) {
      LOG(warn, RefDeviceNotFound, ref.name, ref.id, tfoId);
      return boost::none;
    }

    return found->second.activeSeason;
  }
}

}  // namespace outputs
}  // namespace dfl
