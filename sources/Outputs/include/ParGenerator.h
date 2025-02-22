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
 * @file  ParGenerator.h
 *
 * @brief Dynaflow launcher PAR file writer for generators header file
 *
 */

#pragma once

#include "Configuration.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "OutputsConstants.h"

#include <DYNCommon.h>
#include <PARParametersSetCollection.h>


namespace dfl {
namespace outputs {

/**
 * @brief generators PAR file writer
 */
class ParGenerator {
 public:
  using ActivePowerCompensation = dfl::inputs::Configuration::ActivePowerCompensation;  ///< Alias for type of active power compensation
  using StartingPointMode = dfl::inputs::Configuration::StartingPointMode;              ///< Alias for starting point mode
  using ModelType = dfl::algo::GeneratorDefinition::ModelType;                          ///< Alias for model type
  /**
   * @brief Construct a new Par Generators object
   *
   * @param generatorDefinitions reference to the list of generator definitions
   */
  explicit ParGenerator(const std::vector<algo::GeneratorDefinition> &generatorDefinitions) : generatorDefinitions_(generatorDefinitions) {}

  /**
   * @brief enrich the parameter set collection for generators
   *
   * @param paramSetCollection parameter set collection to enrich
   * @param activePowerCompensation the type of active power compensation
   * @param basename basename for current simulation
   * @param dirname the dirname of the output PAR file
   * @param startingPointMode starting point mode
   * @param dynamicDataBaseManager the dynamic DB manager to use
   */
  void write(const std::unique_ptr<parameters::ParametersSetCollection>& paramSetCollection,
              ActivePowerCompensation activePowerCompensation,
              const std::string& basename,
              const boost::filesystem::path& dirname,
              StartingPointMode startingPointMode,
              const inputs::DynamicDataBaseManager& dynamicDataBaseManager);

 private:
  /**
   * @brief Get the Generator Macro Parameter Set Id object
   *
   * @param modelType type of modelling chosen for a generator
   * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
   * @return Generator Parameter Set Id object
   */
  std::string getGeneratorMacroParameterSetId(ModelType modelType, bool fixedP);

  /**
   * @brief build a generator macro parameter set
   *
   * @param def the generator definition to use
   * @param activePowerCompensation the type of active power compensation
   * @param targetP generator targetP value
   * @param startingPointMode starting point mode
   * @return the new macro parameter set
   */
  std::unique_ptr<parameters::MacroParameterSet> buildGeneratorMacroParameterSet(const algo::GeneratorDefinition& def,
                                                                                  ActivePowerCompensation activePowerCompensation,
                                                                                  double targetP,
                                                                                  StartingPointMode startingPointMode);

  /**
   * @brief Write constants parameter sets for generators
   *
   * @param activePowerCompensation the type of active power compensation
   * @param generator generator definition
   * @param startingPointMode starting point mode
   *
   * @returns the parameter set
   */
  std::shared_ptr<parameters::ParametersSet> writeConstantGeneratorsSets(ActivePowerCompensation activePowerCompensation,
                                                                         const algo::GeneratorDefinition &generator, StartingPointMode startingPointMode);

  /**
   * @brief Update parameter set with SignalN generator parameters and references
   *
   * @param set the parameter set to update
   * @param activePowerCompensation the type of active power compensation
   * @param targetP generator targetP value
   * @param startingPointMode starting point mode
   * @param hasActivePowerControl true if the generator has active power control information
   */
  void updateSignalNGenerator(std::shared_ptr<parameters::ParametersSet> set, dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                              double targetP, StartingPointMode startingPointMode, bool hasActivePowerControl);

  /**
   * @brief Update parameter set with transformer parameters
   *
   * @param set the parameter set to update
   * @param isNuclear true if the energy source of the generator is nuclear
   */
  void updateTransfoParameters(std::shared_ptr<parameters::ParametersSet> set, bool isNuclear);

  /**
   * @brief Update parameter set with Rpcl parameters
   *
   * @param set the parameter set to update
   * @param genId the current generator id
   * @param databaseSetting the settings found in setting file
   * @param Rcpl2 true if the model used is RPCL2, false otherwise
   */
  void updateRpclParameters(std::shared_ptr<parameters::ParametersSet> set, const std::string &genId, const inputs::SettingDataBase::Set &databaseSetting,
                            bool Rcpl2);

  /**
   * @brief Write generator parameter set
   *
   * @param def the generator definition to use
   * @param basename the basename for the simulation
   * @param dirname the dirname of the output directory
   *
   * @returns the parameter set
   */
  std::shared_ptr<parameters::ParametersSet> writeGenerator(const algo::GeneratorDefinition &def, const std::string &basename,
                                                            const boost::filesystem::path &dirname);

  /**
   * @brief set the kGover value based on if generator has active power control and targetP value
   *
   * @param set the parameter set to update
   * @param hasActivePowerControl if the generator has active power control information
   * @param targetP generator targetP value
   */
  template <class T> void setKGover(T &set, const bool hasActivePowerControl, const double targetP);

  /**
   * @brief update a parameter set with information specific to remote voltage regulation for a generator
   *        and that cannot be included in a macroParameter
   *
   * @param def the generator definition to use
   * @param set the parameter set to be updated
   */
  void updateRemoteRegulationParameters(const algo::GeneratorDefinition &def, std::shared_ptr<parameters::ParametersSet> set);

 private:
  const std::vector<algo::GeneratorDefinition> &generatorDefinitions_;  ///< list of generators definitions
};

}  // namespace outputs
}  // namespace dfl
