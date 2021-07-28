//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Node.h"
#include "Tests.h"

TEST(TestNode, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("0", vl, 0.0, {});
  auto node0 = dfl::inputs::Node::build("0", vl, 10., {});
  auto node2 = dfl::inputs::Node::build("1", vl, 4.5, {});

  ASSERT_TRUE(*node == *node0);
  ASSERT_TRUE(*node0 != *node2);
  ASSERT_FALSE(*node != *node0);
  ASSERT_FALSE(*node == *node2);
  ASSERT_TRUE(*node < *node2);
  ASSERT_TRUE(*node <= *node2);
  ASSERT_TRUE(*node <= *node0);
  ASSERT_TRUE(*node2 > *node);
  ASSERT_TRUE(*node2 >= *node);
  ASSERT_TRUE(*node0 >= *node);
}

TEST(TestNode, line) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  auto node0 = dfl::inputs::Node::build("0", vl, 0.0, {});
  auto node1 = dfl::inputs::Node::build("1", vl, 10., shunts1);
  auto node2 = dfl::inputs::Node::build("2", vl, 4.5, shunts2);

  auto line = dfl::inputs::Line::build("LINE", node0, node1, "ETE");
  auto line2 = dfl::inputs::Line::build("LINE", node1, node2, "UNDEFINED");
  ASSERT_EQ(node0->shunts.size(), 0);
  ASSERT_EQ(node1->shunts.size(), 1);
  ASSERT_EQ(node2->shunts.size(), 2);
  ASSERT_EQ(node0->neighbours.size(), 1);
  ASSERT_EQ(node1->neighbours.size(), 2);
  ASSERT_EQ(node2->neighbours.size(), 1);
}

TEST(TestNode, Tfo) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  auto node0 = dfl::inputs::Node::build("0", vl, 0.0, {});
  auto node1 = dfl::inputs::Node::build("1", vl, 10., {});

  auto node00 = dfl::inputs::Node::build("0", vl, 0.0, {});
  auto node01 = dfl::inputs::Node::build("1", vl, 10., {});
  auto node02 = dfl::inputs::Node::build("2", vl, 4.5, {});

  auto tfo = dfl::inputs::Tfo::build("TFO", node0, node1);
  ASSERT_EQ(node0->neighbours.size(), 1);
  auto tfo2 = dfl::inputs::Tfo::build("TFO", node00, node01, node02);
  ASSERT_EQ(node00->neighbours.size(), 2);
  ASSERT_EQ(node01->neighbours.size(), 2);
  ASSERT_EQ(node02->neighbours.size(), 2);
}
