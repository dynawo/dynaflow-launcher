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

#include "Configuration.h"
#include "ContingencyValidationAlgorithm.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "LoadDefinitionAlgorithm.h"
#include "MainConnexComponentAlgorithm.h"
#include "NetworkManager.h"
#include "SVarCDefinitionAlgorithm.h"
#include "ShuntDefinitionAlgorithm.h"
#include "SlackNodeAlgorithm.h"
#include "Tests.h"

#include <DYNBusInterface.h>
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/optional.hpp>
#include <map>
#include <set>
#include <tuple>
#include <vector>

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
  boost::shared_ptr<DYN::BusInterface> getRegulatedBus(const std::string& regulatingComponent) const final {
    return boost::shared_ptr<DYN::BusInterface>();
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
  std::for_each(main.begin(), main.end(), [&nodeids_main](const std::shared_ptr<dfl::inputs::Node>& node) { nodeids_main.push_back(node->id); });
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
  std::for_each(main.begin(), main.end(), [&nodeids_main](const std::shared_ptr<dfl::inputs::Node>& node) { nodeids_main.push_back(node->id); });
  std::sort(nodeids_main.begin(), nodeids_main.end());
  ASSERT_EQ(expected_nodes, nodeids_main);
}

static void
generatorsEquals(const dfl::algo::GeneratorDefinition& lhs, const dfl::algo::GeneratorDefinition& rhs) {
  ASSERT_EQ(lhs.id, rhs.id);
  ASSERT_EQ(lhs.model, rhs.model);
  ASSERT_EQ(lhs.points.size(), rhs.points.size());
  ASSERT_EQ(lhs.qmin, rhs.qmin);
  ASSERT_EQ(lhs.qmax, rhs.qmax);
  ASSERT_EQ(lhs.pmin, rhs.pmin);
  ASSERT_EQ(lhs.pmax, rhs.pmax);
  for (size_t index_p = 0; index_p < lhs.points.size(); ++index_p) {
    ASSERT_EQ(lhs.points[index_p].p, rhs.points[index_p].p);
    ASSERT_EQ(lhs.points[index_p].qmax, rhs.points[index_p].qmax);
    ASSERT_EQ(lhs.points[index_p].qmin, rhs.points[index_p].qmin);
  }
}

TEST(Generators, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}),  dfl::inputs::Node::build("1", vl, 1.0, {}),  dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}),  dfl::inputs::Node::build("4", vl2, 5.0, {}), dfl::inputs::Node::build("5", vl2, 5.0, {}),
      dfl::inputs::Node::build("6", vl2, 0.0, {}),
  };

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));

  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  const std::string bus3 = "BUS_3";
  const std::string bus4 = "BUS_4";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN, "0", points0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN, "0", points, -1, 1, -1, 1, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN, "2", points, -2, 2, -2, 2, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN, "4", points, -5, 5, -5, 5, 0, bus3)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "0", points0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "0", points, -1, 1, -1, 1, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "2", points, -2, 2, -2, 2, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "4", points, -5, 5, -5, 5, 0, bus3)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, bus1, bus3);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, bus2, bus2);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, bus3, bus2);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::algo::GeneratorDefinitionAlgorithm::BusGenMap busesWithDynamicModel;
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busesWithDynamicModel, busMap, true, testServiceManager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  busesWithDynamicModel.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busesWithDynamicModel, busMap, false, testServiceManager);

  for (const auto& node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}
TEST(Generators, noGeneratorRegulating) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{dfl::inputs::Node::build("0", vl, 0.0, {})};

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});

  const std::string bus1 = "BUS_1";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "0", points, 0, 0, 0, 0, 0, bus1)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "0", points, 0, 0, 0, 0, 0, bus1)};

  nodes[0]->generators.emplace_back("01", false, points, 0, 0, 0, 0, 0, bus1, bus1);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap;
  dfl::algo::GeneratorDefinitionAlgorithm::BusGenMap busesWithDynamicModel;
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busesWithDynamicModel, busMap, true, testServiceManager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(1, generators.size());
  ASSERT_FALSE(algoRes->isAtLeastOneGeneratorRegulating);

  generators.clear();
  busesWithDynamicModel.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busesWithDynamicModel, busMap, false, testServiceManager);

  for (const auto& node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(1, generators.size());
  ASSERT_FALSE(algoRes->isAtLeastOneGeneratorRegulating);
}

TEST(Generators, SwitchConnexity) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}),  dfl::inputs::Node::build("1", vl, 1.0, {}),  dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}),  dfl::inputs::Node::build("4", vl2, 5.0, {}), dfl::inputs::Node::build("5", vl2, 5.0, {}),
      dfl::inputs::Node::build("6", vl2, 0.0, {}),
  };

  testServiceManager->add("0", "VL", "1");
  testServiceManager->add("1", "VL", "2");
  testServiceManager->add("4", "VL2", "5");

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));
  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  const std::string bus3 = "BUS_3";

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN, "0", points0, -1, 1, -1, 1, 1,
                                     bus1),  // due to switch connexity
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN, "2", points, -2, 2, -2, 2, 2,
                                     bus2),  // due to switch connexity
      dfl::algo::GeneratorDefinition("04", dfl::algo::GeneratorDefinition::ModelType::SIGNALN, "4", points, -5, 5, -5, 5, 5, bus3),
  };

  nodes[0]->generators.emplace_back("00", true, points0, -1, 1, -1, 1, 1, bus1, bus1);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 2, bus2, bus2);

  nodes[4]->generators.emplace_back("04", true, points, -5, 5, -5, 5, 5, bus3, bus3);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::algo::GeneratorDefinitionAlgorithm::BusGenMap busesWithDynamicModel;
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busesWithDynamicModel, busMap, true, testServiceManager);

  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(3, generators.size());
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
  }
}

