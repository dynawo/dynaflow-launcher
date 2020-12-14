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
#include <DYNBusInterface.h>
#include "Configuration.h"
#include "Dico.h"
#include "Tests.h"

#include <algorithm>
#include <boost/make_shared.hpp>
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
    return boost::shared_ptr<DYN::BusInterface> ();
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
      dfl::inputs::Node::build("0", vl, 0.0), dfl::inputs::Node::build("1", vl, 1.0), dfl::inputs::Node::build("2", vl, 2.0),
      dfl::inputs::Node::build("3", vl, 3.0), dfl::inputs::Node::build("4", vl, 4.0), dfl::inputs::Node::build("5", vl, 5.0),
      dfl::inputs::Node::build("6", vl, 0.0),
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
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0), dfl::inputs::Node::build("1", vl, 1.0), dfl::inputs::Node::build("2", vl, 2.0),
      dfl::inputs::Node::build("3", vl, 3.0), dfl::inputs::Node::build("4", vl, 5.0), dfl::inputs::Node::build("5", vl, 5.0),
      dfl::inputs::Node::build("6", vl, 0.0),
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
      dfl::inputs::Node::build("0", vl, 0.0), dfl::inputs::Node::build("1", vl, 1.0), dfl::inputs::Node::build("2", vl, 2.0),
      dfl::inputs::Node::build("3", vl, 3.0), dfl::inputs::Node::build("4", vl, 5.0), dfl::inputs::Node::build("5", vl, 5.0),
      dfl::inputs::Node::build("6", vl, 0.0),
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
      dfl::inputs::Node::build("0", vl, 0.0), dfl::inputs::Node::build("1", vl, 1.0), dfl::inputs::Node::build("2", vl, 2.0),
      dfl::inputs::Node::build("3", vl, 3.0), dfl::inputs::Node::build("4", vl, 5.0), dfl::inputs::Node::build("5", vl, 5.0),
      dfl::inputs::Node::build("6", vl, 0.0),
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
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{dfl::inputs::Node::build("0", vl, 0.0), dfl::inputs::Node::build("1", vl, 1.0),
                                                        dfl::inputs::Node::build("2", vl, 2.0), dfl::inputs::Node::build("3", vl, 3.0),
                                                        dfl::inputs::Node::build("4", vl, 5.0), dfl::inputs::Node::build("5", vl, 5.0)};
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

TEST(Generators, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0),  dfl::inputs::Node::build("1", vl, 1.0),  dfl::inputs::Node::build("2", vl, 2.0),
      dfl::inputs::Node::build("3", vl, 3.0),  dfl::inputs::Node::build("4", vl2, 5.0), dfl::inputs::Node::build("5", vl2, 5.0),
      dfl::inputs::Node::build("6", vl2, 0.0),
  };

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "0", points0, 0, 0, 0,
                                     0),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "0", points, -1, 1, -1,
                                     1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN, "2", points, -2, 2, -2, 2),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN, "4", points, -5, 5, -5, 5),
  };

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "0", points0, 0, 0, 0,
                                     0),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "0", points, -1, 1, -1,
                                     1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "2", points, -2, 2, -2, 2),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "4", points, -5, 5, -5, 5),
  };

  nodes[0]->generators.emplace_back("00", points0, 0, 0, 0, 0, 0);
  nodes[0]->generators.emplace_back("01", points, -1, 1, -1, 1, 0);

  nodes[2]->generators.emplace_back("02", points, -2, 2, -2, 2, 0);

  nodes[4]->generators.emplace_back("05", points, -5, 5, -5, 5, 0);

  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, true, testServiceManager);

  std::for_each(nodes.begin(), nodes.end(), algo_infinite);

  ASSERT_EQ(4, generators.size());
  for (size_t index = 0; index < generators.size(); ++index) {
    ASSERT_EQ(expected_gens_infinite[index].id, generators[index].id);
    ASSERT_EQ(expected_gens_infinite[index].model, generators[index].model);
    ASSERT_EQ(expected_gens_infinite[index].points.size(), generators[index].points.size());
    ASSERT_EQ(expected_gens_infinite[index].qmin, generators[index].qmin);
    ASSERT_EQ(expected_gens_infinite[index].qmax, generators[index].qmax);
    ASSERT_EQ(expected_gens_infinite[index].pmin, generators[index].pmin);
    ASSERT_EQ(expected_gens_infinite[index].pmax, generators[index].pmax);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
    for (size_t index_p = 0; index_p < expected_gens_infinite[index].points.size(); ++index_p) {
      ASSERT_EQ(expected_gens_infinite[index].points[index_p].p, generators[index].points[index_p].p);
      ASSERT_EQ(expected_gens_infinite[index].points[index_p].qmax, generators[index].points[index_p].qmax);
      ASSERT_EQ(expected_gens_infinite[index].points[index_p].qmin, generators[index].points[index_p].qmin);
    }
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, false, testServiceManager);

  std::for_each(nodes.begin(), nodes.end(), algo_finite);

  ASSERT_EQ(4, generators.size());
  for (size_t index = 0; index < generators.size(); ++index) {
    ASSERT_EQ(expected_gens_finite[index].id, generators[index].id);
    ASSERT_EQ(expected_gens_finite[index].model, generators[index].model);
    ASSERT_EQ(expected_gens_finite[index].qmin, generators[index].qmin);
    ASSERT_EQ(expected_gens_finite[index].qmax, generators[index].qmax);
    ASSERT_EQ(expected_gens_finite[index].pmin, generators[index].pmin);
    ASSERT_EQ(expected_gens_finite[index].pmax, generators[index].pmax);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
    ASSERT_EQ(expected_gens_finite[index].points.size(), generators[index].points.size());
    for (size_t index_p = 0; index_p < expected_gens_finite[index].points.size(); ++index_p) {
      ASSERT_EQ(expected_gens_finite[index].points[index_p].p, generators[index].points[index_p].p);
      ASSERT_EQ(expected_gens_finite[index].points[index_p].qmax, generators[index].points[index_p].qmax);
      ASSERT_EQ(expected_gens_finite[index].points[index_p].qmin, generators[index].points[index_p].qmin);
    }
  }
}

