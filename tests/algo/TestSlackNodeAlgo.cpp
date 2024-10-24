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
 * @file  TestSlackNodeAlgo.cpp
 *
 * @brief Slack Node Algo library test file
 *
 */

#include "SlackNodeAlgorithm.h"
#include "Tests.h"

namespace test {
/**
 * @brief Implement Service manager interface stub for testing purpose
 *
 * This aims to stub the Dynawo service manager interface, to avoid using dynawo library during unit testing.
 * The idea is to represent the switches connections by a map of (busId, voltagelevelId) / otherBusId,
 * allowing to easy retrieve the list of bus connected by a switch to another bus in the same voltage level
 */
class TestAlgoServiceManagerInterface : public DYN::ServiceManagerInterface {
 public:
  using MapKey = std::tuple<std::string, std::string>;

 public:
  void clear() {
    map_.clear();
  }

  /**
   * @brief Add a switch connection
   *
   * this will perform the connection @p busId <=> @p otherbusid in voltage level
   *
   * @param busId the bus to connect to
   * @param VLId the voltage level id containing both buses
   * @param otherbusid the other bus id
   */
  void add(const std::string& busId, const std::string& VLId, const std::string& otherbusid) {
    map_[std::tie(busId, VLId)].push_back(otherbusid);
    map_[std::tie(otherbusid, VLId)].push_back(busId);
  }

  /**
   * @copydoc DYN::ServiceManagerInterface::getBusesConnectedBySwitch
   */
  std::vector<std::string> getBusesConnectedBySwitch(const std::string& busId, const std::string& VLId) const final {
    auto it = map_.find(std::tie(busId, VLId));
    if (it == map_.end()) {
      return {};
    }

    std::set<std::string> set;
    for (const auto& id : it->second) {
      updateSet(set, busId, VLId);
    }
    std::vector<std::string> ret(set.begin(), set.end());
    ret.erase(std::find(ret.begin(), ret.end(), busId));
    return ret;
  }

  /**
   * @copydoc DYN::ServiceManagerInterface::getRegulatedBus
   */
  std::shared_ptr<DYN::BusInterface> getRegulatedBus(const std::string& regulatingComponent) const final {
    return std::shared_ptr<DYN::BusInterface>();
  }

 private:
  void updateSet(std::set<std::string>& set, const std::string& str, const std::string& vlid) const {
    if (set.count(str) > 0) {
      return;
    }

    set.insert(str);
    auto it = map_.find(std::tie(str, vlid));
    if (it == map_.end()) {
      return;
    }

    for (const auto& id : it->second) {
      updateSet(set, id, vlid);
    }
  }

 private:
  std::map<MapKey, std::vector<std::string>> map_;
};
}  // namespace test

TEST(SlackNodeAlgo, Base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}), dfl::inputs::Node::build("1", vl, 1.0, {}), dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}), dfl::inputs::Node::build("4", vl, 4.0, {}), dfl::inputs::Node::build("5", vl, 5.0, {}),
      dfl::inputs::Node::build("6", vl, 0.0, {}),
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

TEST(SlackNodeAlgo, BaseNonFictitious) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  bool fictitious = true;
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}), dfl::inputs::Node::build("1", vl, 1.0, {}), dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}), dfl::inputs::Node::build("4", vl, 4.0, {}), dfl::inputs::Node::build("5", vl, 5.0, {}, fictitious),
      dfl::inputs::Node::build("6", vl, 0.0, {}),
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
  ASSERT_EQ("4", slack_node->id);  // Node 5 is discarded because it is fictitious
}

TEST(SlackNodeAlgo, neighbours) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}), dfl::inputs::Node::build("1", vl, 1.0, {}), dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}), dfl::inputs::Node::build("4", vl, 5.0, {}), dfl::inputs::Node::build("5", vl, 5.0, {}),
      dfl::inputs::Node::build("6", vl, 0.0, {}),
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
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}), dfl::inputs::Node::build("1", vl, 1.0, {}), dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}), dfl::inputs::Node::build("4", vl, 5.0, {}), dfl::inputs::Node::build("5", vl, 5.0, {}),
      dfl::inputs::Node::build("6", vl, 0.0, {}),
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