TEST(Loads, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  using dfl::algo::LoadDefinition;
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{dfl::inputs::Node::build("0", vl, 98.0, {}), dfl::inputs::Node::build("1", vl, 111.0, {}),
                                                        dfl::inputs::Node::build("2", vl, 24.0, {}), dfl::inputs::Node::build("3", vl, 63.0, {}),
                                                        dfl::inputs::Node::build("4", vl, 56.0, {}), dfl::inputs::Node::build("5", vl, 46.0, {})};

  dfl::algo::LoadDefinitionAlgorithm::Loads expected_loads = {LoadDefinition("00", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "0"),
                                                              LoadDefinition("01", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "0"),
                                                              LoadDefinition("02", LoadDefinition::ModelType::NETWORK, "0"),
                                                              LoadDefinition("03", LoadDefinition::ModelType::NETWORK, "0"),
                                                              LoadDefinition("04", LoadDefinition::ModelType::NETWORK, "2"),
                                                              LoadDefinition("05", LoadDefinition::ModelType::NETWORK, "4")};

  nodes[0]->loads.emplace_back("00", false, false);
  nodes[0]->loads.emplace_back("01", false, false);
  nodes[0]->loads.emplace_back("02", true, false);
  nodes[0]->loads.emplace_back("03", false, true);

  nodes[2]->loads.emplace_back("04", false, false);

  nodes[4]->loads.emplace_back("05", false, false);

  dfl::algo::LoadDefinitionAlgorithm::Loads loads;
  double dsoVoltageLevel = 63.0;
  dfl::algo::LoadDefinitionAlgorithm algo(loads, dsoVoltageLevel);

  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo(node, algoRes);
  }

  ASSERT_EQ(6, loads.size());
  for (size_t index = 0; index < loads.size(); ++index) {
    ASSERT_EQ(expected_loads[index].id, loads[index].id);
    ASSERT_EQ(expected_loads[index].modelType, loads[index].modelType);
  }
}

static bool
hvdcLineDefinitionEqual(const dfl::algo::HVDCDefinition& lhs, const dfl::algo::HVDCDefinition& rhs) {
  // we do not check the model here as a dedicated test will check the compliance
  return lhs.id == rhs.id && lhs.converterType == rhs.converterType && lhs.converter1Id == rhs.converter1Id && lhs.converter1BusId == rhs.converter1BusId &&
         lhs.converter2VoltageRegulationOn == rhs.converter2VoltageRegulationOn && lhs.converter2Id == rhs.converter2Id &&
         lhs.converter2BusId == rhs.converter2BusId && lhs.converter2VoltageRegulationOn == rhs.converter2VoltageRegulationOn && lhs.position == rhs.position &&
         lhs.pMax == rhs.pMax && lhs.powerFactors == rhs.powerFactors && boost::equal_pointees(lhs.vscDefinition1, rhs.vscDefinition1) &&
         boost::equal_pointees(lhs.vscDefinition2, rhs.vscDefinition2);
}

