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

#include "AlgorithmsResults.h"
#include "Contingencies.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HvdcLine.h"
#include "LoadDefinitionAlgorithm.h"
#include "Node.h"
#include "SVarCDefinitionAlgorithm.h"

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
  explicit ValidContingencies(const std::vector<inputs::Contingency> &contingencies);

  /**
   * @brief Mark the element given by id and type as valid in all contingencies where it is referred
   *
   * @param id the id of the element found in the network
   * @param type the type of the element as it has been found in the network
   * @param isNetwork if element is network model
   */
  void markElementValid(const ElementId &id, inputs::ContingencyElement::Type type, const bool isNetwork);

  /**
   * @brief Keep as valid contingencies only the ones that have all elements marked as valid
   */
  void keepContingenciesWithAllElementsValid();

  /**
   * @brief All valid contigencies
   * @return valid contingencies
   */
  const std::vector<dfl::inputs::Contingency> &get() { return validContingencies_; }

  /**
   * @brief Get the networkElements_ object
   *
   * @return set containing contingencies elements id using network model
   */
  const std::unordered_set<ElementId> &getNetworkElements() const { return networkElements_; }

 private:
  using ContingenciesRef = std::reference_wrapper<const std::vector<inputs::Contingency>>;     ///< Alias for a reference to the list of contingencies
  using ElementContingenciesMap = std::unordered_map<ElementId, std::vector<ContingencyRef>>;  ///< Alias for map of element contingencies
  using ElementIds = std::unordered_set<ElementId>;                                            ///< Alias for set of element ids
  using ValidatingContingenciesMap = std::unordered_map<ContingencyId, ElementIds>;            ///< Alias for map of contingencies with valid elements found

  ContingenciesRef contingencies_;                            ///< Contingencies requested in the inputs
  ElementContingenciesMap elementContingencies_;              ///< For each element identifier, all the contingencies where it is referenced
  ValidatingContingenciesMap validatingContingencies_;        ///< All contingencies with valid elements found, indexed by contingencyId
  std::vector<dfl::inputs::Contingency> validContingencies_;  ///< Only valid contingencies
  ElementIds networkElements_;                                ///< Set containing contingencies elements id using network model
};

/**
 * @brief Validation of contingencies requested in the inputs based on nodes data
 */
class ContingencyValidationAlgorithmOnNodes {
 public:
  /**
   * @brief Constructor
   * @param validContingencies The class keeping track of valid contingencies
   */
  explicit ContingencyValidationAlgorithmOnNodes(ValidContingencies &validContingencies) : validContingencies_(validContingencies) {}

  /**
   * @brief Application operator.
   *
   * Visits the elements in the node and reports them as valid for contingencies
   *
   * @param node the node to process
   * @param algoRes pointer to algorithms results class
   */
  void operator()(const NodePtr &node, std::shared_ptr<AlgorithmsResults> &algoRes);

 private:
  ValidContingencies &validContingencies_;  ///< the contingencies being validated by the algorithm
};

/**
 * @brief Validation of contingencies based on definitions data
 */
class ContingencyValidationAlgorithmOnDefs {
 public:
  /**
   * @brief Construct a new Contingency Validation Algorithm On Defs object
   *
   * @param validContingencies The class keeping track of valid contingencies
   */
  explicit ContingencyValidationAlgorithmOnDefs(ValidContingencies &validContingencies) : validContingencies_(validContingencies) {}

  /**
   * @brief reports algo definitions as valid contingencies
   *
   * @param loads list of load definitions
   * @param generators list of generator definitions
   * @param svarcs list of staticVarCompensator definitions
   */
  void fillValidContingenciesOnDefs(const std::vector<algo::LoadDefinition> &loads, const std::vector<algo::GeneratorDefinition> &generators,
                                    const std::vector<algo::StaticVarCompensatorDefinition> &svarcs);

 private:
  ValidContingencies &validContingencies_;  ///< the contingencies being validated by the algorithm
};
}  // namespace algo
}  // namespace dfl
