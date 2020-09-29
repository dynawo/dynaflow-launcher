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
    return InfiniteReactiveLimits_;
  }

  /**
   * @brief determines if PST regulation is on
   *
   * @returns the parameter value
   */
  bool isPSTRegulationOn() const {
    return PSTRegulationOn_;
  }

  /**
   * @brief determines if SVC regulation is on
   *
   * @returns the parameter value
   */
  bool isSVCRegulationOn() const {
    return SVCRegulationOn_;
  }

  /**
   * @brief determines if Shunt regulation is on
   *
   * @returns the parameter value
   */
  bool isShuntRegulationOn() const {
    return ShuntRegulationOn_;
  }

  /**
   * @brief determines if slack bus is determined automatically
   *
   * @returns the parameter value
   */
  bool isAutomaticSlackBusOn() const {
    return AutomaticSlackBusOn_;
  }

  /**
   * @brief determines if VSC are considered as generators
   *
   * @returns the parameter value
   */
  bool useVSCAsGenerators() const {
    return VSCAsGenerators_;
  }

  /**
   * @brief determines ifLCC are considered as loads
   *
   * @returns the parameter value
   */
  bool useLCCAsLoads() const {
    return LCCAsLoads_;
  }

 private:
  bool InfiniteReactiveLimits_ = false;  ///< infinite reactive limits
  bool PSTRegulationOn_ = true;          ///< PST regulation on
  bool SVCRegulationOn_ = true;          ///< SVC regulation on
  bool ShuntRegulationOn_ = true;        ///< Shunt regulation on
  bool AutomaticSlackBusOn_ = true;      ///< automatic slack bus on
  bool VSCAsGenerators_ = false;         ///< VSC considered as generators
  bool LCCAsLoads_ = false;              ///< LCC are considered as generators
};

}  // namespace inputs
}  // namespace dfl
