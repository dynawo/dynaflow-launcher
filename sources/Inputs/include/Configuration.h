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
   * @brief determines if VSC are considered as generators
   *
   * @returns the parameter value
   */
  bool useVSCAsGenerators() const {
    return useVSCAsGenerators_;
  }

  /**
   * @brief determines if LCC are considered as loads
   *
   * @returns the parameter value
   */
  bool useLCCAsLoads() const {
    return useLCCAsLoads_;
  }

  /**
   * @brief Retrieves the output directory
   *
   * @returns the parameter value
   */
  const std::string& outputDir() const {
    return outputDir_;
  }

 private:
  bool useInfiniteReactiveLimits_ = false;  ///< infinite reactive limits
  bool isPSTRegulationOn_ = true;           ///< PST regulation on
  bool isSVCRegulationOn_ = true;           ///< SVC regulation on
  bool isShuntRegulationOn_ = true;         ///< Shunt regulation on
  bool isAutomaticSlackBusOn_ = true;       ///< automatic slack bus on
  bool useVSCAsGenerators_ = false;         ///< VSC considered as generators
  bool useLCCAsLoads_ = false;              ///< LCC are considered as generators
  std::string outputDir_ = ".";             ///< Directory for output files
};

}  // namespace inputs
}  // namespace dfl