TEST(HvdcLine, base) {
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0, {}), dfl::inputs::Node::build("1", vl, 111.0, {}), dfl::inputs::Node::build("2", vl, 24.0, {}),
      dfl::inputs::Node::build("3", vl, 63.0, {}), dfl::inputs::Node::build("4", vl, 56.0, {}),  dfl::inputs::Node::build("5", vl, 46.0, {}),
      dfl::inputs::Node::build("6", vl, 0.0, {}),
  };
  auto dummyStation = std::make_shared<dfl::inputs::LCCConverter>("StationN", "_BUS___99_TN", nullptr, 99.);
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "_BUS___99_TN", nullptr, false, 0., 0.,
                                                                     std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto lccStation1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation1", "_BUS___11_TN", nullptr, 1.);
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "_BUS___11_TN", nullptr, false, 0., 0.,
                                                                 std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto lccStationMain1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStationMain1", "_BUS__11_TN", nullptr, 1.);

  auto lccStationMain2 = std::make_shared<dfl::inputs::LCCConverter>("LCCStationMain2", "_BUS__11_TN", nullptr, 2.);

  auto hvdcLineLCC = dfl::inputs::HvdcLine::build("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation1, dummyStation, boost::none, 0.0);
  auto hvdcLineVSC = dfl::inputs::HvdcLine::build("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, dummyStationVSC, vscStation2, boost::none, 10.);
  auto hvdcLineBothInMainComponent =
      dfl::inputs::HvdcLine::build("HVDCLineBothInMain", dfl::inputs::HvdcLine::ConverterType::LCC, lccStationMain1, lccStationMain2, boost::none, 20.);

  // model not checked in this test : see the dedicated test
  std::vector<dfl::algo::HVDCDefinition> expected_hvdcLines = {
      dfl::algo::HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", boost::none, "StationN",
                                "_BUS___99_TN", boost::none, dfl::algo::HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT,
                                dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling, {1., 99.}, 0., boost::none, boost::none, boost::none),
      dfl::algo::HVDCDefinition(
          "HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "StationN", "_BUS___99_TN", false, "VSCStation2", "_BUS___11_TN", false,
          dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling, {0., 0.}, 10.,
          dfl::algo::VSCDefinition(dummyStationVSC->converterId, dummyStationVSC->qMax, dummyStationVSC->qMin, 10., dummyStationVSC->points),
          dfl::algo::VSCDefinition(vscStation2->converterId, vscStation2->qMax, vscStation2->qMin, 10., vscStation2->points), boost::none),
      dfl::algo::HVDCDefinition("HVDCLineBothInMain", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStationMain1", "_BUS__11_TN", boost::none,
                                "LCCStationMain2", "_BUS__11_TN", boost::none, dfl::algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT,
                                dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling, {1., 2.}, 20., boost::none, boost::none, boost::none),
  };

  nodes[0]->converters.emplace_back(lccStation1);
  nodes[2]->converters.emplace_back(lccStationMain1);
  nodes[0]->converters.emplace_back(lccStationMain2);
  nodes[4]->converters.emplace_back(vscStation2);

  dfl::algo::HVDCLineDefinitions hvdcDefs;
  constexpr bool useReactiveLimits = true;
  dfl::inputs::NetworkManager::BusMapRegulating map;
  std::unordered_set<std::shared_ptr<dfl::inputs::Converter>> set{vscStation2};
  dfl::algo::HVDCDefinitionAlgorithm algo(hvdcDefs, useReactiveLimits, set, map, testServiceManager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo(node, algoRes);
  }

  const auto& hvdcLines = hvdcDefs.hvdcLines;
  ASSERT_EQ(3, hvdcLines.size());
  size_t index = 0;
  for (const auto& expected_hvdcLine : expected_hvdcLines) {
    auto it = hvdcLines.find(expected_hvdcLine.id);
    ASSERT_NE(hvdcLines.end(), it);
    ASSERT_TRUE(hvdcLineDefinitionEqual(expected_hvdcLine, it->second)) << " Fail for " << expected_hvdcLine.id;
  }
  ASSERT_EQ(hvdcDefs.vscBusVSCDefinitionsMap.size(), 0);
}

static bool
compareVSCDefinition(const dfl::algo::VSCDefinition& lhs, const dfl::algo::VSCDefinition& rhs) {
  return lhs.id == rhs.id && lhs.qmax == rhs.qmax && lhs.qmin == rhs.qmin && lhs.points.size() == rhs.points.size() &&
         std::equal(lhs.points.begin(), lhs.points.end(), rhs.points.begin(),
                    [](const dfl::algo::VSCDefinition::ReactiveCurvePoint& lhs, const dfl::algo::VSCDefinition::ReactiveCurvePoint& rhs) {
                      return lhs.p == rhs.p && lhs.qmax == rhs.qmax && lhs.qmin == rhs.qmin;
                    });
}

static void
checkVSCIds(const dfl::algo::HVDCLineDefinitions& hvdcDefs) {
  static const dfl::algo::HVDCLineDefinitions::BusVSCMap expectedMap = {
      std::make_pair("2", dfl::algo::VSCDefinition("VSCStation2", 2.1, 2., 2., {})),
      std::make_pair("7", dfl::algo::VSCDefinition("VSCStation7", 7.1, 7., 2.7, {})),
      std::make_pair("8", dfl::algo::VSCDefinition("VSCStation8", 8.1, 8., 2.8, {})),
      std::make_pair("11", dfl::algo::VSCDefinition("VSCStation11", 11.1, 11., 11., {})),
      std::make_pair("12", dfl::algo::VSCDefinition("VSCStation12", 12.1, 12., 12., {})),
  };
  ASSERT_EQ(hvdcDefs.vscBusVSCDefinitionsMap.size(), expectedMap.size());
  for (const auto& pair : expectedMap) {
    ASSERT_GT(hvdcDefs.vscBusVSCDefinitionsMap.count(pair.first), 0);
    ASSERT_TRUE(compareVSCDefinition(hvdcDefs.vscBusVSCDefinitionsMap.at(pair.first), pair.second));
  }
}

