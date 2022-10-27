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
 * @file  Configuration.h
 *
 * @brief Configuration header file
 *
 */
#pragma once

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>
#include <string>
#include <unordered_set>

namespace dfl {
namespace inputs {
/**
 * @brief Class for dynaflow launcher specific configuration
 */
class Configuration {
 public:
  /// @brief The kind of simulation that is requested
  enum class SimulationKind {
    STEADY_STATE_CALCULATION = 0,  ///< A steady-state calculation
    SECURITY_ANALYSIS              ///< A security analysis for a given list of contingencies
  };
  /// @brief Simulation starting mode
  enum class StartingPointMode { WARM = 0, FLAT };
  /**
   * @brief Constructor
   *
   * exit the program on error in parsing the file
   *
   * @param filepath the configuration file to use
   * @param simulationKind the simulation kind (Steady state or Security analysis)
   */
  explicit Configuration(const boost::filesystem::path& filepath,
                         dfl::inputs::Configuration::SimulationKind simulationKind = dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION);
  /**
   * @brief determines if we use infinite reactive limits
   *
   * @returns the parameter value
   */
  bool useInfiniteReactiveLimits() const {
    return useInfiniteReactiveLimits_;
  }

  /**
   * @brief determines if SVarC regulation is on
   *
   * @returns the parameter value
   */
  bool isSVarCRegulationOn() const {
    return isSVarCRegulationOn_;
  }

  /**
   * @brief determines if Shunt regulation is on
   *
   * @returns the parameter value
   */
  bool isShuntRegulationOn() const {
    return isShuntRegulationOn_;
  }

  /**
   * @brief determines if slack bus is determined automatically
   *
   * @returns the parameter value
   */
  bool isAutomaticSlackBusOn() const {
    return isAutomaticSlackBusOn_;
  }

  /**
   * @brief Retrieves the output directory
   *
   * @returns the parameter value
   */
  const boost::filesystem::path& outputDir() const {
    return outputDir_;
  }

  /**
   * @brief Retrieves the minimum voltage level of the load to be taken into account
   *
   * @returns the parameter value
   */
  double getDsoVoltageLevel() const {
    return dsoVoltageLevel_;
  }

  /**
   * @brief Retrieves the maximum voltage level we assume that generator's transformers are already described in the static description
   *
   * @returns the parameter value
   */
  double getTfoVoltageLevel() const {
    return tfoVoltageLevel_;
  }

  /**
   * @brief Get the Time at which the simulation will start
   *
   * @returns the start time value
   */
  double getStartTime() const {
    return startTime_;
  }

  /**
   * @brief Get the Time at which the simulation will end
   *
   * @returns the stop time value
   */
  double getStopTime() const {
    return stopTime_;
  }

  /**
   * @brief Get the precision of the simulation
   *
   * @returns the precision value if set, boost::none otherwise
   */
  const boost::optional<double> getPrecision() const {
    return precision_;
  }

  /**
   * @brief Retrieves the Time at which the events related to each contingency will be simulated
   *
   * @returns the time of event value
   */
  double getTimeOfEvent() const {
    return timeOfEvent_;
  }

  /**
   * @brief retrieves the maximum value of the solver timestep
   *
   * @return value of timestep
   */
  double getTimeStep() const {
    return timeStep_;
  }

  /**
   * @brief type of active power compensation for generator
   */
  enum class ActivePowerCompensation {
    P,         ///< active power mismatch compensation proportional to active power injection P
    TARGET_P,  ///< active power mismatch compensation proportional to active power target targetP
    PMAX       ///< active power mismatch compensation proportional to generator maximal active power PMax
  };

  /**
   * @brief Retrieves the type of active power compensation
   *
   * @returns the parameter value
   */
  ActivePowerCompensation getActivePowerCompensation() const {
    return activePowerCompensation_;
  }

  /**
   * @brief Retrieves the setting file path
   * @returns the setting file path
   */
  const boost::filesystem::path& settingFilePath() const {
    return settingFilePath_;
  }

  /**
   * @brief Retrieves the assembling file path
   * @returns the assembling file path
   */
  const boost::filesystem::path& assemblingFilePath() const {
    return assemblingFilePath_;
  }

  /**
   * @brief Chosen outputs
   *
   * enum that gathers all possible chosen outputs
   */
  enum class ChosenOutputEnum : size_t { STEADYSTATE = 0, CONSTRAINTS, LOSTEQ, TIMELINE };

  /**
   * @brief Hash structure for chosenOutputEnum
   */
  struct ChosenOutputHash {
    /**
     * @brief Operator to retrieve chosenOutputEnum hash value
     *
     * @param chosenOutputEnum the chosenOutputEnum to hash
     * @returns the hash value
     */
    size_t operator()(ChosenOutputEnum chosenOutputEnum) const {
      return static_cast<size_t>(chosenOutputEnum);
    }
  };

  /**
   * @brief Indicate the starting point mode
   *
   * @returns the starting point mode
   */
  StartingPointMode getStartingPointMode() const {
    return startingPointMode_;
  }

  /**
   * @brief Indicate if the output is chosen
   *
   * @param output the chosen output to check
   * @returns true if the output is chosen, false otherwise
   */
  bool isChosenOutput(const ChosenOutputEnum output) const {
    return static_cast<bool>(chosenOutputs_.count(output));
  }

 private:
  /**
  * @brief Helper function to update the starting point mode
  *
  * @param tree the element of the boost tree
  */
  void updateStartingPointMode(const boost::property_tree::ptree& tree);
  /**
  * @brief Helper function to update the chosen outputs
  *
  * @param tree the element of the boost tree
  * @param simulationKind simulation kind of the simulation
  */
  void updateChosenOutput(const boost::property_tree::ptree& tree, dfl::inputs::Configuration::SimulationKind simulationKind);

  StartingPointMode startingPointMode_ = StartingPointMode::WARM;                    ///< simulation starting point mode
  bool useInfiniteReactiveLimits_ = false;                                           ///< infinite reactive limits
  bool isSVarCRegulationOn_ = true;                                                  ///< StaticVarCompensator regulation on
  bool isShuntRegulationOn_ = true;                                                  ///< Shunt regulation on
  bool isAutomaticSlackBusOn_ = true;                                                ///< automatic slack bus on
  boost::filesystem::path outputDir_ = boost::filesystem::current_path();            ///< Directory for output files
  double dsoVoltageLevel_ = 45.0;                                                    ///< Minimum voltage level of the load to be taken into account
  ActivePowerCompensation activePowerCompensation_ = ActivePowerCompensation::PMAX;  ///< Type of active power compensation
  boost::filesystem::path settingFilePath_;                                          ///< setting file path
  boost::filesystem::path assemblingFilePath_;                                       ///< assembling file path
  double startTime_ = 0.;                                                            ///< start time of simulation
  double stopTime_ = 100.;                                                           ///< stop time for simulation
  boost::optional<double> precision_;                                                ///< Precision of the simulation
  double timeStep_ = 10.;                                                            ///< maximum value of the solver timestep
  double timeOfEvent_ = 10.;                                                         ///< time for contingency simulation (security analysis only)
  std::unordered_set<ChosenOutputEnum, ChosenOutputHash> chosenOutputs_;             ///< chosen configuration outputs
  double tfoVoltageLevel_ = 100;  ///< Maximum voltage level we assume that generator's transformers are already described in the static description
};

}  // namespace inputs
}  // namespace dfl