TEST(Generators, SwitchConnexity) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0),  dfl::inputs::Node::build("1", vl, 1.0),  dfl::inputs::Node::build("2", vl, 2.0),
      dfl::inputs::Node::build("3", vl, 3.0),  dfl::inputs::Node::build("4", vl2, 5.0), dfl::inputs::Node::build("5", vl2, 5.0),
      dfl::inputs::Node::build("6", vl2, 0.0),
  };

  testServiceManager->add("0", "VL", "1");
  testServiceManager->add("1", "VL", "2");
  testServiceManager->add("4", "VL2", "5");

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "0", points0, -1, 1, -1,
                                     1, 1),  // due to switch connexity
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "2", points, -2, 2, -2,
                                     2, 2),  // due to switch connexity
      dfl::algo::GeneratorDefinition("04", dfl::algo::GeneratorDefinition::ModelType::SIGNALN, "4", points, -5, 5, -5, 5, 5),
  };

  nodes[0]->generators.emplace_back("00", points0, -1, 1, -1, 1, 1);

  nodes[2]->generators.emplace_back("02", points, -2, 2, -2, 2 ,2);

  nodes[4]->generators.emplace_back("04", points, -5, 5, -5, 5 ,5);

  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, true, testServiceManager);

  std::for_each(nodes.begin(), nodes.end(), algo_infinite);

  ASSERT_EQ(3, generators.size());
  for (size_t index = 0; index < generators.size(); ++index) {
    ASSERT_EQ(expected_gens_infinite[index].id, generators[index].id);
    ASSERT_EQ(expected_gens_infinite[index].model, generators[index].model);
    ASSERT_EQ(expected_gens_infinite[index].points.size(), generators[index].points.size());
    ASSERT_EQ(expected_gens_infinite[index].qmin, generators[index].qmin);
    ASSERT_EQ(expected_gens_infinite[index].qmax, generators[index].qmax);
    ASSERT_EQ(expected_gens_infinite[index].pmin, generators[index].pmin);
    ASSERT_EQ(expected_gens_infinite[index].pmax, generators[index].pmax);
    for (size_t index_p = 0; index_p < expected_gens_infinite[index].points.size(); ++index_p) {
      ASSERT_EQ(expected_gens_infinite[index].points[index_p].p, generators[index].points[index_p].p);
      ASSERT_EQ(expected_gens_infinite[index].points[index_p].qmax, generators[index].points[index_p].qmax);
      ASSERT_EQ(expected_gens_infinite[index].points[index_p].qmin, generators[index].points[index_p].qmin);
    }
  }
}

TEST(Loads, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0), dfl::inputs::Node::build("1", vl, 111.0), dfl::inputs::Node::build("2", vl, 24.0),
      dfl::inputs::Node::build("3", vl, 63.0), dfl::inputs::Node::build("4", vl, 56.0),  dfl::inputs::Node::build("5", vl, 46.0),
      dfl::inputs::Node::build("6", vl, 0.0),
  };

  dfl::algo::LoadDefinitionAlgorithm::Loads expected_loads = {
      dfl::algo::LoadDefinition("00", "0"),
      dfl::algo::LoadDefinition("01", "0"),
      dfl::algo::LoadDefinition("05", "4"),
  };

  nodes[0]->loads.emplace_back("00");
  nodes[0]->loads.emplace_back("01");

  nodes[2]->loads.emplace_back("02");

  nodes[4]->loads.emplace_back("05");

  dfl::algo::LoadDefinitionAlgorithm::Loads loads;
  double dsoVoltageLevel = 45.0;
  dfl::algo::LoadDefinitionAlgorithm algo(loads, dsoVoltageLevel);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(3, loads.size());
  for (size_t index = 0; index < loads.size(); ++index) {
    ASSERT_EQ(expected_loads[index].id, loads[index].id);
  }
}

