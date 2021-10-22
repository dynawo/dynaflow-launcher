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
#include <string>

namespace dfl {
namespace inputs {
/**
 * @brief Class for dynaflow launcher specific configuration
 */
class Configuration {
 public:
  /**
   * @brief Constructor
   *
   * exit the program on error in parsing the file
   *
   * @param filepath the configuration file to use
   */
  explicit Configuration(const boost::filesystem::path& filepath);

  /**
   * @brief determines if we use infinite reactive limits
   *
   * @returns the parameter value
   */
  bool useInfiniteReactiveLimits() const {
    return useInfiniteReactiveLimits_;
  }

  /**
   * @brief determines if PST regulation is on
   *
   * @returns the parameter value
   */
  bool isPSTRegulationOn() const {
    return isPSTRegulationOn_;
  }

  /**
   * @brief determines if SVC regulation is on
   *
   * @returns the parameter value
   */
  bool isSVCRegulationOn() const {
    return isSVCRegulationOn_;
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
   * @brief Get the Time at which the simulation will start
   *
   * @returns the start time value
   */
  float getStartTime() const {
    return startTime_;
  }

  /**
   * @brief Get the Time at which the simulation will end
   *
   * @returns the stop time value
   */
  float getStopTime() const {
    return stopTime_;
  }

  /**
   * @brief Retrieves the Time at which the events related to each contingency will be simulated
   *
   * @returns the time of event value
   */
  float getTimeOfEvent() const {
    return timeOfEvent_;
  }

  /**
   * @brief Retrieves the number of threads used by dynaflow launcher
   *
   * @returns the number of threads
   */
  int getNumberOfThreads() const {
    return numberOfThreads_;
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

 private:
  bool useInfiniteReactiveLimits_ = false;                                           ///< infinite reactive limits
  bool isPSTRegulationOn_ = true;                                                    ///< PST regulation on
  bool isSVCRegulationOn_ = true;                                                    ///< SVC regulation on
  bool isShuntRegulationOn_ = true;                                                  ///< Shunt regulation on
  bool isAutomaticSlackBusOn_ = true;                                                ///< automatic slack bus on
  boost::filesystem::path outputDir_ = boost::filesystem::current_path();            ///< Directory for output files
  double dsoVoltageLevel_ = 45.0;                                                    ///< Minimum voltage level of the load to be taken into account
  ActivePowerCompensation activePowerCompensation_ = ActivePowerCompensation::PMAX;  ///< Type of active power compensation
  boost::filesystem::path settingFilePath_;                                          ///< setting file path
  boost::filesystem::path assemblingFilePath_;                                       ///< assembling file path
  float startTime_ = 0;                                                              ///< Moment (in seconds) at which starts the simulation
  float stopTime_ = 100;                                                             ///< Moment (in seconds) at which ends the simulation
  // Security Analysis only
  double timeOfEvent_ = 80.0;  ///< Moment (in seconds) at which the contingencies are simulated
  int numberOfThreads_ = 4;    ///< The number of threads used to simulate
};

}  // namespace inputs
}  // namespace dfl
