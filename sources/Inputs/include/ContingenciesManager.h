//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  ContingenciesManager.h
 *
 * @brief ContingenciesManager header file
 *
 */
#pragma once

#include "Contingencies.h"

#include <boost/filesystem.hpp>
#include <string>

namespace dfl {
namespace inputs {

/**
 * @brief Manage the contingencies given as input for a Security Analysis simulation
 */
class ContingenciesManager {
 public:
  /**
   * @brief Constructor
   *
   * Load contingency from file. Exit the program on error in parsing the file
   *
   * @param filepath the JSON contingencies file to use
   */
  explicit ContingenciesManager(const boost::filesystem::path& filepath);

  /**
   * @brief List of contingencies
   *
   * Obtain the list of contingencies defined in the input
   *
   * @return contingency list
   */
  const std::vector<Contingency>& get() const {
    return contingencies_;
  }

 private:
  /// @brief Load contingencies from an input file
  /// @param filepath the JSON contigencies file to load
  void load(const boost::filesystem::path& filepath);

  std::vector<Contingency> contingencies_;  ///< Contingencies obtained from input file
};

}  // namespace inputs
}  // namespace dfl
