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
  auto node = dfl::inputs::Node::build("0", vl, 0.0, 1);
  auto node0 = dfl::inputs::Node::build("0", vl, 10., 5);
  auto node2 = dfl::inputs::Node::build("1", vl, 4.5, 1);

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
  auto node0 = dfl::inputs::Node::build("0", vl, 0.0, 0);
  auto node1 = dfl::inputs::Node::build("1", vl, 10., 1);
  auto node2 = dfl::inputs::Node::build("2", vl, 4.5, 2);

  auto line = dfl::inputs::Line::build("LINE", node0, node1);
  auto line2 = dfl::inputs::Line::build("LINE", node1, node2);
  ASSERT_EQ(node0->nbShunts, 0);
  ASSERT_EQ(node1->nbShunts, 1);
  ASSERT_EQ(node2->nbShunts, 2);
  ASSERT_EQ(node0->neighbours.size(), 1);
  ASSERT_EQ(node1->neighbours.size(), 2);
  ASSERT_EQ(node2->neighbours.size(), 1);
}
