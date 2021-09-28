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
 * @brief Contingency elements
 */
struct ContingencyElement {
  /**
   * @brief Enum with accepted types for the elements in a contingency
   */
  enum class Type {
    LOAD = 0,
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
   * @brief Constructor
   *
   * @param id the element id
   * @param type the element type
   */
  ContingencyElement(const std::string& id, Type type) : id(id), type(type) {}

  /**
   * @brief Check if a given type is compatible with a reference type
   *
   * Used to check element types observed in the network against types used in input contingencies
   *
   * @param type a type to check
   * @param referenceType the reference type to check against
   */
  static bool isCompatible(Type type, Type referenceType);

  /**
   * @brief Get type enum value from a string
   *
   * Parses a string into its enum value
   *
   * @return none if not a valid type, otherwise the enum value
   */
  static boost::optional<Type> typeFromString(const std::string& str);

  /**
   * @brief Type to string
   *
   * Transforms a type enum value into its string representation
   *
   * @return string representation of element type
   */
  static std::string toString(Type type);

  const std::string id;  ///< Identifier of an element affected by a contingency
  const Type type;       ///< Type of the element affected by the contingency (BRANCH, GENERATOR, LOAD, ...)
};

/**
 * @brief Contingency
 */
struct Contingency {
  /**
   * @brief Constructor
   *
   * @param id the contingency id
   */
  explicit Contingency(const std::string& id) : id(id), elements{} {}

  const std::string id;                      ///< Identifier of the contingency
  std::vector<ContingencyElement> elements;  ///< Elements affected by the contingency
};

}  // namespace inputs
}  // namespace dfl
