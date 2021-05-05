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
 * @file  Contingencies.h
 *
 * @brief Contingencies header file
 *
 */
#pragma once

#include <boost/filesystem.hpp>
#include <string>

namespace dfl {
namespace inputs {
/**
 * @brief Class for dynaflow launcher contingencies definition
 */
class Contingencies {
 public:
  /**
   * @brief Contingency element definition
   */
  struct ContingencyElementDefinition {
    /**
     * @brief Constructor
     */
    explicit ContingencyElementDefinition(const std::string& id) : id(id) {}

    std::string id;    ///< id of the element affected by a contingency
    std::string type;  ///< type of the element affected by the contingency (BRANCH, GENERATOR, LOAD, ...)
  };

  /**
   * @brief Contingency definition
   */
  struct ContingencyDefinition {
    /**
     * @brief Constructor
     */
    explicit ContingencyDefinition(const std::string& id) : id(id), elements{} {}

    std::string id;                                      ///< id of the contingency
    std::vector<ContingencyElementDefinition> elements;  ///< elements affected by the contingency
  };

  /**
   * @brief Constructor
   *
   * Start with an empty list of contingencies
   *
   */
  Contingencies() {}

  /**
   * @brief Constructor
   *
   * exit the program on error in parsing the file
   *
   * @param filepath the JSON contingencies file to use
   */
  explicit Contingencies(const std::string& filepath);

  /**
   * @brief Definitions
   *
   * obtain a reference to the contingency definitions
   *
   */
  const std::vector<ContingencyDefinition>& definitions() const { return contingencies; }

  /**
   * @brief Log
   *
   * Log the contingencies definitions
   *
   */
  void log();

 private:
  std::vector<ContingencyDefinition> contingencies;  ///< contingency definitions
};

}  // namespace inputs
}  // namespace dfl
