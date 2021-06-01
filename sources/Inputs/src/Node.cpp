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
Node::build(const NodeId& id, const std::shared_ptr<VoltageLevel>& vl, double nominalVoltage, unsigned int nbShunts) {
  auto ret = std::shared_ptr<Node>(new Node(id, vl, nominalVoltage, nbShunts));
  vl->nodes.push_back(ret);
  return ret;
}

Node::Node(const NodeId& idNode, const std::shared_ptr<VoltageLevel> vl, double nominalVoltageNode, unsigned int nbShunts) :
    id(idNode),
    voltageLevel(vl),
    nominalVoltage{nominalVoltageNode},
    nbShunts{nbShunts},
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

/////////////////////////////////////////////////

std::shared_ptr<Line>
Line::build(const LineId& lineId, const std::shared_ptr<Node>& node1, const std::shared_ptr<Node>& node2) {
  auto ret = std::shared_ptr<Line>(new Line(lineId, node1, node2));
  node1->neighbours.push_back(node2);
  node2->neighbours.push_back(node1);
  node1->lines.push_back(ret);
  node2->lines.push_back(ret);

  return ret;
}

Line::Line(const LineId& lineId, const std::shared_ptr<Node>& node1, const std::shared_ptr<Node>& node2) : id(lineId), nodes{node1, node2} {}

///////////////////////////////////////////////////

std::shared_ptr<Tfo>
Tfo::build(const TfoId& tfoId, const std::shared_ptr<Node>& node1, const std::shared_ptr<Node>& node2) {
  auto ret = std::shared_ptr<Tfo>(new Tfo(tfoId, node1, node2));

  node1->neighbours.push_back(node2);
  node2->neighbours.push_back(node1);

  node1->tfos.push_back(ret);
  node2->tfos.push_back(ret);

  return ret;
}

std::shared_ptr<Tfo>
Tfo::build(const TfoId& tfoId, const std::shared_ptr<Node>& node1, const std::shared_ptr<Node>& node2, const std::shared_ptr<Node>& node3) {
  auto ret = std::shared_ptr<Tfo>(new Tfo(tfoId, node1, node2, node3));

  node1->neighbours.push_back(node2);
  node1->neighbours.push_back(node3);

  node2->neighbours.push_back(node1);
  node2->neighbours.push_back(node3);

  node3->neighbours.push_back(node1);
  node3->neighbours.push_back(node2);

  node1->tfos.push_back(ret);
  node2->tfos.push_back(ret);
  node3->tfos.push_back(ret);

  return ret;
}

Tfo::Tfo(const TfoId tfoId, const std::shared_ptr<Node>& node1, const std::shared_ptr<Node>& node2) : id(tfoId), nodes{node1, node2} {}

Tfo::Tfo(const TfoId tfoId, const std::shared_ptr<Node>& node1, const std::shared_ptr<Node>& node2, const std::shared_ptr<Node>& node3) :
    id(tfoId),
    nodes{node1, node2, node3} {}

}  // namespace inputs
}  // namespace dfl
