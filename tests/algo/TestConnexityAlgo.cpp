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
 * @file  TestConnexityAlgo.cpp
 *
 * @brief MainConnexComponentAlgorithm library test file
 */

#include "MainConnexComponentAlgorithm.h"
#include "Tests.h"

TEST(Connexity, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}), dfl::inputs::Node::build("1", vl, 1.0, {}), dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}), dfl::inputs::Node::build("4", vl, 5.0, {}), dfl::inputs::Node::build("5", vl, 5.0, {}),
      dfl::inputs::Node::build("6", vl, 0.0, {}),
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
  std::for_each(main.begin(), main.end(), [&nodeids_main](const std::shared_ptr<dfl::inputs::Node> &node) { nodeids_main.push_back(node->id); });
  std::sort(nodeids_main.begin(), nodeids_main.end());
  ASSERT_EQ(expected_nodes, nodeids_main);
}

TEST(Connexity, SameSize) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{dfl::inputs::Node::build("0", vl, 0.0, {}), dfl::inputs::Node::build("1", vl, 1.0, {}),
                                                        dfl::inputs::Node::build("2", vl, 2.0, {}), dfl::inputs::Node::build("3", vl, 3.0, {}),
                                                        dfl::inputs::Node::build("4", vl, 5.0, {}), dfl::inputs::Node::build("5", vl, 5.0, {})};
  std::vector<dfl::inputs::Node::NodeId> expected_nodes{"0", "1", "2"};

  nodes[0]->neighbours.push_back(nodes[1]);
  nodes[0]->neighbours.push_back(nodes[2]);
  nodes[1]->neighbours.push_back(nodes[0]);
  nodes[2]->neighbours.push_back(nodes[0]);

  nodes[3]->neighbours.push_back(nodes[4]);
  nodes[3]->neighbours.push_back(nodes[5]);
  nodes[4]->neighbours.push_back(nodes[3]);
  nodes[5]->neighbours.push_back(nodes[3]);

  dfl::algo::MainConnexComponentAlgorithm::ConnexGroup main;
  dfl::algo::MainConnexComponentAlgorithm algo(main);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(3, main.size());
  std::vector<dfl::inputs::Node::NodeId> nodeids_main;
  std::for_each(main.begin(), main.end(), [&nodeids_main](const std::shared_ptr<dfl::inputs::Node> &node) { nodeids_main.push_back(node->id); });
  std::sort(nodeids_main.begin(), nodeids_main.end());
  ASSERT_EQ(expected_nodes, nodeids_main);
}

TEST(Connexity, notRetainedSwitch) {
  using dfl::inputs::NetworkManager;
  NetworkManager manager("res/IEEE14_disconnected_shunts.iidm");

  dfl::algo::MainConnexComponentAlgorithm::ConnexGroup main;
  dfl::algo::MainConnexComponentAlgorithm algo(main);
  manager.onNode(algo);
  manager.walkNodes();

  ASSERT_EQ(10, main.size());
}
