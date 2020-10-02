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

#include <algorithm>
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

  std::for_each(nodes.begin(), nodes.end(), algo);

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

  std::for_each(nodes.begin(), nodes.end(), algo);

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

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_NE(nullptr, slack_node);
  ASSERT_EQ("4", slack_node->id);  // first found
}

TEST(Connexity, base) {
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      std::make_shared<dfl::inputs::Node>("0", 0.0), std::make_shared<dfl::inputs::Node>("1", 1.0), std::make_shared<dfl::inputs::Node>("2", 2.0),
      std::make_shared<dfl::inputs::Node>("3", 3.0), std::make_shared<dfl::inputs::Node>("4", 5.0), std::make_shared<dfl::inputs::Node>("5", 5.0),
      std::make_shared<dfl::inputs::Node>("6", 0.0),
  };
  std::vector<dfl::inputs::Node::NodeId> expected_nodes{"0", "1", "2", "3"};

  nodes[0]->neighbours.push_back(nodes[1]);
  nodes[0]->neighbours.push_back(nodes[2]);
  nodes[2]->neighbours.push_back(nodes[3]);
  nodes[1]->neighbours.push_back(nodes[0]);
  nodes[2]->neighbours.push_back(nodes[0]);
  nodes[3]->neighbours.push_back(nodes[2]);

  nodes[4]->neighbours.push_back(nodes[5]);
  nodes[5]->neighbours.push_back(nodes[6]);
  nodes[5]->neighbours.push_back(nodes[4]);
  nodes[6]->neighbours.push_back(nodes[5]);

  dfl::algo::MainConnexComponentAlgorithm::ConnexGroup main;
  dfl::algo::MainConnexComponentAlgorithm algo(main);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(4, main.size());
  std::vector<dfl::inputs::Node::NodeId> nodeids_main;
  std::for_each(main.begin(), main.end(), [&nodeids_main](const std::shared_ptr<dfl::inputs::Node>& node) { nodeids_main.push_back(node->id); });
  std::sort(nodeids_main.begin(), nodeids_main.end());
  ASSERT_EQ(expected_nodes, nodeids_main);
}
