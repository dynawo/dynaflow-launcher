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
#include <unordered_map>
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
    SECURITY_ANALYSIS              ///< A security analysis for a given list of
                                   ///< contingencies
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
  explicit Configuration(const boost::filesystem::path &filepath,
                         dfl::inputs::Configuration::SimulationKind simulationKind = dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION);

  /**
   * @brief performs sanity checks on the configuration
   *
   * @attention This method should be called after the constructor to verify that the configuration is properly set up.
   *
   * @attention The sanity checks should be executed after log initialization to ensure that any potential errors in the configuration are logged appropriately.
   */
  void sanityCheck() const;

  /**
   * @brief determines if we use infinite reactive limits
   *
   * @returns the parameter value
   */
  bool useInfiniteReactiveLimits() const { return useInfiniteReactiveLimits_; }

  /**
   * @brief getter for time table step value
   *
   * @returns the time table step value
   */
  unsigned int timeTableStep() const { return timeTableStep_; }

  /**
   * @brief determines if SVarC regulation is on
   *
   * @returns the parameter value
   */
  bool isSVarCRegulationOn() const { return isSVarCRegulationOn_; }

  /**
   * @brief determines if Shunt regulation is on
   *
   * @returns the parameter value
   */
  bool isShuntRegulationOn() const { return isShuntRegulationOn_; }

  /**
   * @brief determines if slack bus is determined automatically
   *
   * @returns the parameter value
   */
  bool isAutomaticSlackBusOn() const { return isAutomaticSlackBusOn_; }

  /**
   * @brief Retrieves the output directory
   *
   * @returns the parameter value
   */
  const boost::filesystem::path &outputDir() const { return outputDir_; }

  /**
   * @brief Retrieves the output zip file name
   *
   * @returns the parameter value
   */
  const std::string &getOutputZipName() const { return outputZipName_; }

  /**
   * @brief Retrieves the minimum voltage level of the load to be taken into
   * account
   *
   * @returns the parameter value
   */
  double getDsoVoltageLevel() const { return dsoVoltageLevel_; }

  /**
   * @brief Retrieves the maximum voltage level we assume that generator's
   * transformers are already described in the static description
   *
   * @returns the parameter value
   */
  double getTfoVoltageLevel() const { return tfoVoltageLevel_; }

  /**
   * @brief Set the Time at which the simulation will start
   *
   * @param startTime the new start time value
   */
  void setStartTime(double startTime) { startTime_ = startTime; }

  /**
   * @brief Get the Time at which the simulation will start
   *
   * @returns the start time value
   */
  double getStartTime() const { return startTime_; }

  /**
   * @brief Set the Time at which the simulation will end
   *
   * @param stopTime the new stop time value
   */
  void setStopTime(double stopTime) { stopTime_ = stopTime; }

  /**
   * @brief Get the Time at which the simulation will end
   *
   * @returns the stop time value
   */
  double getStopTime() const { return stopTime_; }

  /**
   * @brief Get the precision of the simulation
   *
   * @returns the precision value if set, boost::none otherwise
   */
  const boost::optional<double> getPrecision() const { return precision_; }

  /**
   * @brief Retrieves the Time at which the events related to each contingency
   * will be simulated
   *
   * @returns the time of event value
   */
  double getTimeOfEvent() const { return timeOfEvent_; }

  /**
   * @brief Set the Time at which the events related to each contingency will be simulated
   *
   * @param timeOfEvent the new time of event value
   */
  void setTimeOfEvent(double timeOfEvent) { timeOfEvent_ = timeOfEvent; }

  /**
   * @brief retrieves the maximum value of the solver timestep
   *
   * @return value of timestep
   */
  double getTimeStep() const { return timeStep_; }

  /**
   * @brief retrieves the maximum value of the solver timestep
   *
   * @return value of timestep
   */
  double getMinTimeStep() const { return minTimeStep_; }

  /**
   * @brief type of active power compensation for generator
   */
  enum class ActivePowerCompensation {
    P,         ///< active power mismatch compensation proportional to active power
               ///< injection P
    TARGET_P,  ///< active power mismatch compensation proportional to active
               ///< power target targetP
    PMAX       ///< active power mismatch compensation proportional to generator
               ///< maximal active power PMax
  };

  /**
   * @brief Retrieves the type of active power compensation
   *
   * @returns the parameter value
   */
  ActivePowerCompensation getActivePowerCompensation() const { return activePowerCompensation_; }

  /**
   * @brief Retrieves the setting file path
   * @returns the setting file path
   */
  const boost::filesystem::path &settingFilePath() const { return settingFilePath_; }

  /**
   * @brief Retrieves the assembling file path
   * @returns the assembling file path
   */
  const boost::filesystem::path &assemblingFilePath() const { return assemblingFilePath_; }

  /**
   * @brief Retrieves the starting dump file path
   * @returns the starting dump file path
   */
  const boost::filesystem::path &startingDumpFilePath() const { return startingDumpFilePath_; }

  /**
   * @brief Set the starting dump file path
   * @param startingDumpFilePath the new starting dump file path
   */
  void setStartingDumpFilePath(const boost::filesystem::path &startingDumpFilePath) { startingDumpFilePath_ = startingDumpFilePath; }
  /**
   * @brief Chosen outputs
   *
   * enum that gathers all possible chosen outputs
   */
  enum class ChosenOutputEnum : size_t { STEADYSTATE = 0, CONSTRAINTS, LOSTEQ, TIMELINE, DUMPSTATE };

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
    size_t operator()(ChosenOutputEnum chosenOutputEnum) const { return static_cast<size_t>(chosenOutputEnum); }
  };

  /**
   * @brief Set the starting point mode
   *
   * @param startingPointMode the starting point mode
   */
  void setStartingPointMode(StartingPointMode startingPointMode) { startingPointMode_ = startingPointMode; }

  /**
   * @brief Indicate the starting point mode
   *
   * @returns the starting point mode
   */
  StartingPointMode getStartingPointMode() const { return startingPointMode_; }

  /**
   * @brief Indicate if the output is chosen
   *
   * @param output the chosen output to check
   * @returns true if the output is chosen, false otherwise
   */
  bool isChosenOutput(const ChosenOutputEnum output) const { return static_cast<bool>(chosenOutputs_.count(output)); }

  /**
   * @brief add an output
   *
   * @param output the chosen output to add
   */
  void addChosenOutput(const ChosenOutputEnum output) { chosenOutputs_.insert(output); }

  /**
   * @brief returns true if a parameter value was given in the configuration
   * file and false if default value was used
   *
   * @param key the parameter key
   * @returns true if a parameter value was given in the configuration file and
   * false if default value was used
   */
  bool defaultValueModified(const std::string &key) const { return parameterValueModified_.find(key) != parameterValueModified_.end(); }

 private:
  /**
   * @brief Helper function to update the starting point mode
   *
   * @param tree the element of the boost tree
   * @param saMode true if simulation is in SA, false otherwise
   */
  void updateStartingPointMode(const boost::property_tree::ptree &tree, const bool saMode);
  /**
   * @brief Helper function to update the chosen outputs
   *
   * @param tree the element of the boost tree
   * @param simulationKind simulation kind of the simulation
   * @param saMode true if simulation is in SA, false otherwise
   */
  void updateChosenOutput(const boost::property_tree::ptree &tree, SimulationKind simulationKind, const bool saMode);

 private:
  boost::filesystem::path filepath_;  ///< the configuration file path
  SimulationKind simulationKind_;     ///< the simulation kind (Steady state or Security analysis)

  // General
  StartingPointMode startingPointMode_ = StartingPointMode::WARM;                    ///< simulation starting point mode
  bool useInfiniteReactiveLimits_ = false;                                           ///< infinite reactive limits
  bool isSVarCRegulationOn_ = true;                                                  ///< StaticVarCompensator regulation on
  bool isShuntRegulationOn_ = true;                                                  ///< Shunt regulation on
  bool isAutomaticSlackBusOn_ = true;                                                ///< automatic slack bus on
  boost::filesystem::path outputDir_ = boost::filesystem::current_path();            ///< Directory for output files
  std::string outputZipName_ = std::string("outputs.zip");                           ///< Name of the zip outputs archive, by default "outputs.zip"
  double dsoVoltageLevel_ = 45.0;                                                    ///< Minimum voltage level of the load to be taken into account
  ActivePowerCompensation activePowerCompensation_ = ActivePowerCompensation::PMAX;  ///< Type of active power compensation
  boost::filesystem::path settingFilePath_;                                          ///< setting file path
  boost::filesystem::path assemblingFilePath_;                                       ///< assembling file path
  double startTime_ = 0.;                                                            ///< start time of simulation
  double stopTime_ = 100.;                                                           ///< stop time for simulation
  boost::optional<double> precision_;                                                ///< Precision of the simulation
  double timeStep_ = 10.;                                                            ///< maximum value of the solver timestep
  double minTimeStep_ = 1.;                                                          ///< minimum value of the solver timestep
  std::unordered_set<ChosenOutputEnum, ChosenOutputHash> chosenOutputs_;             ///< chosen configuration outputs
  double tfoVoltageLevel_ = 100;     ///< Maximum voltage level we assume that generator's transformers are already described in the static description
  unsigned int timeTableStep_ = 0;  ///< time table step to display progress

  // SA
  double timeOfEvent_ = 10.;                                ///< time for contingency simulation (security analysis only)
  boost::filesystem::path startingDumpFilePath_;            ///< starting dump file path
                                                            ///< are already described in the static description
  std::unordered_set<std::string> parameterValueModified_;  ///< a parameter key is present in this if the
                                                            ///< value was redefined in the configuration
                                                            ///< file
};

}  // namespace inputs
}  // namespace dfl
