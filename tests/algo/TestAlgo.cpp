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
 * @file  TestAlgo.cpp
 *
 * @brief Algo library test file
 *
 */

#include "Algo.h"
#include "Tests.h"

#include <vector>

TEST(SlackNodeAlgo, Base) {
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      std::make_shared<dfl::inputs::Node>("0", 0.0), std::make_shared<dfl::inputs::Node>("1", 1.0), std::make_shared<dfl::inputs::Node>("2", 2.0),
      std::make_shared<dfl::inputs::Node>("3", 3.0), std::make_shared<dfl::inputs::Node>("4", 4.0), std::make_shared<dfl::inputs::Node>("5", 5.0),
      std::make_shared<dfl::inputs::Node>("6", 0.0),
  };

  nodes[0]->neighbours.push_back(nodes[1]);
  nodes[0]->neighbours.push_back(nodes[2]);
  nodes[0]->neighbours.push_back(nodes[3]);

  nodes[4]->neighbours.push_back(nodes[1]);
  nodes[4]->neighbours.push_back(nodes[2]);
  nodes[4]->neighbours.push_back(nodes[3]);

  std::shared_ptr<dfl::inputs::Node> slack_node;
  dfl::algo::SlackNodeAlgorithm algo(slack_node);

  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    algo(*it);
  }

  ASSERT_NE(nullptr, slack_node);
  ASSERT_EQ("5", slack_node->id);
}

TEST(SlackNodeAlgo, neighbours) {
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      std::make_shared<dfl::inputs::Node>("0", 0.0), std::make_shared<dfl::inputs::Node>("1", 1.0), std::make_shared<dfl::inputs::Node>("2", 2.0),
      std::make_shared<dfl::inputs::Node>("3", 3.0), std::make_shared<dfl::inputs::Node>("4", 5.0), std::make_shared<dfl::inputs::Node>("5", 5.0),
      std::make_shared<dfl::inputs::Node>("6", 0.0),
  };

  nodes[0]->neighbours.push_back(nodes[1]);
  nodes[0]->neighbours.push_back(nodes[2]);
  nodes[0]->neighbours.push_back(nodes[3]);

  nodes[4]->neighbours.push_back(nodes[1]);
  nodes[4]->neighbours.push_back(nodes[2]);
  nodes[4]->neighbours.push_back(nodes[3]);

  std::shared_ptr<dfl::inputs::Node> slack_node;
  dfl::algo::SlackNodeAlgorithm algo(slack_node);

  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    algo(*it);
  }

  ASSERT_NE(nullptr, slack_node);
  ASSERT_EQ("4", slack_node->id);
}

TEST(SlackNodeAlgo, equivalent) {
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      std::make_shared<dfl::inputs::Node>("0", 0.0), std::make_shared<dfl::inputs::Node>("1", 1.0), std::make_shared<dfl::inputs::Node>("2", 2.0),
      std::make_shared<dfl::inputs::Node>("3", 3.0), std::make_shared<dfl::inputs::Node>("4", 5.0), std::make_shared<dfl::inputs::Node>("5", 5.0),
      std::make_shared<dfl::inputs::Node>("6", 0.0),
  };

  nodes[5]->neighbours.push_back(nodes[1]);
  nodes[5]->neighbours.push_back(nodes[2]);
  nodes[5]->neighbours.push_back(nodes[3]);

  nodes[4]->neighbours.push_back(nodes[1]);
  nodes[4]->neighbours.push_back(nodes[2]);
  nodes[4]->neighbours.push_back(nodes[3]);

  std::shared_ptr<dfl::inputs::Node> slack_node;
  dfl::algo::SlackNodeAlgorithm algo(slack_node);

  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    algo(*it);
  }

  ASSERT_NE(nullptr, slack_node);
  ASSERT_EQ("4", slack_node->id);  // first found
}
