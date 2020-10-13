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
  dfl::inputs::Node node("0", 0.0);
  dfl::inputs::Node node0("0", 10.);
  dfl::inputs::Node node2("1", 4.5);

  ASSERT_TRUE(node == node0);
  ASSERT_TRUE(node0 != node2);
  ASSERT_FALSE(node != node0);
  ASSERT_FALSE(node == node2);
  ASSERT_TRUE(node < node2);
  ASSERT_TRUE(node <= node2);
  ASSERT_TRUE(node <= node0);
  ASSERT_TRUE(node2 > node);
  ASSERT_TRUE(node2 >= node);
  ASSERT_TRUE(node0 >= node);
}