TEST(hvdcLine, models) {
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0, {}), dfl::inputs::Node::build("1", vl, 111.0, {}), dfl::inputs::Node::build("2", vl, 24.0, {}),
      dfl::inputs::Node::build("3", vl, 63.0, {}), dfl::inputs::Node::build("4", vl, 56.0, {}),  dfl::inputs::Node::build("5", vl, 46.0, {}),
      dfl::inputs::Node::build("6", vl, 0.0, {}),  dfl::inputs::Node::build("7", vl, 0.0, {}),   dfl::inputs::Node::build("8", vl, 0.0, {}),
      dfl::inputs::Node::build("9", vl, 0.0, {}),  dfl::inputs::Node::build("10", vl, 0.0, {}),  dfl::inputs::Node::build("11", vl, 0.0, {}),
      dfl::inputs::Node::build("12", vl, 0.0, {}),
  };
  std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint> emptyPoints{};

  auto activeControl = boost::optional<dfl::inputs::HvdcLine::ActivePowerControl>(dfl::inputs::HvdcLine::ActivePowerControl(10., 5.));

  auto dummyStation = std::make_shared<dfl::inputs::LCCConverter>("StationN", "_BUS___99_TN", nullptr, 1.);
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "_BUS___99_TN", nullptr, false, 0., 0., emptyPoints);
  auto lccStation1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation1", "0", nullptr, 1.);
  auto lccStation3 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation3", "3", nullptr, 1.);
  auto lccStation4 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation4", "4", nullptr, 1.);
  auto vscStation1 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation1", "1", nullptr, false, 1.1, 1., emptyPoints);
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "2", nullptr, true, 2.1, 2., emptyPoints);
  auto vscStation21 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation21", "2", nullptr, true, 2.1, 2., emptyPoints);
  auto vscStation22 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation22", "2", nullptr, true, 2.1, 2., emptyPoints);
  auto vscStation23 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation23", "2", nullptr, true, 2.1, 2., emptyPoints);
  auto vscStation5 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation5", "5", nullptr, true, 5.1, 5., emptyPoints);
  auto vscStation6 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation6", "6", nullptr, true, 6.1, 6., emptyPoints);
  auto vscStation7 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation7", "7", nullptr, true, 7.1, 7., emptyPoints);
  auto vscStation8 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation8", "8", nullptr, true, 8.1, 8., emptyPoints);
  auto vscStation9 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation9", "9", nullptr, true, 9.1, 9., emptyPoints);
  auto vscStation10 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation10", "10", nullptr, true, 10.1, 10., emptyPoints);
  auto vscStation11 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation11", "11", nullptr, true, 11.1, 11., emptyPoints);
  auto vscStation12 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation12", "12", nullptr, true, 12.1, 12., emptyPoints);
  auto hvdcLineLCC = dfl::inputs::HvdcLine::build("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation1, dummyStation, boost::none,
                                                  0);  // first is in main cc
  auto hvdcLineVSC = dfl::inputs::HvdcLine::build("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation1, dummyStationVSC, boost::none,
                                                  1);  // first in main cc
  auto hvdcLineVSC2 = dfl::inputs::HvdcLine::build("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, dummyStationVSC, vscStation2, boost::none,
                                                   2);  // second in main cc
  auto hvdcLineVSC3 = dfl::inputs::HvdcLine::build("HVDCVSCLine3", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation21, dummyStationVSC, boost::none,
                                                   2);  // first in main cc
  auto hvdcLineBothInMainComponent = dfl::inputs::HvdcLine::build("HVDCLineBothInMain1", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation3, lccStation4,
                                                                  boost::none, 3.4);  // both in main cc
  auto hvdcLineBothInMainComponent2 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain2", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation5, vscStation6,
                                                                   activeControl, 5.6);  // both in main cc
  auto hvdcLineBothInMainComponent3 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain3", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation22, vscStation7,
                                                                   activeControl, 2.7);  // both in main cc
  auto hvdcLineBothInMainComponent4 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain4", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation9, vscStation10,
                                                                   boost::none, 9.10);  // both in main cc
  auto hvdcLineBothInMainComponent5 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain5", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation23, vscStation8,
                                                                   boost::none, 2.8);  // both in main cc
  auto hvdcLineVSCSwitch1 =
      dfl::inputs::HvdcLine::build("HVDCVSCLineSwitch1", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation11, dummyStationVSC, boost::none,
                                   11);  // first in main cc
  auto hvdcLineVSCSwitch2 =
      dfl::inputs::HvdcLine::build("HVDCVSCLineSwitch2", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation12, dummyStationVSC, boost::none,
                                   12);  // first in main cc
  nodes[0]->converters.push_back(lccStation1);
  nodes[1]->converters.push_back(vscStation1);
  nodes[2]->converters.push_back(vscStation2);
  nodes[2]->converters.push_back(vscStation21);
  nodes[2]->converters.push_back(vscStation22);
  nodes[2]->converters.push_back(vscStation23);
  nodes[3]->converters.push_back(lccStation3);
  nodes[4]->converters.push_back(lccStation4);
  nodes[5]->converters.push_back(vscStation5);
  nodes[6]->converters.push_back(vscStation6);
  nodes[7]->converters.push_back(vscStation7);
  nodes[8]->converters.push_back(vscStation8);
  nodes[9]->converters.push_back(vscStation9);
  nodes[10]->converters.push_back(vscStation10);
  nodes[11]->converters.push_back(vscStation11);
  nodes[12]->converters.push_back(vscStation12);

  testServiceManager->add("11", vl->id, "12");

  dfl::inputs::NetworkManager::BusMapRegulating busMap{std::make_pair("2", dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES)};

  dfl::algo::HVDCLineDefinitions hvdcDefs;
  bool useReactiveLimits = true;
  std::unordered_set<std::shared_ptr<dfl::inputs::Converter>> set{
      vscStation1, vscStation2, vscStation21, vscStation22, vscStation23, vscStation5,  vscStation6,
      vscStation7, vscStation8, vscStation9,  vscStation10, vscStation11, vscStation12,
  };
  dfl::algo::HVDCDefinitionAlgorithm algo(hvdcDefs, useReactiveLimits, set, busMap, testServiceManager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo(node, algoRes);
  }

  auto& hvdcLines = hvdcDefs.hvdcLines;
  ASSERT_EQ(hvdcLines.size(), 11);
  ASSERT_EQ(hvdcLines.at("HVDCLCCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhi);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVEmulation);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulation);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain4").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPV);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain5").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQProp);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);

  checkVSCIds(hvdcDefs);

  hvdcLines.clear();
  hvdcDefs.vscBusVSCDefinitionsMap.clear();
  // case diagrams
  useReactiveLimits = false;
  dfl::algo::HVDCDefinitionAlgorithm algo2(hvdcDefs, useReactiveLimits, set, busMap, testServiceManager);
  for (const auto& node : nodes) {
    algo2(node, algoRes);
  }

  ASSERT_EQ(hvdcLines.size(), 11);
  ASSERT_EQ(hvdcLines.at("HVDCLCCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulation);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulation);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain4").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain5").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);

  checkVSCIds(hvdcDefs);
}