static bool
hvdcLineDefinitionEqual(const dfl::algo::HvdcLineDefinition& lhs, const dfl::algo::HvdcLineDefinition& rhs) {
  return lhs.id == rhs.id && lhs.converterType == rhs.converterType && lhs.converter1_id == rhs.converter1_id && lhs.converter1_busId == rhs.converter1_busId &&
         lhs.converter2_voltageRegulationOn == rhs.converter2_voltageRegulationOn && lhs.converter2_id == rhs.converter2_id &&
         lhs.converter2_busId == rhs.converter2_busId && lhs.converter2_voltageRegulationOn == rhs.converter2_voltageRegulationOn &&
         lhs.position == rhs.position;
}

TEST(HvdcLine, base) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0), dfl::inputs::Node::build("1", vl, 111.0), dfl::inputs::Node::build("2", vl, 24.0),
      dfl::inputs::Node::build("3", vl, 63.0), dfl::inputs::Node::build("4", vl, 56.0),  dfl::inputs::Node::build("5", vl, 46.0),
      dfl::inputs::Node::build("6", vl, 0.0),
  };
  auto lccStation1 = dfl::inputs::ConverterInterface("LCCStation1", "_BUS___11_TN");
  auto vscStation2 = dfl::inputs::ConverterInterface("VSCStation2", "_BUS___11_TN", false);
  auto lccStationMain1 = dfl::inputs::ConverterInterface("LCCStationMain1", "_BUS__11_TN");

  auto lccStationMain2 = dfl::inputs::ConverterInterface("LCCStationMain2", "_BUS__11_TN");

  auto hvdcLineLCC = std::make_shared<dfl::inputs::HvdcLine>("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN",
                                                             boost::optional<bool>(), "LCCStation2", "_BUS___10_TN", boost::optional<bool>());
  auto hvdcLineVSC = std::make_shared<dfl::inputs::HvdcLine>("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true,
                                                             "VSCStation2", "_BUS___11_TN", false);
  auto hvdcLineBothInMainComponent =
      std::make_shared<dfl::inputs::HvdcLine>("HVDCLineBothInMain", dfl::inputs::HvdcLine::ConverterType::VSC, "LCCStationMain1", "_BUS__11_TN",
                                              boost::optional<bool>(), "LCCStationMain2", "_BUS__11_TN", boost::optional<bool>());

  lccStation1.hvdcLine = hvdcLineLCC;
  vscStation2.hvdcLine = hvdcLineVSC;
  lccStationMain2.hvdcLine = hvdcLineBothInMainComponent;
  lccStationMain1.hvdcLine = hvdcLineBothInMainComponent;

  dfl::algo::HvdcLineDefinition::HvdcLines expected_hvdcLines = {
      dfl::algo::HvdcLineDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", boost::optional<bool>(),
                                    "LCCStation2", "_BUS___10_TN", boost::optional<bool>(), dfl::algo::HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT),
      dfl::algo::HvdcLineDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2",
                                    "_BUS___11_TN", false, dfl::algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT)};

  nodes[0]->converterInterfaces.emplace_back(lccStation1);
  nodes[2]->converterInterfaces.emplace_back(lccStationMain1);
  nodes[0]->converterInterfaces.emplace_back(lccStationMain2);
  nodes[4]->converterInterfaces.emplace_back(vscStation2);

  dfl::algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap hvdcLines;
  dfl::algo::ControllerInterfaceDefinitionAlgorithm algo(hvdcLines);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(2, hvdcLines.size());
  size_t index = 0;
  for (const auto& expected_hvdcLine : expected_hvdcLines) {
    auto it = hvdcLines.find(expected_hvdcLine.id);
    ASSERT_NE(hvdcLines.end(), it);
    ASSERT_TRUE(hvdcLineDefinitionEqual(expected_hvdcLine, it->second));
  }
}

static void
testDiagramValidity(std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points, bool isDiagramValid) {
  using dfl::inputs::Generator;
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  Generator generator("G1", points, 3., 30., 33., 330., 100);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, false, testServiceManager);
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::shared_ptr<dfl::inputs::Node> node = dfl::inputs::Node::build("0", vl, 0.0);

  node->generators.emplace_back(generator);
  algo_infinite(node);
  if (isDiagramValid) {
    ASSERT_EQ(generators.size(), 1);
  } else {
    ASSERT_EQ(generators.size(), 0);
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
