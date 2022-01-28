//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  ContingencyValidationAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for contingencies validation header file
 *
 */

#pragma once

#include "Contingencies.h"
#include "HvdcLine.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node

namespace algo {

/**
 * @brief Contingencies valid for simulation
 */
class ValidContingencies {
 public:
  using ContingencyId = std::string;                                         ///< Alias for contingency identifier
  using ElementId = std::string;                                             ///< Alias for element identifier in contingency
  using ContingencyRef = std::reference_wrapper<const inputs::Contingency>;  ///< Alias for reference to contingency

  /**
   * @brief Constructor
   * @param contingencies The list of contingencies given in the inputs
   */
  explicit ValidContingencies(const std::vector<inputs::Contingency>& contingencies);

  /**
   * @brief Mark the element given by id and type as valid in all contingencies where it is referred
   *
   * @param id the id of the element found in the network
   * @param type the type of the element as it has been found in the network
   */
  void markElementValid(const ElementId& id, inputs::ContingencyElement::Type type);

  /**
   * @brief Keep as valid contingencies only the ones that have all elements marked as valid
   */
  void keepContingenciesWithAllElementsValid();

  /**
   * @brief All valid contigencies
   * @return valid contingencies
   */
  const std::vector<ContingencyRef>& get() {
    return validContingencies_;
  }

 private:
  using ContingenciesRef = std::reference_wrapper<const std::vector<inputs::Contingency>>;     ///< Alias for a reference to the list of contingencies
  using ElementContingenciesMap = std::unordered_map<ElementId, std::vector<ContingencyRef>>;  ///< Alias for map of element contingencies
  using ElementIds = std::unordered_set<ElementId>;                                            ///< Alias for set of element ids
  using ValidatingContingenciesMap = std::unordered_map<ContingencyId, ElementIds>;            ///< Alias for map of contingencies with valid elements found

  ContingenciesRef contingencies_;                      ///< Contingencies requested in the inputs
  ElementContingenciesMap elementContingencies_;        ///< For each element identifier, all the contingencies where it is referenced
  ValidatingContingenciesMap validatingContingencies_;  ///< All contingencies with valid elements found, indexed by contingencyId
  std::vector<ContingencyRef> validContingencies_;      ///< Only valid contingencies
};

/**
 * @brief Validation of contingencies requested in the inputs
 */
class ContingencyValidationAlgorithm {
 public:
  /**
   * @brief Constructor
   * @param validContingencies The class keeping track of valid contingencies
   */
  explicit ContingencyValidationAlgorithm(ValidContingencies& validContingencies) : validContingencies_(validContingencies) {}

  /**
   * @brief Application operator.
   *
   * Visits the elements in the node and reports them as valid for contingencies
   *
   * @param node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  ValidContingencies& validContingencies_;  ///< the contingencies being validated by the algorithm
};
}  // namespace algo
}  // namespace dfl
