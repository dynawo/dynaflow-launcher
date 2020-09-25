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

#include <boost/shared_ptr.hpp>
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
class Node {
 public:
  using NodeId = std::string;  ///< node id definition

 public:
  /**
   * @brief Constructor
   *
   * @param id the node id
   * @param voltageLevel the voltage level associated with the node
   */
  Node(const NodeId& id, double voltageLevel);

  /**
   * @brief Retrieves the node id
   *
   * @returns node id
   */
  const NodeId& id() const {
    return id_;
  }

  /**
   * @brief Retrieves the voltage level
   *
   * @returns voltage level
   */
  double voltageLevel() const {
    return voltageLevel_;
  }

  /**
   * @brief Retrieves the neighbours of the node
   *
   * @returns the list of neighbours
   */
  const std::vector<boost::shared_ptr<Node>>& neighbours() const {
    return neighbours_;
  }

  /**
   * @brief Add a neighbour
   *
   * @param node the new neighbour
   */
  void addNeighbour(boost::shared_ptr<Node> node) {
    neighbours_.push_back(node);
  }

 private:
  NodeId id_;                                        ///< node id
  double voltageLevel_;                              ///< voltage level of the node
  std::vector<boost::shared_ptr<Node>> neighbours_;  ///< list of neighbours
};

}  // namespace inputs
}  // namespace dfl
