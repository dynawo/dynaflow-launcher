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
#include "DynamicDataBaseManager.h"

#include <DYNLineInterface.h>
#include <PARMacroParameterSet.h>
#include <PARParameterFactory.h>
#include <PARParametersSet.h>
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
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
     * @param hvdcDefinitions HVDC lines definitions
     * @param activePowerCompensation the type of active power compensation
     * @param busesWithDynamicModel map of bus ids to a generator that regulates them
     * @param dynamicDataBaseManager dynamic database manager to use
     * @param counters the counters definitions to use
     * @param models list of dynamic models definitions
     * @param linesById the lines to use
     * @param svarcsDefinitions the SVarC definitions to use
     * @param loadsDefinitions the loads definitions to use
     */
    ParDefinition(const std::string& base, const boost::filesystem::path& dir, const boost::filesystem::path& filename,
                  const std::vector<algo::GeneratorDefinition>& gens, const algo::HVDCLineDefinitions& hvdcDefinitions,
                  inputs::Configuration::ActivePowerCompensation activePowerCompensation,
                  const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                  const algo::ShuntCounterDefinitions& counters, const algo::DynamicModelDefinitions& models, const algo::LinesByIdDefinitions& linesById,
                  const std::vector<algo::StaticVarCompensatorDefinition>& svarcsDefinitions,
                  const std::vector<algo::LoadDefinition>& loadsDefinitions) :
        basename(base),
        dirname(dir),
        filepath(filename),
        generators(gens),
        hvdcDefinitions(hvdcDefinitions),
        activePowerCompensation(activePowerCompensation),
        busesWithDynamicModel(busesWithDynamicModel),
        dynamicDataBaseManager(dynamicDataBaseManager),
        shuntCounters(counters),
        dynamicModelsDefinitions(models),
        linesByIdDefinitions(linesById),
        svarcsDefinitions(svarcsDefinitions),
        loadsDefinitions(loadsDefinitions) {}

    std::string basename;                                                         ///< basename
    boost::filesystem::path dirname;                                              ///< Dirname of output file relative to execution dir
    boost::filesystem::path filepath;                                             ///< file path of the output file to write
    std::vector<algo::GeneratorDefinition> generators;                            ///< list of generators
    const algo::HVDCLineDefinitions& hvdcDefinitions;                             ///< HVDC definitions
    dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation;  ///< the type of active power compensation
    const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel;   ///< map of bus ids to a generator that regulates them
    const inputs::DynamicDataBaseManager& dynamicDataBaseManager;                 ///< dynamic database manager
    const algo::ShuntCounterDefinitions& shuntCounters;                           ///< Shunt counters to use
    const algo::DynamicModelDefinitions& dynamicModelsDefinitions;                ///< list of defined dynamic models
    const algo::LinesByIdDefinitions& linesByIdDefinitions;                       ///< lines by id to use
    std::vector<algo::StaticVarCompensatorDefinition> svarcsDefinitions;          ///< list of static var compensator definitions
    std::vector<algo::LoadDefinition> loadsDefinitions;                           ///< list of loads definitions
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
  void write() const;

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
                                                                     const boost::filesystem::path& dirname);
  /**
   * @brief Write hvdc line parameter set
   *
   * @param hvdcLine the hvdc line definition to use
   * @param basename the basename for the simulation
   * @param dirname the dirname of the output directory
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeHdvcLine(const algo::HVDCDefinition& hvdcLine, const std::string& basename,
                                                                    const boost::filesystem::path& dirname);

  /**
   * @brief Write remote voltage regulators parameter set
   *
   * @param busId the bus id to use
   * @param elementId the element id to use (generator or VSC converter)
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeVRRemote(const std::string& busId, const std::string& elementId);

  /**
   * @brief Write setting set for dynamic models
   *
   * @param set the configuration set to write
   * @param assemblingDoc the corresponding assembling document handler
   * @param counters the counters to use
   * @param models the models definitions to use
   * @param linesById lines by id to use
   *
   * @returns the parameter set to add
   */
  static boost::shared_ptr<parameters::ParametersSet> writeDynamicModelParameterSet(const inputs::SettingXmlDocument::Set& set,
                                                                                    const inputs::AssemblingXmlDocument& assemblingDoc,
                                                                                    const algo::ShuntCounterDefinitions& counters,
                                                                                    const algo::DynamicModelDefinitions& models,
                                                                                    const algo::LinesByIdDefinitions& linesById);

  /**
   * @brief Retrieves the first component connected through the dynamic model to a transformer
   *
   * @param dynModelDef the dynamic model to process
   * @returns the transformer id connected to the dynamic model, nullopt if not found
   */
  static boost::optional<std::string> getTransformerComponentId(const algo::DynamicModelDefinition& dynModelDef);

  /**
   * @brief Retrieve active season
   *
   * @param ref the Ref XML element referencing the active season
   * @param linesById Dynawo lines by id to use
   * @param assemblingDoc the assembling document containing the association referenced
   */
  static boost::optional<std::string> getActiveSeason(const inputs::SettingXmlDocument::Ref& ref, const algo::LinesByIdDefinitions& linesById,
                                                      const inputs::AssemblingXmlDocument& assemblingDoc);

  /**
   * @brief Write parameter set for static var compensator
   * @param svarc the static var compensator to use
   * @returns the parameter set to add to the exported file
   */
  static boost::shared_ptr<parameters::ParametersSet> writeStaticVarCompensator(const algo::StaticVarCompensatorDefinition& svarc);

  /**
   * @brief Write the macro parameter set used for static var compensators
   * @returns the macro parameter set for SVarC
   */
  static boost::shared_ptr<parameters::MacroParameterSet> writeMacroParameterSetStaticVarCompensators();

  /**
   * @brief Computes the susceptance value in PU unit
   *
   * bMaxPU = B * Vnom² / Sb
   *
   * @param b the susceptance value to convert
   * @param VNom the nominal voltage of the corresponding SVarC
   * @returns the converted susceptance value
   */
  static double computeBPU(double b, double VNom) {
    return b * VNom * VNom / Sb_;
  }

  /**
   * @brief Computes KAC emulation parameter
   *
   * @param droop droop data, in MW/deg
   * @returns KAC, in p.u/rad (base SnRef=100MW)
   */
  static inline constexpr double computeKAC(double droop) {
    return droop * 1.8 / pi_;
  }

 private:
  static constexpr double kGoverNullValue_ = 0.;                 ///< KGover null value
  static constexpr double kGoverDefaultValue_ = 1.;              ///< KGover default value
  static constexpr double pi_ = M_PI;                            ///< PI value
  static constexpr double svarcThresholdDown_ = 0.;              ///< time threshold down for SVarC
  static constexpr double svarcThresholdUp_ = 60.;               ///< time threshold up for SVarC
  static constexpr double Sb_ = 100;                             ///< Sb value
  static const std::string componentTransformerIdTag_;           ///< TFO special tag for component id
  static const std::string seasonTag_;                           ///< Season special tag
  static const std::string macroParameterSetStaticCompensator_;  ///< Name of the macro parameter set for static var compensator

 private:
  ParDefinition def_;  ///< PAR file definition
};

}  // namespace outputs
}  // namespace dfl