static void
testDiagramValidity(std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points, bool isDiagramValid) {
  using dfl::inputs::Generator;
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  Generator generator("G1", true, points, 3., 30., 33., 330., 100, bus1, bus2);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::shared_ptr<dfl::inputs::Node> node = dfl::inputs::Node::build("0", vl, 0.0, {});

  const dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::algo::GeneratorDefinitionAlgorithm::BusGenMap busesWithDynamicModel;
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busesWithDynamicModel, busMap, false, testServiceManager);

  node->generators.emplace_back(generator);
  algo_infinite(node, algoRes);
  ASSERT_EQ(generators.size(), 1);
  if (!isDiagramValid) {
    ASSERT_TRUE(generators.at(0).isNetwork());
    ASSERT_EQ(generators.at(0).model, dfl::algo::GeneratorDefinition::ModelType::NETWORK);
  }
}

TEST(Generators, validDiagram) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({Generator::ReactiveCurvePoint(12., 44., 440.), Generator::ReactiveCurvePoint(65., 44., 440.)});
  bool isDiagramValid = true;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Generators, allPEqual) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({Generator::ReactiveCurvePoint(65., 44., 450.), Generator::ReactiveCurvePoint(65., 24., 420.),
                                                     Generator::ReactiveCurvePoint(65., 44., 250.), Generator::ReactiveCurvePoint(65., 45., 440.)});
  bool isDiagramValid = false;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Generators, allQminEqualQmax) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({Generator::ReactiveCurvePoint(1., 44., 44.), Generator::ReactiveCurvePoint(2., 420., 420.),
                                                     Generator::ReactiveCurvePoint(3., 250., 250.), Generator::ReactiveCurvePoint(4., 440., 440.)});
  bool isDiagramValid = false;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Generators, validDiagram2) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({Generator::ReactiveCurvePoint(10., 44., 440.), Generator::ReactiveCurvePoint(12., 44., 440.),
                                                     Generator::ReactiveCurvePoint(12., 44., 440.), Generator::ReactiveCurvePoint(65., 44., 440.)});
  bool isDiagramValid = true;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Generators, validDiagram3) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({Generator::ReactiveCurvePoint(10., 44., 44.), Generator::ReactiveCurvePoint(11., 44., 44.),
                                                     Generator::ReactiveCurvePoint(12., 44., 44.), Generator::ReactiveCurvePoint(65., 1., 87.)});
  bool isDiagramValid = true;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Generators, emptyDiagram) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({});
  bool isDiagramValid = true;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Generators, oneReactiveCurvePoint) {
  using dfl::inputs::Generator;
  std::vector<Generator::ReactiveCurvePoint> points({Generator::ReactiveCurvePoint(12., 44., 440.)});
  bool isDiagramValid = false;
  testDiagramValidity(points, isDiagramValid);
}

