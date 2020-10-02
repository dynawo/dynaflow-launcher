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
 * @file  Node.h
 *
 * @brief Node structure header file
 *
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace dfl {
/// @brief Namespace for inputs of Dynaflow launcher
namespace inputs {

/**
 * @brief topological node structure
 *
 * This implement a graph node concept. It contains only the information required to perform the algorithms and not all information extractable from network file
 */
struct Node {
  using NodeId = std::string;  ///< node id definition

  /**
   * @brief Constructor
   *
   * @param id the node id
   * @param nominalVoltage the voltage level associated with the node
   */
  Node(const NodeId& id, double nominalVoltage);

  const NodeId id;                                ///< node id
  const double nominalVoltage;                    ///< Nominal voltage of the node
  std::vector<std::shared_ptr<Node>> neighbours;  ///< list of neighbours
};

/**
 * @brief Determines if two nodes are equal
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns sttaus of the comparaison
 */
bool operator==(const Node& lhs, const Node& rhs);

/**
 * @brief Determines if two nodes are different
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns sttaus of the comparaison
 */
bool operator!=(const Node& lhs, const Node& rhs);

/**
 * @brief Determines if a node is inferior to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns sttaus of the comparaison
 */
bool operator<(const Node& lhs, const Node& rhs);

/**
 * @brief Determines if a node is inferior or equal to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns sttaus of the comparaison
 */
bool operator<=(const Node& lhs, const Node& rhs);

/**
 * @brief Determines if a node is superior to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns sttaus of the comparaison
 */
bool operator>(const Node& lhs, const Node& rhs);

/**
 * @brief Determines if a node is superior or equal to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns sttaus of the comparaison
 */
bool operator>=(const Node& lhs, const Node& rhs);

}  // namespace inputs
}  // namespace dfl
