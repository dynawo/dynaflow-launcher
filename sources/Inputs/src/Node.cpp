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
 * @file  Node.cpp
 *
 * @brief Node structure implementation file
 *
 */

#include "Node.h"

namespace dfl {
namespace inputs {

std::shared_ptr<Node>
Node::build(const NodeId& id, const std::shared_ptr<VoltageLevel>& vl, double nominalVoltage) {
  auto ret = std::shared_ptr<Node>(new Node(id, vl, nominalVoltage));
  vl->nodes.push_back(ret);
  return ret;
}

Node::Node(const NodeId& idNode, const std::shared_ptr<VoltageLevel> vl, double nominalVoltageNode) :
    id(idNode),
    voltageLevel(vl),
    nominalVoltage{nominalVoltageNode},
    neighbours{} {}

bool
operator==(const Node& lhs, const Node& rhs) {
  return lhs.id == rhs.id;
}

bool
operator!=(const Node& lhs, const Node& rhs) {
  return !(lhs == rhs);
}

bool
operator<(const Node& lhs, const Node& rhs) {
  return lhs.id < rhs.id;
}

bool
operator<=(const Node& lhs, const Node& rhs) {
  return (lhs < rhs) || (lhs == rhs);
}

bool
operator>(const Node& lhs, const Node& rhs) {
  return !(lhs <= rhs);
}

bool
operator>=(const Node& lhs, const Node& rhs) {
  return !(lhs < rhs);
}

/////////////////////////////////////////////////

VoltageLevel::VoltageLevel(const VoltageLevelId& vlid) : id(vlid) {}

}  // namespace inputs
}  // namespace dfl
