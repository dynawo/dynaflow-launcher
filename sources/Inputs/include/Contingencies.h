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

#include <boost/optional.hpp>
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
   * @brief Enum that defines network elements types
   */
  enum class Type {
    GENERATOR,
    LINE,
    BRANCH,
    SHUNT_COMPENSATOR,
    LOAD,
    DANGLING_LINE,
    HVDC_LINE,
    STATIC_VAR_COMPENSATOR,
    BUSBAR_SECTION,
    TWO_WINDINGS_TRANSFORMER
  };

  /**
   * @brief Explains why an element is considered invalid
   */
  enum class ElementInvalidReason {
    GENERATOR_NOT_FOUND,
    TWOWINDINGS_TRANFORMER_NOT_FOUND,
    LINE_NOT_FOUND,
    BRANCH_NOT_FOUND,
    SHUNT_COMPENSATOR_NOT_FOUND,
    LOAD_NOT_FOUND,
    DANGLING_LINE_NOT_FOUND,
    HVDC_LINE_NOT_FOUND,
    STATIC_VAR_COMPENSATOR_NOT_FOUND,
    BUSBAR_SECTION_NOT_FOUND,
    NOT_IN_MAIN_CONNECTED_COMPONENT,
  };

  /**
   * @brief Contingency element definition
   */
  struct ContingencyElementDefinition {
    /**
     * @brief Constructor
     */
    explicit ContingencyElementDefinition(const std::string& id) : id(id) {}

    std::string id;    ///< id of the element affected by a contingency
    Type type;  ///< type of the element affected by the contingency (BRANCH, GENERATOR, LOAD, ...)
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
   * Define which contingencies do we have
   *
   */
  explicit Contingencies(std::vector<std::shared_ptr<ContingencyDefinition>> contingencies): contingencies_(contingencies) {}

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
  const std::vector<std::shared_ptr<ContingencyDefinition>>& definitions() const { return contingencies_; }

  /**
   * @brief Log
   *
   * Log the contingencies definitions
   *
   */
  void log();

  /**
   * @brief Get type enum
   *
   * Parses a string into it's 'Type' enum representation
   *
   * @return none if not a valid type, otherwise the enum value
   */
  static boost::optional<Type> typeFromString(const std::string& str);

  /**
   * @brief ElementInvalidReason to string
   *
   * Transforms an ElementInvalidReason into a string description
   *
   */
  static std::string toString(ElementInvalidReason reason);

  /**
   * @brief Type to string
   *
   * Transforms a Type enum into it's string representation
   *
   */
  static std::string toString(Type type);

 private:
  std::vector<std::shared_ptr<ContingencyDefinition>> contingencies_;  ///< contingency definitions
};

}  // namespace inputs
}  // namespace dfl