TEST(Counter, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto vl3 = std::make_shared<dfl::inputs::VoltageLevel>("VL3");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  std::vector<dfl::inputs::Shunt> shunts3 = {dfl::inputs::Shunt("3.1"), dfl::inputs::Shunt("3.2"), dfl::inputs::Shunt("3.3")};
  std::vector<dfl::inputs::Shunt> shunts4 = {dfl::inputs::Shunt("4.1"), dfl::inputs::Shunt("4.2"), dfl::inputs::Shunt("4.3"), dfl::inputs::Shunt("4.4")};
  std::vector<dfl::inputs::Shunt> shunts5 = {dfl::inputs::Shunt("5.1"), dfl::inputs::Shunt("5.2"), dfl::inputs::Shunt("5.3"), dfl::inputs::Shunt("5.4"),
                                             dfl::inputs::Shunt("5.5")};
  std::vector<dfl::inputs::Shunt> shunts6 = {dfl::inputs::Shunt("6.1"), dfl::inputs::Shunt("6.2"), dfl::inputs::Shunt("6.3"),
                                             dfl::inputs::Shunt("6.4"), dfl::inputs::Shunt("6.5"), dfl::inputs::Shunt("6.6")};
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl3, 0.0, {}),      dfl::inputs::Node::build("1", vl, 1.0, shunts1),  dfl::inputs::Node::build("2", vl, 2.0, shunts2),
      dfl::inputs::Node::build("3", vl, 3.0, shunts3),  dfl::inputs::Node::build("4", vl2, 5.0, shunts4), dfl::inputs::Node::build("5", vl2, 5.0, shunts5),
      dfl::inputs::Node::build("6", vl2, 0.0, shunts6),
  };

  dfl::algo::ShuntCounterDefinitions defs;
  dfl::algo::ShuntCounterAlgorithm algo(defs);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(defs.nbShunts.size(), 3);
  ASSERT_EQ(defs.nbShunts.at("VL"), 6);
  ASSERT_EQ(defs.nbShunts.at("VL2"), 15);
  ASSERT_EQ(defs.nbShunts.at("VL3"), 0);
}

TEST(LinesByIds, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VLb");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  std::vector<dfl::inputs::Shunt> shunts3 = {dfl::inputs::Shunt("3.1"), dfl::inputs::Shunt("3.2"), dfl::inputs::Shunt("3.3")};
  std::vector<dfl::inputs::Shunt> shunts4 = {dfl::inputs::Shunt("4.1"), dfl::inputs::Shunt("4.2"), dfl::inputs::Shunt("4.3"), dfl::inputs::Shunt("4.4")};
  std::vector<dfl::inputs::Shunt> shunts5 = {dfl::inputs::Shunt("5.1"), dfl::inputs::Shunt("5.2"), dfl::inputs::Shunt("5.3"), dfl::inputs::Shunt("5.4"),
                                             dfl::inputs::Shunt("5.5")};
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("VL0", vl2, 0.0, {}),     dfl::inputs::Node::build("VL1", vl, 1.0, shunts1), dfl::inputs::Node::build("VL2", vl, 2.0, shunts2),
      dfl::inputs::Node::build("VL3", vl, 3.0, shunts3), dfl::inputs::Node::build("VL4", vl, 4.0, shunts4), dfl::inputs::Node::build("VL5", vl, 5.0, shunts5),
      dfl::inputs::Node::build("VL6", vl, 0.0, {}),
  };
  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{
      dfl::inputs::Line::build("0", nodes[0], nodes[1], "ETE", true, true),   dfl::inputs::Line::build("1", nodes[0], nodes[2], "UNDEFINED", true, true),
      dfl::inputs::Line::build("2", nodes[0], nodes[3], "HIVER", true, true), dfl::inputs::Line::build("3", nodes[3], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("4", nodes[2], nodes[4], "HIVER", true, true), dfl::inputs::Line::build("5", nodes[1], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("6", nodes[5], nodes[6], "HIVER", true, true),
  };

  dfl::algo::LinesByIdDefinitions def;
  dfl::algo::LinesByIdAlgorithm algo(def);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(def.linesMap.size(), 7);
  ASSERT_EQ(def.linesMap.at("0").id, "0");
  ASSERT_EQ(def.linesMap.at("0").activeSeason, "ETE");
  ASSERT_EQ(def.linesMap.at("2").id, "2");
  ASSERT_EQ(def.linesMap.at("2").activeSeason, "HIVER");
  ASSERT_EQ(def.linesMap.at("3").id, "3");
  ASSERT_EQ(def.linesMap.at("3").activeSeason, "UNDEFINED");
}

TEST(SVARC, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 1.0, {}),  dfl::inputs::Node::build("1", vl, 1.0, {}),  dfl::inputs::Node::build("2", vl, 2.0, {}),
      dfl::inputs::Node::build("3", vl, 3.0, {}),  dfl::inputs::Node::build("4", vl2, 5.0, {}), dfl::inputs::Node::build("5", vl2, 5.0, {}),
      dfl::inputs::Node::build("6", vl2, 0.0, {}),
  };

  nodes[0]->svarcs.emplace_back("SVARC0", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., false, false, "0", "0", 1.);
  nodes[0]->svarcs.emplace_back("SVARC1", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., true, false, "0", "0", 1.);
  nodes[0]->svarcs.emplace_back("SVARC2", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., false, true, "0", "0", 1.);
  nodes[0]->svarcs.emplace_back("SVARC3", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., true, true, "0", "0", 1.);
  nodes[0]->svarcs.emplace_back("SVARC4", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., false, true, "0", "1", 1.);
  nodes[0]->svarcs.emplace_back("SVARC5", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., true, true, "0", "1", 1.);
  nodes[0]->svarcs.emplace_back("SVARC6", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., false, false, "0", "1", 1.);
  nodes[0]->svarcs.emplace_back("SVARC7", true, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., true, false, "0", "1", 1.);
  nodes[0]->svarcs.emplace_back("SVARC8", false, 0., 10., 100, 230, 215, 230, 235, 245, 10., 10., true, false, "0", "1", 1.);

  using modelType = dfl::algo::StaticVarCompensatorDefinition::ModelType;
  dfl::algo::StaticVarCompensatorAlgorithm::SVarCDefinitions expected_svarcs = {
      dfl::algo::StaticVarCompensatorDefinition("SVARC0", modelType::SVARCPV, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC1", modelType::SVARCPVMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC2", modelType::SVARCPVPROP, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC3", modelType::SVARCPVPROPMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC4", modelType::SVARCPVPROPREMOTE, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC5", modelType::SVARCPVPROPREMOTEMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC6", modelType::SVARCPVREMOTE, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC7", modelType::SVARCPVREMOTEMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      dfl::algo::StaticVarCompensatorDefinition("SVARC8", modelType::NETWORK, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.)};

  dfl::algo::StaticVarCompensatorAlgorithm::SVarCDefinitions svarcs;
  dfl::algo::StaticVarCompensatorAlgorithm algo(svarcs);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algo(node, algoRes);
  }
  ASSERT_EQ(9, svarcs.size());
  for (size_t index = 0; index < svarcs.size(); ++index) {
    ASSERT_EQ(expected_svarcs[index].id, svarcs[index].id);
    ASSERT_EQ(expected_svarcs[index].model, svarcs[index].model);
  }
}

