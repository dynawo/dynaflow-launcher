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

#include <PARParametersSetCollection.h>
#include <boost/shared_ptr.hpp>

namespace dfl {
namespace outputs {

/**
 * @brief generators PAR file writer
 */
class ParGenerator {
 public:
  /**
   * @brief Construct a new Par Generators object
   *
   * @param generatorDefinitions reference to the list of load definitions
   */
  explicit ParGenerator(const std::vector<algo::GeneratorDefinition>& generatorDefinitions) : generatorDefinitions_(generatorDefinitions) {}

  /**
   * @brief enrich the parameter set collection for generators
   *
   * @param paramSetCollection parameter set collection to enrich
   * @param activePowerCompensation the type of active power compensation
   * @param basename basename for current simulation
   * @param dirname the dirname of the output PAR file
   * @param busesWithDynamicModel map of bus ids to a generator that regulates them
   * @param startingPointMode starting point mode
   */
  void write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection,
             dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
             const std::string& basename,
             const boost::filesystem::path& dirname,
             const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel,
             dfl::inputs::Configuration::StartingPointMode startingPointMode);

 private:
  /**
   * @brief Get the Generator Macro Parameter Set Id object
   *
   * @param modelType type of modelling chosen for a generator
   * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
   * @return Generator Parameter Set Id object
   */
  std::string getGeneratorMacroParameterSetId(algo::GeneratorDefinition::ModelType modelType, bool fixedP);

  /**
   * @brief Get the Generator Parameter Set Id object
   *
   * @param modelType type of modelling chosen for a generator
   * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
   * @return Generator Parameter Set Id object
   */
  std::string getGeneratorParameterSetId(algo::GeneratorDefinition::ModelType modelType, bool fixedP);

  /**
   * @brief build a generator macro parameter set
   *
   * @param modelType type of modelling chosen for a generator
   * @param activePowerCompensation the type of active power compensation
   * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
   * @param startingPointMode starting point mode
   * @return the new macro parameter set
   */
  boost::shared_ptr<parameters::MacroParameterSet> buildGeneratorMacroParameterSet(algo::GeneratorDefinition::ModelType modelType,
                                                                                   inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                                                                                   bool fixedP,
                                                                                   dfl::inputs::Configuration::StartingPointMode startingPointMode);

  /**
    * @brief Write constants parameter sets for generators
    *
    * @param activePowerCompensation the type of active power compensation
    * @param modelType type of modelling chosen for a generator
    * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
    * @param startingPointMode starting point mode
    *
    * @returns the parameter set
    */
  boost::shared_ptr<parameters::ParametersSet> writeConstantGeneratorsSets(dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                                                                           dfl::algo::GeneratorDefinition::ModelType modelType,
                                                                           bool fixedP,
                                                                           dfl::inputs::Configuration::StartingPointMode startingPointMode);

  /**
   * @brief Update parameter set with SignalN generator parameters and references
   *
   * @param modelId the model of the generator
   * @param activePowerCompensation the type of active power compensation
   * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
   * @param startingPointMode starting point mode
   *
   * @return result parameter set
   */
  boost::shared_ptr<parameters::ParametersSet> updateSignalNGenerator(const std::string& modelId,
                                                                      dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                                                                      bool fixedP,
                                                                      dfl::inputs::Configuration::StartingPointMode startingPointMode);

  /**
   * @brief Update parameter set with remote references
   *
   * @param set the parameter set to update
   */
  void updatePropParameters(boost::shared_ptr<parameters::ParametersSet> set);

  /**
   * @brief Update parameter set with transformer parameters
   *
   * @param generator the generator definition to use
   * @param set the parameter set to update
   */
  void updateTransfoParameters(const algo::GeneratorDefinition& generator, boost::shared_ptr<parameters::ParametersSet> set);

  /**
   * @brief Write generator parameter set
   *
   * @param def the generator definition to use
   * @param basename the basename for the simulation
   * @param dirname the dirname of the output directory
   *
   * @returns the parameter set
   */
  boost::shared_ptr<parameters::ParametersSet> writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename,
                                                              const boost::filesystem::path& dirname);

 private:
  std::vector<algo::GeneratorDefinition> generatorDefinitions_;  ///< list of generators definitions
};

}  // namespace outputs
}  // namespace dfl
