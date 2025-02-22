//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  ParDynModel.h
 *
 * @brief Dynaflow launcher PAR file writer for defined dynamic models header file
 *
 */

#pragma once

#include "DynModelDefinitionAlgorithm.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "OutputsConstants.h"
#include "ShuntDefinitionAlgorithm.h"
#include "TransfoDefinitionAlgorithm.h"

#include <PARParametersSetCollection.h>


namespace dfl {
namespace outputs {

/**
 * @brief dynamic models PAR file writer
 */
class ParDynModel {
 public:
  /**
   * @brief Construct a new Par Dyn Model object
   *
   * @param dynamicModelsDefinitions reference to the list of defined dynamic models definitions
   * @param gens generators definition coming from algorithms
   * @param hvdcDefinitions hvdc definitions coming from algorithms
   */
  explicit ParDynModel(const algo::DynamicModelDefinitions &dynamicModelsDefinitions, const std::vector<algo::GeneratorDefinition> &gens,
                       const algo::HVDCLineDefinitions &hvdcDefinitions);

  /**
   * @brief enrich the parameter set collection for defined dynamic models
   *
   * @param paramSetCollection parameter set collection to enrich
   * @param dynamicDataBaseManager the dynamic DB manager to use
   * @param shuntCounters the counters to use
   * @param linesByIdDefinitions lines by id to use
   * @param transformersById transformers by id to use
   */
  void write(const std::unique_ptr<parameters::ParametersSetCollection>& paramSetCollection,
              const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
              const algo::ShuntCounterDefinitions& shuntCounters,
              const algo::LinesByIdDefinitions& linesByIdDefinitions,
              const algo::TransformersByIdDefinitions& transformersById);

 private:
  /**
   * @brief Retrieves the first component connected through the dynamic model to a transformer
   *
   * @param dynModelDef the dynamic model to process
   * @returns the transformer id connected to the dynamic model, nullopt if not found
   */
  boost::optional<std::string> getTransformerComponentId(const algo::DynamicModelDefinition &dynModelDef);

  /**
   * @brief Write setting set for dynamic models
   *
   * @param set the configuration set to write
   * @param dynamicDataBaseManager the dynamic DB manager to use
   * @param automaton the current secondary voltage control automaton
   * @param counters the counters to use
   * @param models the models definitions to use
   * @param linesById lines by id to use
   * @param transformersById transformers by id to use
   *
   * @returns the parameter set to add
   */
  std::shared_ptr<parameters::ParametersSet>
  writeDynamicModelParameterSet(const inputs::SettingDataBase::Set &set, const inputs::DynamicDataBaseManager &dynamicDataBaseManager,
                                const algo::DynamicModelDefinition &automaton, const algo::ShuntCounterDefinitions &counters,
                                const algo::DynamicModelDefinitions &models, const algo::LinesByIdDefinitions &linesById,
                                const algo::TransformersByIdDefinitions &transformersById);

  /**
   * @brief Write setting set for secondary voltage controller models
   *
   * @param set the configuration set to write
   * @param dynamicDataBaseManager the dynamic DB manager to use
   * @param automaton the current secondary voltage control automaton
   *
   * @returns the parameter set to add
   */
  std::shared_ptr<parameters::ParametersSet> writeSVCParameterSet(const inputs::SettingDataBase::Set &set,
                                                                    const inputs::DynamicDataBaseManager &dynamicDataBaseManager,
                                                                    const algo::DynamicModelDefinition &automaton);

  /**
   * @brief Retrieve active season
   *
   * @param ref the Ref XML element referencing the active season
   * @param linesById Dynawo lines by id to use
   * @param transformersById transformers by id to use
   * @param dynamicDataBaseManager the dynamic DB manager to use
   *
   * @return active season value
   */
  boost::optional<std::string> getActiveSeason(const inputs::SettingDataBase::Ref &ref, const algo::LinesByIdDefinitions &linesById,
                                               const algo::TransformersByIdDefinitions &transformersById,
                                               const inputs::DynamicDataBaseManager &dynamicDataBaseManager);

 private:
  const algo::DynamicModelDefinitions &dynamicModelsDefinitions_;       ///< list of defined dynamic models
  const std::vector<algo::GeneratorDefinition> &generatorDefinitions_;  ///< list of generator definitions
  const algo::HVDCLineDefinitions &hvdcDefinitions_;                    ///< hvdc definitions
  std::unordered_map<std::string, size_t> generatorIdToIndex_;          ///< map of generator ids to their index
};

}  // namespace outputs
}  // namespace dfl