static void
addContingency(std::vector<dfl::inputs::Contingency>& contingencies, const std::string& id, const std::string elementId,
               dfl::inputs::ContingencyElement::Type elementType) {
  contingencies.emplace_back(id);
  contingencies[contingencies.size() - 1].elements.emplace_back(elementId, elementType);
}

TEST(ContingencyValidation, base) {
  using Type = dfl::inputs::ContingencyElement::Type;

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 110.0, {dfl::inputs::Shunt("SHUNT")}),
      dfl::inputs::Node::build("1", vl, 110.0, {}),
      dfl::inputs::Node::build("2", vl, 110.0, {}),
      dfl::inputs::Node::build("3", vl, 110.0, {}),
      dfl::inputs::Node::build("4", vl2, 220.0, {}),
      dfl::inputs::Node::build("5", vl2, 220.0, {}),
      dfl::inputs::Node::build("6", vl2, 220.0, {}),
      dfl::inputs::Node::build("7", vl2, 220.0, {}),
      dfl::inputs::Node::build("8", vl2, 220.0, {}),
  };

  dfl::algo::LoadDefinitionAlgorithm::Loads loads = {dfl::algo::LoadDefinition("LOAD", dfl::algo::LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "3"),
                                                     dfl::algo::LoadDefinition("LOADNETWORK", dfl::algo::LoadDefinition::ModelType::NETWORK, "3")};

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators = {
      dfl::algo::GeneratorDefinition("GENERATOR", dfl::algo::GeneratorDefinition::ModelType::SIGNALN, "4", points, 0, 0, 0, 0, 0, "4"),
      dfl::algo::GeneratorDefinition("GENERATORNETWORK", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "4", points, 0, 0, 0, 0, 0, "4")};

  dfl::algo::StaticVarCompensatorAlgorithm::SVarCDefinitions svarcs = {
      dfl::algo::StaticVarCompensatorDefinition("SVARC", dfl::algo::StaticVarCompensatorDefinition::ModelType::SVARCPV, 0., 10., 230, 230, 215, 230, 235, 245,
                                                0., 10, 230.),
      dfl::algo::StaticVarCompensatorDefinition("SVARCNETWORK", dfl::algo::StaticVarCompensatorDefinition::ModelType::NETWORK, 0., 10., 230, 230, 215, 230, 235,
                                                245, 0., 10, 230.)};

  nodes[7]->danglingLines.emplace_back("DANGLINGLINE");
  nodes[8]->busBarSections.emplace_back("BUSBAR");

  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{dfl::inputs::Line::build("LINE", nodes[0], nodes[1], "ETE", true, true)};
  std::vector<std::shared_ptr<dfl::inputs::Tfo>> tfos{dfl::inputs::Tfo::build("TFO", nodes[0], nodes[1], true, true),
                                                      dfl::inputs::Tfo::build("TFO3", nodes[0], nodes[1], nodes[2], true, true, true)};

  auto conv1 = std::make_shared<dfl::inputs::LCCConverter>("LccStation1", "0", nullptr, 1.);
  auto conv2 = std::make_shared<dfl::inputs::LCCConverter>("LccStation2", "1", nullptr, 1.);
  nodes[0]->converters.push_back(conv1);
  nodes[1]->converters.push_back(conv1);
  auto hvdc_line = dfl::inputs::HvdcLine::build("HVDC_LINE", dfl::inputs::HvdcLine::ConverterType::LCC, conv1, conv2, boost::none, 3.4);
  conv1->hvdcLine = hvdc_line;
  conv2->hvdcLine = hvdc_line;

  auto contingencies = std::vector<dfl::inputs::Contingency>();

  addContingency(contingencies, "load", "LOAD", Type::LOAD);
  addContingency(contingencies, "loadnetwork", "LOADNETWORK", Type::LOAD);
  addContingency(contingencies, "load_bad_id", "XXX", Type::LOAD);
  addContingency(contingencies, "load_bad_type", "LOAD", Type::GENERATOR);
  addContingency(contingencies, "load_multiple_elements_one_bad", "LOAD", Type::LOAD);
  contingencies[contingencies.size() - 1].elements.emplace_back("XXX", Type::LINE);

  addContingency(contingencies, "generator", "GENERATOR", Type::GENERATOR);
  addContingency(contingencies, "generatornetwork", "GENERATORNETWORK", Type::GENERATOR);
  addContingency(contingencies, "generator_bad_id", "XXX", Type::GENERATOR);

  addContingency(contingencies, "shunt_compensator", "SHUNT", Type::SHUNT_COMPENSATOR);
  addContingency(contingencies, "shunt_compensator_bad_id", "XXX", Type::SHUNT_COMPENSATOR);

  addContingency(contingencies, "static_var_compensator", "SVARC", Type::STATIC_VAR_COMPENSATOR);
  addContingency(contingencies, "static_var_compensator_network", "SVARCNETWORK", Type::STATIC_VAR_COMPENSATOR);
  addContingency(contingencies, "static_var_compensator_bad_id", "XXX", Type::STATIC_VAR_COMPENSATOR);

  addContingency(contingencies, "dangling_line", "DANGLINGLINE", Type::DANGLING_LINE);
  addContingency(contingencies, "dangling_line_bad_id", "XXX", Type::DANGLING_LINE);

  addContingency(contingencies, "busbarsection", "BUSBAR", Type::BUSBAR_SECTION);
  addContingency(contingencies, "busbarsection_bad_id", "XXX", Type::BUSBAR_SECTION);

  addContingency(contingencies, "line", "LINE", Type::LINE);
  addContingency(contingencies, "line_bad_id", "XXX", Type::LINE);
  addContingency(contingencies, "line_branch", "LINE", Type::BRANCH);
  addContingency(contingencies, "line_branch_bad_id", "XXX", Type::BRANCH);

  addContingency(contingencies, "2wt", "TFO", Type::TWO_WINDINGS_TRANSFORMER);
  addContingency(contingencies, "2wt_bad_id", "XXX", Type::TWO_WINDINGS_TRANSFORMER);
  addContingency(contingencies, "2wt_branch", "TFO", Type::BRANCH);
  addContingency(contingencies, "2wt_branch_bad_id", "XXX", Type::BRANCH);

  addContingency(contingencies, "3wt", "TFO3", Type::THREE_WINDINGS_TRANSFORMER);
  addContingency(contingencies, "3wt_bad_id", "XXX", Type::THREE_WINDINGS_TRANSFORMER);

  addContingency(contingencies, "hvdcline", "HVDC_LINE", Type::HVDC_LINE);
  addContingency(contingencies, "hvdcline_bad_id", "XXX", Type::HVDC_LINE);

  auto validContingencies = dfl::algo::ValidContingencies(contingencies);
  auto algoOnInputs = dfl::algo::ContingencyValidationAlgorithmOnNodes(validContingencies);

  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algoOnInputs(node, algoRes);
  }
  auto algoOnDefs = dfl::algo::ContingencyValidationAlgorithmOnDefs(validContingencies);
  algoOnDefs.fillValidContingenciesOnDefs(loads, generators, svarcs);

  validContingencies.keepContingenciesWithAllElementsValid();

  auto expectedValidContingencies = std::set<std::string>();
  expectedValidContingencies.emplace("load");
  expectedValidContingencies.emplace("loadnetwork");
  expectedValidContingencies.emplace("generator");
  expectedValidContingencies.emplace("generatornetwork");
  expectedValidContingencies.emplace("shunt_compensator");
  expectedValidContingencies.emplace("static_var_compensator");
  expectedValidContingencies.emplace("static_var_compensator_network");
  expectedValidContingencies.emplace("dangling_line");
  expectedValidContingencies.emplace("busbarsection");
  expectedValidContingencies.emplace("line");
  expectedValidContingencies.emplace("line_branch");
  expectedValidContingencies.emplace("2wt");
  expectedValidContingencies.emplace("2wt_branch");
  expectedValidContingencies.emplace("3wt");
  expectedValidContingencies.emplace("hvdcline");

  for (auto validContingency : validContingencies.get()) {
    auto expected = expectedValidContingencies.find(validContingency.get().id);
    ASSERT_TRUE(expected != expectedValidContingencies.end());
    expectedValidContingencies.erase(expected);
  }

  ASSERT_TRUE(expectedValidContingencies.empty());
  auto elementsNetworkType = validContingencies.getNetworkElements();
  ASSERT_EQ(elementsNetworkType.size(), 3);
  ASSERT_FALSE(elementsNetworkType.find("LOAD") != elementsNetworkType.end());
  ASSERT_TRUE(elementsNetworkType.find("LOADNETWORK") != elementsNetworkType.end());
  ASSERT_TRUE(elementsNetworkType.find("GENERATORNETWORK") != elementsNetworkType.end());
  ASSERT_TRUE(elementsNetworkType.find("SVARCNETWORK") != elementsNetworkType.end());
}
