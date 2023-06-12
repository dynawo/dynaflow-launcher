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

#include "Configuration.h"
#include "DynModelDefinitionAlgorithm.h"
#include "DynamicDataBaseManager.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "LoadDefinitionAlgorithm.h"
#include "ParDynModel.h"
#include "ParGenerator.h"
#include "ParHvdc.h"
#include "ParLoads.h"
#include "ParSVarC.h"
#include "ParVRRemote.h"
#include "SVarCDefinitionAlgorithm.h"
#include "ShuntDefinitionAlgorithm.h"

#include <string>
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
     * @param config configuration
     * @param filename file path for output PAR file (corresponds to basename)
     * @param gens list of the generators taken into account
     * @param hvdcDefinitions HVDC lines definitions
     * @param busesRegulatedBySeveralGenerators map of bus ids to a generator that regulates them
     * @param dynamicDataBaseManager dynamic database manager to use
     * @param counters the counters definitions to use
     * @param models list of dynamic models definitions
     * @param linesById the lines to use
     * @param tfosById the transformers to use
     * @param svarcsDefinitions the SVarC definitions to use
     * @param loadsDefinitions the loads definitions to use
     */
    ParDefinition(const std::string &base, const inputs::Configuration &config, const boost::filesystem::path &filename,
                  const std::vector<algo::GeneratorDefinition> &gens, const algo::HVDCLineDefinitions &hvdcDefinitions,
                  const algo::GeneratorDefinitionAlgorithm::BusGenMap &busesRegulatedBySeveralGenerators,
                  const dfl::inputs::DynamicDataBaseManager &dynamicDataBaseManager, const algo::ShuntCounterDefinitions &counters,
                  const algo::DynamicModelDefinitions &models, const algo::LinesByIdDefinitions &linesById, const algo::TransformersByIdDefinitions &tfosById,
                  const std::vector<algo::StaticVarCompensatorDefinition> &svarcsDefinitions, const std::vector<algo::LoadDefinition> &loadsDefinitions)
        : basename_(base), dirname_(config.outputDir()), filepath_(filename), activePowerCompensation_(config.getActivePowerCompensation()),
          dynamicDataBaseManager_(dynamicDataBaseManager), shuntCounters_(counters), linesByIdDefinitions_(linesById), tfosByIdDefinitions_(tfosById),
          parLoads_(new ParLoads(loadsDefinitions)), parSVarC_(new ParSVarC(svarcsDefinitions)), parHvdc_(new ParHvdc(hvdcDefinitions)),
          parGenerator_(new ParGenerator(gens)), parDynModel_(new ParDynModel(models, gens, hvdcDefinitions)),
          parVRRemote_(new ParVRRemote(gens, busesRegulatedBySeveralGenerators, hvdcDefinitions)), startingPointMode_(config.getStartingPointMode()) {}

    std::string basename_;                                                         ///< basename
    boost::filesystem::path dirname_;                                              ///< Dirname of output file relative to execution dir
    boost::filesystem::path filepath_;                                             ///< file path of the output file to write
    dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation_;  ///< the type of active power compensation
    const inputs::DynamicDataBaseManager &dynamicDataBaseManager_;                 ///< dynamic database manager
    const algo::ShuntCounterDefinitions &shuntCounters_;                           ///< Shunt counters to use
    const algo::LinesByIdDefinitions &linesByIdDefinitions_;                       ///< lines by id to use
    const algo::TransformersByIdDefinitions &tfosByIdDefinitions_;                 ///< transformers by id to use
    std::shared_ptr<ParLoads> parLoads_;                                           ///< reference to load par writer
    std::shared_ptr<ParSVarC> parSVarC_;                                           ///< reference to svarcs par writer
    std::shared_ptr<ParHvdc> parHvdc_;                                             ///< reference to hvdcs par writer
    std::shared_ptr<ParGenerator> parGenerator_;                                   ///< reference to generators par writer
    std::shared_ptr<ParDynModel> parDynModel_;                                     ///< reference to defined dynamic model par writer
    std::shared_ptr<ParVRRemote> parVRRemote_;                                     ///< reference to VRRemote par writer
    dfl::inputs::Configuration::StartingPointMode startingPointMode_;              ///< starting point mode
  };

  /**
   * @brief Constructor
   *
   * @param def PAR file definition
   */
  explicit Par(ParDefinition &&def);

  /**
   * @brief Export PAR file
   */
  void write() const;

 private:
  ParDefinition def_;  ///< PAR file definition
};

}  // namespace outputs
}  // namespace dfl
