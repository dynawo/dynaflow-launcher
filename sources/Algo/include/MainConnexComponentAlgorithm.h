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
 * @file  MainConnexComponentAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for determination of main connected component header file
 *
 */

#pragma once

#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief Algorithm to determine largest connex component
 */
class MainConnexComponentAlgorithm {
 public:
  using ConnexGroup = std::vector<NodePtr>;  ///< Alias for group of nodes

 public:
  /**
   * @brief Constructor
   *
   * @param mainConnexity main connex component to update
   */
  explicit MainConnexComponentAlgorithm(ConnexGroup& mainConnexity);

  /**
   * @brief Perform algorithm
   *
   * For each node, we determine, by going through its neighbours, which other nodes are connexs
   * and we mark the ones we already processed to avoid processing them again.
   *
   *  @brief node the node to process
   */
  void operator()(const NodePtr& node);

 private:
  /**
  * @brief Equality Comparator for nodes
  */
  struct EqualCompareNode {
    /**
     * @brief invocable operator
     *
     * performs @p lhs == @p rhs by relying on Node equality operator
     *
     * @param lhs first node
     * @param rhs second node
     *
     * @returns comparaison status
     */
    bool operator()(const NodePtr& lhs, const NodePtr& rhs) const {
      return (*lhs) == (*rhs);
    }
  };

 private:
  /**
  * @brief Update connexity group recursively
  *
  * Update group with all nodes in inputs and their neighbours
  *
  * @param group the group to update
  * @param nodes the nodes to add to the group
  */
  void updateConnexGroup(ConnexGroup& group, const std::vector<NodePtr>& nodes);

 private:
  std::unordered_set<NodePtr, std::hash<NodePtr>, EqualCompareNode> markedNodes_;  ///< the set of marked nodes for the algorithm
  ConnexGroup& mainConnexity_;                                                     ///< the main connex component to update
};
}  // namespace algo
}  // namespace dfl
