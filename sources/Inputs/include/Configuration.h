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
  explicit Configuration(const std::string& filepath);

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
  const std::string& outputDir() const {
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
   * @brief Get the timestamp at which the simulation will start
   *
   * @returns the parameter value
   */
  uint32_t getStartTimestamp() const {
    return startTimestamp_;
  }

  /**
   * @brief Get the timestamp at which the simulation will end
   *
   * @returns the parameter value
   */
  uint32_t getEndTimestamp() const {
    return endTimestamp_;
  }

  /**
   * @brief Retrieves the timestamp at which the contingencies will happen
   *
   * @returns the parameter value
   */
  double getContingenciesTimestamp() const {
    return contingenciesTimestamp_;
  }

  /**
   * @brief Retrieves the number of threads used by dynaflow launcher
   *
   * @returns the parameter value
   */
  uint16_t getNumberOfThreads() const {
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

 private:
  bool useInfiniteReactiveLimits_ = false;                                           ///< infinite reactive limits
  bool isPSTRegulationOn_ = true;                                                    ///< PST regulation on
  bool isSVCRegulationOn_ = true;                                                    ///< SVC regulation on
  bool isShuntRegulationOn_ = true;                                                  ///< Shunt regulation on
  bool isAutomaticSlackBusOn_ = true;                                                ///< automatic slack bus on
  std::string outputDir_ = boost::filesystem::current_path().generic_string();       ///< Directory for output files
  double dsoVoltageLevel_ = 45.0;                                                    ///< Minimum voltage level of the load to be taken into account
  ActivePowerCompensation activePowerCompensation_ = ActivePowerCompensation::PMAX;  ///< Type of active power compensation
  uint32_t startTimestamp_ = 0;                                                      ///< Moment (in seconds) at which starts the simulation
  uint32_t endTimestamp_ = 100;                                                      ///< Moment (in seconds) at which ends the simulation
  // Security Analysis only
  double contingenciesTimestamp_ = 80.0;                                             ///< When will the contengencies be simulated
  uint16_t numberOfThreads_ = 4;                                                     ///< The number of threads used to simulate
};

}  // namespace inputs
}  // namespace dfl
