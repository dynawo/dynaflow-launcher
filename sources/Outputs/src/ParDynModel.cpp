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

#include "Log.h"
#include "ParCommon.h"

namespace dfl {
namespace outputs {

void
ParDynModel::write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                   const algo::ShuntCounterDefinitions& shuntCounters, const algo::LinesByIdDefinitions& linesByIdDefinitions) {
  const auto& sets = dynamicDataBaseManager.settingDocument().sets();
  for (const auto& set : sets) {
    auto new_set =
        writeDynamicModelParameterSet(set, dynamicDataBaseManager.assemblingDocument(), shuntCounters, dynamicModelsDefinitions_, linesByIdDefinitions);
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
ParDynModel::writeDynamicModelParameterSet(const inputs::SettingXmlDocument::Set& set, const inputs::AssemblingXmlDocument& assemblingDoc,
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
      LOG(debug, CountIdNotFound, count.id);
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
        LOG(warn, TFOComponentNotFound, ref.name, set.id);
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
      LOG(warn, RefUnsupportedTag, ref.name, ref.tag);
    }
  }
  return new_set;
}

boost::optional<std::string>
ParDynModel::getActiveSeason(const inputs::SettingXmlDocument::Ref& ref, const algo::LinesByIdDefinitions& linesById,
                             const inputs::AssemblingXmlDocument& assemblingDoc) {
  const auto& singleAssocations = assemblingDoc.singleAssociations();

  // the ref element references the single association to use
  auto foundAsso = std::find_if(singleAssocations.begin(), singleAssocations.end(),
                                [&ref](const inputs::AssemblingXmlDocument::SingleAssociation& asso) { return asso.id == ref.id; });
  if (foundAsso == singleAssocations.end()) {
    LOG(warn, SingleAssociationRefNotFound, ref.name, ref.id);
    return boost::none;
  }
  if (!foundAsso->line) {
    LOG(warn, SingleAssociationRefNotALine, ref.name, ref.id);
    return boost::none;
  }
  const auto& lineId = foundAsso->line->name;
  auto foundLine = linesById.linesMap.find(lineId);
  if (foundLine == linesById.linesMap.end()) {
    LOG(warn, RefLineNotFound, ref.name, ref.id, lineId);
    return boost::none;
  }

  return foundLine->second.activeSeason;
}

}  // namespace outputs
}  // namespace dfl
