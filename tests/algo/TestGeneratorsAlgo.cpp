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
 * @file  TestGeneratorsAlgo.cpp
 *
 * @brief GeneratorDefinitionAlgorithm library test file
 */

#include "GeneratorDefinitionAlgorithm.h"
#include "Tests.h"

#include <DYNMultiProcessingContext.h>

#include <boost/make_shared.hpp>

// Required for testing unit tests
testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::multiprocessing::Context mpiContext;

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
  void clear() { map_.clear(); }

  /**
   * @brief Add a switch connection
   *
   * this will perform the connection @p busId <=> @p otherbusid in voltage level
   *
   * @param busId the bus to connect to
   * @param VLId the voltage level id containing both buses
   * @param otherbusid the other bus id
   */
  void add(const std::string &busId, const std::string &VLId, const std::string &otherbusid) {
    map_[std::tie(busId, VLId)].push_back(otherbusid);
    map_[std::tie(otherbusid, VLId)].push_back(busId);
  }

  /**
   * @copydoc DYN::ServiceManagerInterface::getBusesConnectedBySwitch
   */
  std::vector<std::string> getBusesConnectedBySwitch(const std::string &busId, const std::string &VLId) const final {
    auto it = map_.find(std::tie(busId, VLId));
    if (it == map_.end()) {
      return {};
    }

    std::set<std::string> set;
    for (const auto &id : it->second) {
      updateSet(set, busId, VLId);
    }
    std::vector<std::string> ret(set.begin(), set.end());
    ret.erase(std::find(ret.begin(), ret.end(), busId));
    return ret;
  }

  /**
   * @copydoc DYN::ServiceManagerInterface::getBusesConnectedBySwitch
   */
  bool isBusConnected(const std::string &busId, const std::string &VLId) const final { return false; }

  /**
   * @copydoc DYN::ServiceManagerInterface::getRegulatedBus
   */
  boost::shared_ptr<DYN::BusInterface> getRegulatedBus(const std::string &regulatingComponent) const final { return boost::shared_ptr<DYN::BusInterface>(); }

 private:
  void updateSet(std::set<std::string> &set, const std::string &str, const std::string &vlid) const {
    if (set.count(str) > 0) {
      return;
    }

    set.insert(str);
    auto it = map_.find(std::tie(str, vlid));
    if (it == map_.end()) {
      return;
    }

    for (const auto &id : it->second) {
      updateSet(set, id, vlid);
    }
  }

 private:
  std::map<MapKey, std::vector<std::string>> map_;
};
}  // namespace test

static void generatorsEquals(const dfl::algo::GeneratorDefinition &lhs, const dfl::algo::GeneratorDefinition &rhs) {
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
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus3)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus3)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 0, bus1, bus3);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 0, bus2, bus2);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 0, bus3, bus2);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};

  dfl::inputs::DynamicDataBaseManager manager("", "");

  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 10.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 10.);

  for (const auto &node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}

TEST(Generators, baseSVC) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
  const std::string bus5 = "BUS_5";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 0, bus2, bus2);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 0, bus3, bus3);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 0, bus5, bus5);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus5, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "res/assembling_test_generator.xml");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 5.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 5.);

  for (const auto &node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}

TEST(Generators, baseSVCRpcl2) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
  const std::string bus5 = "BUS_5";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL2_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 0, bus2, bus2);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 0, bus3, bus3);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 0, bus5, bus5);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus5, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "res/assembling_test_generator_rpcl2.xml");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 5.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 5.);

  for (const auto &node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}

TEST(Generators, baseSVCTfo) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
  const std::string bus5 = "BUS_5";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 10, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 10, bus2, bus2);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 10, bus3, bus3);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 0, bus5, bus5);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus5, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "res/assembling_test_generator.xml");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 5.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 5.);

  for (const auto &node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}

TEST(Generators, baseSVCTfoRpcl2) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
  const std::string bus5 = "BUS_5";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL2_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus2),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus3),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus5)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 10, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 10, bus2, bus2);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 10, bus3, bus3);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 0, bus5, bus5);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus5, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "res/assembling_test_generator_rpcl2.xml");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 5.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 5.);

  for (const auto &node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}

