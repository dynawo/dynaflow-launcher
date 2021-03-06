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
 * @file  Par.h
 *
 * @brief Dynaflow launcher PAR file writer header file
 *
 */

#pragma once

#include "Algo.h"
#include "Configuration.h"

#include <PARParametersSet.h>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfl {
namespace outputs {
/**
 * @brief PAR file writer
 */
class Par {
 public:
  /**
   * @brief PAR file definition
   */
  struct ParDefinition {
    /**
     * @brief Constructor
     *
     * @param base basename for current simulation
     * @param dir the dirname of the output PAR file
     * @param filename file path for output PAR file (corresponds to basename)
     * @param gens list of the generators taken into account
     * @param hvdcLines list of hdvc lines taken into account
     * @param activePowerCompensation the type of active power compensation
     * @param busesWithDynamicModel map of bus ids to a generator that regulates them
     */
    ParDefinition(const std::string& base, const boost::filesystem::path& dir, const boost::filesystem::path& filename,
                  const std::vector<algo::GeneratorDefinition>& gens, const algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap& hvdcLines,
                  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                  const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel) :
        basename(base),
        dirname(dir),
        filepath(filename),
        generators(gens),
        hvdcLines(hvdcLines),
        activePowerCompensation(activePowerCompensation),
        busesWithDynamicModel(busesWithDynamicModel) {}

    std::string basename;                                                         ///< basename
    boost::filesystem::path dirname;                                              ///< Dirname of output file relative to execution dir
    boost::filesystem::path filepath;                                             ///< file path of the output file to write
    std::vector<algo::GeneratorDefinition> generators;                            ///< list of generators
    algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap hvdcLines;          ///< list of hvdc lines
    dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation;  ///< the type of active power compensation
    const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel;   ///< map of bus ids to a generator that regulates them
  };

  /**
   * @brief Constructor
   *
   * @param def PAR file definition
   */
  explicit Par(ParDefinition&& def);

  /**
   * @brief Export PAR file
   */
  void write();

 private:
  /**
    * @brief Write constants parameter sets for generators
    *
    * @param activePowerCompensation the type of active power compensation
    * @param modelType type of modelling chosen for a generator
    * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
    *
    * @returns the parameter set
    */
  static boost::shared_ptr<parameters::ParametersSet> writeConstantGeneratorsSets(dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                                                                                  dfl::algo::GeneratorDefinition::ModelType modelType, bool fixedP);

  /**
    * @brief Write constants parameter sets for load
    *
    * @returns the parameter set
    */
  static boost::shared_ptr<parameters::ParametersSet> writeConstantLoadsSet();

  /**
   * @brief Update parameter set with SignalN generator parameters and references
   *
   * @param modelId the model of the generator
   * @param activePowerCompensation the type of active power compensation
   * @param fixedP boolean to determine if the set represents a generator with a targetP equal to 0
   */
  static boost::shared_ptr<parameters::ParametersSet>
  updateSignalNGenerator(const std::string& modelId, dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation, bool fixedP);

  /**
   * @brief Update parameter set with remote references
   *
   * @param set the parameter set to update
   */
  static void updatePropParameters(boost::shared_ptr<parameters::ParametersSet> set);

  /**
   * @brief Write generator parameter set
   *
   * @param def the generator definition to use
   * @param basename the basename for the simulation
   * @param dirname the dirname of the output directory
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename,
                                                                     const std::string& dirname);
  /**
   * @brief Write hvdc line parameter set
   *
   * @param hvdcLine the hvdc line definition to use
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeHdvcLine(const algo::HvdcLineDefinition& hvdcLine);

  /**
   * @brief Write remote voltage regulators parameter set
   *
   * @param busId the bus id to use
   * @param genId the generator id to use
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeVRRemote(const std::string& busId, const std::string& genId);

 private:
  ParDefinition def_;                                ///< PAR file definition
  static constexpr double kGoverNullValue_ = 0.;     ///< KGover null value
  static constexpr double kGoverDefaultValue_ = 1.;  ///< KGover default value
};

}  // namespace outputs
}  // namespace dfl
