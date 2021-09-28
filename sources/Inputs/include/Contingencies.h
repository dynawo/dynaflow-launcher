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
#include <boost/optional.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace dfl {
namespace inputs {
/**
 * @brief Class for dynaflow launcher contingencies definition
 */
class Contingencies {
 public:
  /**
   * @brief Enum that defines accepted network element types
   */
  enum class ElementType {
    LOAD,
    GENERATOR,
    BRANCH,
    LINE,
    TWO_WINDINGS_TRANSFORMER,
    THREE_WINDINGS_TRANSFORMER,
    SHUNT_COMPENSATOR,
    STATIC_VAR_COMPENSATOR,
    DANGLING_LINE,
    HVDC_LINE,
    BUSBAR_SECTION
  };

  /**
   * @brief Contingency element definition
   */
  struct ContingencyElementDefinition {
    /**
     * @brief The validation status of the contingency element
     */
    enum class ValidationStatus { NOT_IN_NETWORK_OR_NOT_IN_MAIN_CC, MAIN_CC_INVALID_TYPE, MAIN_CC_VALID_TYPE };

    /**
     * @brief Constructor
     */
    explicit ContingencyElementDefinition(const std::string& id) : id(id), status(ValidationStatus::NOT_IN_NETWORK_OR_NOT_IN_MAIN_CC) {}

    std::string id;           ///< id of the element affected by a contingency
    ElementType type;         ///< type of the element affected by the contingency (BRANCH, GENERATOR, LOAD, ...)
    ValidationStatus status;  ///< validation status of the element affected by the contingency
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
   * Empty list of contingencies
   */
  Contingencies() {}

  /**
   * @brief Constructor
   *
   * Load contingency definitions from file. Exit the program on error in parsing the file
   *
   * @param filepath the JSON contingencies file to use
   */
  explicit Contingencies(const boost::filesystem::path& filepath);

  /**
   * @brief Definitions
   *
   * Obtain a reference to the contingency definitions
   *
   * @return reference to contingency definitions
   */
  const std::vector<std::shared_ptr<ContingencyDefinition>>& definitions() const {
    return contingencies_;
  }

  /**
   * @brief Mark the element given by id and type as valid in all contingencies where it is referred
   *
   * @param id the id of the element found in the snetwork
   * @param type the type of the element as it has been found in the network
   */
  void markElementValid(const std::string id, ElementType type);

  /// @brief Check that a contingency can be simulated by Dynawo
  static bool isValidForSimulation(const ContingencyDefinition& c);

  /**
   * @brief Get element type enum from a string
   *
   * Parses a string into its 'ElementType' enum value
   *
   * @return none if not a valid type, otherwise the enum value
   */
  static boost::optional<ElementType> elementTypeFromString(const std::string& str);

  /**
   * @brief ElementType to string
   *
   * Transforms an ElementType enum into its string representation
   *
   * @return string representation of element type
   */
  static std::string toString(ElementType type);

  /// @brief Validation status to string
  static std::string toString(ContingencyElementDefinition::ValidationStatus status);

 private:
  /// @brief Load contingency definitions from an input file
  void loadDefinitions(const boost::filesystem::path& filepath);

  /// @brief Complete initializations after definitions have been read
  void init();

  /// @brief Check if a network element type is valid against an element type defined in contingencies
  static bool isValidType(ElementType type, ElementType referenceType);

  /// @brief Contingency definitions received as input
  std::vector<std::shared_ptr<ContingencyDefinition>> contingencies_;

  /// @brief For each element identifier, a list of all the contingencies where it is referenced
  std::unordered_map<std::string, std::vector<std::shared_ptr<ContingencyDefinition>>> elementContingencies_;
};

}  // namespace inputs
}  // namespace dfl