TEST(Generators, generatorRemoteRegulationWithTfo) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus3)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus3)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 0, bus1, bus3);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 0, bus2, bus2);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 0, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 15, bus3, bus2);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 10.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 10.);

  for (const auto &node : nodes) {
    algo_finite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
    ASSERT_EQ(expected_gens_finite[index].targetP, generators[index].targetP);
  }
}
TEST(Generators, generatorWithTfo) {
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE, "2", points, -2, 2, -2, 2, 0, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 0, bus3)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_SIGNALN, "0", points0, 0, 0, 0, 0, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RECTANGULAR, "0", points, -1, 1, -1, 1, 0, 0,
                                     bus1),  // multiple generators on the same node
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 0, bus2),
      dfl::algo::GeneratorDefinition("03", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "3", points, 0, 0, 0, 0, 0, 0, bus4),
      dfl::algo::GeneratorDefinition("05", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 0, bus3)};

  nodes[0]->generators.emplace_back("00", true, points0, 0, 0, 0, 0, 0, 0, 10, bus1, bus1);
  nodes[0]->generators.emplace_back("01", true, points, -1, 1, -1, 1, 0, 0, 10, bus2, bus2);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 0, 0, 10, bus2, bus2);
  nodes[3]->generators.emplace_back("03", false, points, 0, 0, 0, 0, 0, 0, 10, bus4, bus4);
  nodes[4]->generators.emplace_back("05", true, points, -5, 5, -5, 5, 0, 0, 0, bus3, bus3);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::ONE},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 5.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(5, generators.size());
  ASSERT_TRUE(algoRes->isAtLeastOneGeneratorRegulating);
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
    ASSERT_EQ(expected_gens_infinite[index].targetP, generators[index].targetP);
  }

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 5.);

  for (const auto &node : nodes) {
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
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager)};

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});

  const std::string bus1 = "BUS_1";
  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_infinite = {
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "0", points, 0, 0, 0, 0, 0, 0, bus1)};

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("01", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "0", points, 0, 0, 0, 0, 0, 0, bus1)};

  nodes[0]->generators.emplace_back("01", false, points, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap;
  dfl::inputs::DynamicDataBaseManager manager("", "");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 10.);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(1, generators.size());
  ASSERT_FALSE(algoRes->isAtLeastOneGeneratorRegulating);

  generators.clear();
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 10.);

  for (const auto &node : nodes) {
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
      dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("1", vl, 1.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 2.0, {}, false, testServiceManager),  dfl::inputs::Node::build("3", vl, 3.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl2, 5.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl2, 5.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl2, 0.0, {}, false, testServiceManager),
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
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "0", points0, -1, 1, -1, 1, 0, 1,
                                     bus1),  // due to switch connexity
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "2", points, -2, 2, -2, 2, 0, 2,
                                     bus2),  // due to switch connexity
      dfl::algo::GeneratorDefinition("04", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, -5, 5, -5, 5, 0, 5, bus3),
  };

  dfl::algo::GeneratorDefinitionAlgorithm::Generators expected_gens_finite = {
      dfl::algo::GeneratorDefinition("00", dfl::algo::GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "0", points0, -1, 1, -1, 1, 0, 1,
                                     bus1),  // due to switch connexity
      dfl::algo::GeneratorDefinition("02", dfl::algo::GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "2", points, -2, 2, -2, 2, 0, 2,
                                     bus2),  // due to switch connexity
      dfl::algo::GeneratorDefinition("04", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RECTANGULAR, "4", points, -5, 5, -5, 5, 0, 5, bus3),
  };

  nodes[0]->generators.emplace_back("00", true, points0, -1, 1, -1, 1, 1, 0, 0, bus1, bus1);

  nodes[2]->generators.emplace_back("02", true, points, -2, 2, -2, 2, 2, 0, 0, bus2, bus2);

  nodes[4]->generators.emplace_back("04", true, points, -5, 5, -5, 5, 5, 0, 0, bus3, bus3);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                          {bus2, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                          {bus3, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  dfl::inputs::DynamicDataBaseManager manager("", "");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, true, 10.);

  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo_infinite(node, algoRes);
  }

  ASSERT_EQ(3, generators.size());
  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_infinite[index], generators[index]);
  }

  generators.clear();
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoResFinite(new dfl::algo::AlgorithmsResults());
  dfl::algo::GeneratorDefinitionAlgorithm algo_finite(generators, busMap, manager, false, 10.);

  for (const auto &node : nodes) {
    algo_finite(node, algoResFinite);
  }

  ASSERT_EQ(3, generators.size());
  ASSERT_TRUE(algoResFinite->isAtLeastOneGeneratorRegulating);

  for (size_t index = 0; index < generators.size(); ++index) {
    generatorsEquals(expected_gens_finite[index], generators[index]);
  }
}

static void testDiagramValidity(std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points, bool isDiagramValid) {
  using dfl::inputs::Generator;
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  Generator generator("G1", true, points, 3., 30., 33., 330., 100, 0, 0., bus1, bus2);
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators;
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::shared_ptr<dfl::inputs::Node> node = dfl::inputs::Node::build("0", vl, 0.0, {}, false, testServiceManager);

  const dfl::inputs::NetworkManager::BusMapRegulating busMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::ONE}};
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  dfl::inputs::DynamicDataBaseManager manager("", "");
  dfl::algo::GeneratorDefinitionAlgorithm algo_infinite(generators, busMap, manager, false, 10.);

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
