//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  TestAlgoDynModel.cpp
 *
 * @brief DynModel Algo library test file
 *
 */

#include "DynModelDefinitionAlgorithm.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "LoadDefinitionAlgorithm.h"
#include "MainConnexComponentAlgorithm.h"
#include "SVarCDefinitionAlgorithm.h"
#include "ShuntDefinitionAlgorithm.h"
#include "SlackNodeAlgorithm.h"
#include "Tests.h"

#include <DYNMultiProcessingContext.h>

#include <algorithm>
#include <vector>

// Required for testing unit tests
testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::multiprocessing::Context mpiContext;

namespace test {
/**
 * @brief Implement Service manager interface stub for testing purpose
 *
 * This aims to stub the Dynawo service manager interface, to avoid using dynawo library during unit testing.
 */
class TestAlgoServiceManagerInterface : public DYN::ServiceManagerInterface {
 public:
  /**
   * Add a disconnected node
   * @param nodeId id of the disconnected node
   */
  void addDisconnectedNode(const std::string &nodeId) { disconnectedNodes_.insert(nodeId); }

  /**
   * @copydoc DYN::ServiceManagerInterface::getBusesConnectedBySwitch
   */
  std::vector<std::string> getBusesConnectedBySwitch(const std::string &busId, const std::string &VLId) const final { return std::vector<std::string>(); }

  /**
   * @copydoc DYN::ServiceManagerInterface::getBusesConnectedBySwitch
   */
  bool isBusConnected(const std::string &busId, const std::string &VLId) const final { return disconnectedNodes_.find(busId) == disconnectedNodes_.end(); }

  /**
   * @copydoc DYN::ServiceManagerInterface::getRegulatedBus
   */
  std::shared_ptr<DYN::BusInterface> getRegulatedBus(const std::string &regulatingComponent) const final { return std::shared_ptr<DYN::BusInterface>(); }

 private:
  std::unordered_set<std::string> disconnectedNodes_;
};
}  // namespace test

TEST(TestAlgoDynModel, base) {
  using dfl::algo::DynamicModelDefinitions;
  using dfl::inputs::DynamicDataBaseManager;

  DynamicDataBaseManager manager(std::vector<boost::filesystem::path>(1, "res/setting.xml"),
                                 std::vector<boost::filesystem::path>(1, "res/assembling.xml"));
  DynamicModelDefinitions defs;
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VLb");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  std::vector<dfl::inputs::Shunt> shunts3 = {dfl::inputs::Shunt("3.1"), dfl::inputs::Shunt("3.2"), dfl::inputs::Shunt("3.3")};
  std::vector<dfl::inputs::Shunt> shunts4 = {dfl::inputs::Shunt("4.1"), dfl::inputs::Shunt("4.2"), dfl::inputs::Shunt("4.3"), dfl::inputs::Shunt("4.4")};
  std::vector<dfl::inputs::Shunt> shunts5 = {dfl::inputs::Shunt("5.1"), dfl::inputs::Shunt("5.2"), dfl::inputs::Shunt("5.3"), dfl::inputs::Shunt("5.4"),
                                             dfl::inputs::Shunt("5.5")};
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("VL0", vl2, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("VL1", vl, 1.0, shunts1, false, testServiceManager),
      dfl::inputs::Node::build("VL2", vl, 2.0, shunts2, false, testServiceManager),
      dfl::inputs::Node::build("VL3", vl, 3.0, shunts3, false, testServiceManager),
      dfl::inputs::Node::build("VL4", vl, 4.0, shunts4, false, testServiceManager),
      dfl::inputs::Node::build("VL5", vl, 5.0, shunts5, false, testServiceManager),
      dfl::inputs::Node::build("VL6", vl, 0.0, {}, false, testServiceManager),
  };
  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{
      dfl::inputs::Line::build("0", nodes[0], nodes[1], "UNDEFINED", true, true), dfl::inputs::Line::build("1", nodes[0], nodes[2], "UNDEFINED", true, true),
      dfl::inputs::Line::build("2", nodes[0], nodes[3], "UNDEFINED", true, true), dfl::inputs::Line::build("3", nodes[3], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("4", nodes[2], nodes[4], "UNDEFINED", true, true), dfl::inputs::Line::build("5", nodes[1], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("6", nodes[5], nodes[6], "UNDEFINED", true, true),
  };

  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "BUS_2", nullptr, false, 0., 0., 0.,
                                                                     std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "BUS_1", nullptr, false, 0., 0., 0.,
                                                                 std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto hvdcLineVSC = dfl::inputs::HvdcLine::build("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, dummyStationVSC, vscStation2, boost::none, 10.,
                                                  false, 320, 322, 0.125, {0.01, 0.01});

  auto tfo = dfl::inputs::Tfo::build("TFO1", nodes[2], nodes[3], "UNDEFINED", true, true);

  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  const std::string bus3 = "BUS_3";
  const std::string bus4 = "BUS_4";
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));

  nodes[1]->generators.emplace_back("G0", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[1]->generators.emplace_back("G3", true, points, -1, 1, -1, 1, 0, 0, 0, bus1, bus3);

  nodes[0]->generators.emplace_back("G1", true, points, -2, 2, -2, 2, 0, 0, 0, bus2, bus2);
  nodes[0]->converters.emplace_back(vscStation2);

  nodes[1]->loads.emplace_back("L0", false, false);
  nodes[1]->loads.emplace_back("L3", false, false);

  nodes[0]->loads.emplace_back("L1", false, false);

  dfl::algo::DynModelAlgorithm algo(defs, manager, true);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());

  for (const auto &node : nodes) {
    algo(node, algoRes);
  }

  ASSERT_EQ(defs.usedMacroConnections.size(), 12);
  std::set<std::string> usedMacroConnections(defs.usedMacroConnections.begin(), defs.usedMacroConnections.end());
  const std::vector<std::string> usedMacroConnectionsRef = {"ToUMeasurement",
                                                            "ToControlledShunts",
                                                            "CLAToIMeasurement",
                                                            "PhaseShifterToAutomatonActivated",
                                                            "CLAToControlledLineState",
                                                            "CLAToAutomatonActivated",
                                                            "PhaseShifterToIMeasurement",
                                                            "PhaseShifterToTap",
                                                            "SVCToUMeasurement",
                                                            "SVCToGenerator",
                                                            "SVCToLoad",
                                                            "SVCToHVDC"};
  std::set<std::string> usedMacroConnectionsRefSet(usedMacroConnectionsRef.begin(), usedMacroConnectionsRef.end());
  ASSERT_EQ(usedMacroConnections, usedMacroConnectionsRefSet);

  ASSERT_EQ(defs.models.size(), 6);
  // bus
  ASSERT_NO_THROW(defs.models.at("MODELE_1_VL4"));
  const auto &dynModel = defs.models.at("MODELE_1_VL4");
  ASSERT_EQ(dynModel.id, "MODELE_1_VL4");
  ASSERT_EQ(dynModel.lib, "dummyLib");
  ASSERT_EQ(dynModel.nodeConnections.size(), 16);

  std::string searched = "ToUMeasurement";
  auto found_connection = std::find_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                                       [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE);
  auto counter = std::count_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                               [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 1);

  searched = "ToControlledShunts";
  counter = std::count_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                          [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 15);

  // Line
  ASSERT_NO_THROW(defs.models.at("DM_SALON"));
  const auto &dynModelCLA = defs.models.at("DM_SALON");
  ASSERT_EQ(dynModelCLA.id, "DM_SALON");
  ASSERT_EQ(dynModelCLA.lib, "dummyLib");
  ASSERT_EQ(dynModelCLA.nodeConnections.size(), 3);

  searched = "CLAToControlledLineState";
  found_connection = std::find_if(dynModelCLA.nodeConnections.begin(), dynModelCLA.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModelCLA.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LINE);

  // TFO
  ASSERT_NO_THROW(defs.models.at("DM_VL661"));
  const auto &dynModel_tfo = defs.models.at("DM_VL661");
  ASSERT_EQ(dynModel_tfo.id, "DM_VL661");
  ASSERT_EQ(dynModel_tfo.lib, "dummyLib");
  ASSERT_EQ(dynModel_tfo.nodeConnections.size(), 3);
  searched = "PhaseShifterToIMeasurement";
  found_connection = std::find_if(dynModel_tfo.nodeConnections.begin(), dynModel_tfo.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel_tfo.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "TFO1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO);

  // GENERATORS
  ASSERT_NO_THROW(defs.models.at("GeneratorAutomaton"));
  const auto &dynModel_gen = defs.models.at("GeneratorAutomaton");
  ASSERT_EQ(dynModel_gen.id, "GeneratorAutomaton");
  ASSERT_EQ(dynModel_gen.lib, "dummyLib");
  ASSERT_EQ(dynModel_gen.nodeConnections.size(), 6);
  searched = "SVCToUMeasurement";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE);
  searched = "SVCToGenerator";
  std::string searched2 = "G0";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "G0");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR);
  searched2 = "G3";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "G3");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR);

  searched = "SVCToLoad";
  searched2 = "L0";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "L0");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LOAD);
  searched2 = "L3";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "L3");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LOAD);

  searched = "SVCToHVDC";
  searched2 = "HVDCVSCLine";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "HVDCVSCLine");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::HVDC);
}

TEST(TestAlgoDynModel, noRegulation) {
  using dfl::algo::DynamicModelDefinitions;
  using dfl::inputs::DynamicDataBaseManager;

  DynamicDataBaseManager manager(std::vector<boost::filesystem::path>(1, "res/setting.xml"),
                                 std::vector<boost::filesystem::path>(1, "res/assembling.xml"));
  DynamicModelDefinitions defs;
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VLb");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  std::vector<dfl::inputs::Shunt> shunts3 = {dfl::inputs::Shunt("3.1"), dfl::inputs::Shunt("3.2"), dfl::inputs::Shunt("3.3")};
  std::vector<dfl::inputs::Shunt> shunts4 = {dfl::inputs::Shunt("4.1"), dfl::inputs::Shunt("4.2"), dfl::inputs::Shunt("4.3"), dfl::inputs::Shunt("4.4")};
  std::vector<dfl::inputs::Shunt> shunts5 = {dfl::inputs::Shunt("5.1"), dfl::inputs::Shunt("5.2"), dfl::inputs::Shunt("5.3"), dfl::inputs::Shunt("5.4"),
                                             dfl::inputs::Shunt("5.5")};

  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("VL0", vl2, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("VL1", vl, 1.0, shunts1, false, testServiceManager),
      dfl::inputs::Node::build("VL2", vl, 2.0, shunts2, false, testServiceManager),
      dfl::inputs::Node::build("VL3", vl, 3.0, shunts3, false, testServiceManager),
      dfl::inputs::Node::build("VL4", vl, 4.0, shunts4, false, testServiceManager),
      dfl::inputs::Node::build("VL5", vl, 5.0, shunts5, false, testServiceManager),
      dfl::inputs::Node::build("VL6", vl, 0.0, {}, false, testServiceManager),
  };
  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{
      dfl::inputs::Line::build("0", nodes[0], nodes[1], "UNDEFINED", true, true), dfl::inputs::Line::build("1", nodes[0], nodes[2], "UNDEFINED", true, true),
      dfl::inputs::Line::build("2", nodes[0], nodes[3], "UNDEFINED", true, true), dfl::inputs::Line::build("3", nodes[3], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("4", nodes[2], nodes[4], "UNDEFINED", true, true), dfl::inputs::Line::build("5", nodes[1], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("6", nodes[5], nodes[6], "UNDEFINED", true, true),
  };

  auto tfo = dfl::inputs::Tfo::build("TFO1", nodes[2], nodes[3], "UNDEFINED", true, true);

  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  const std::string bus3 = "BUS_3";
  const std::string bus4 = "BUS_4";
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));

  nodes[1]->generators.emplace_back("G0", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[1]->generators.emplace_back("G3", true, points, -1, 1, -1, 1, 0, 0, 0, bus1, bus3);

  nodes[0]->generators.emplace_back("G1", true, points, -2, 2, -2, 2, 0, 0, 0, bus2, bus2);

  dfl::algo::DynModelAlgorithm algo(defs, manager, false);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());

  for (const auto &node : nodes) {
    algo(node, algoRes);
  }

  ASSERT_EQ(defs.usedMacroConnections.size(), 9);
  std::set<std::string> usedMacroConnections(defs.usedMacroConnections.begin(), defs.usedMacroConnections.end());
  const std::vector<std::string> usedMacroConnectionsRef = {"ToUMeasurement",
                                                            "CLAToIMeasurement",
                                                            "CLAToControlledLineState",
                                                            "CLAToAutomatonActivated",
                                                            "PhaseShifterToIMeasurement",
                                                            "PhaseShifterToTap",
                                                            "PhaseShifterToAutomatonActivated",
                                                            "SVCToUMeasurement",
                                                            "SVCToGenerator"};
  std::set<std::string> usedMacroConnectionsRefSet(usedMacroConnectionsRef.begin(), usedMacroConnectionsRef.end());
  ASSERT_EQ(usedMacroConnections, usedMacroConnectionsRefSet);

  ASSERT_EQ(defs.models.size(), 6);
  // bus
  ASSERT_NO_THROW(defs.models.at("MODELE_1_VL4"));
  const auto &dynModel = defs.models.at("MODELE_1_VL4");
  ASSERT_EQ(dynModel.id, "MODELE_1_VL4");
  ASSERT_EQ(dynModel.lib, "dummyLib");
  ASSERT_EQ(dynModel.nodeConnections.size(), 1);  // only bus Umesurement is connected

  std::string searched = "ToUMeasurement";
  auto found_connection = std::find_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                                       [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE);
  auto counter = std::count_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                               [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 1);

  searched = "ToControlledShunts";
  counter = std::count_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                          [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 0);  // Not found here because shunt regulation is off

  // Line
  ASSERT_NO_THROW(defs.models.at("DM_SALON"));
  const auto &dynModelCLA = defs.models.at("DM_SALON");
  ASSERT_EQ(dynModelCLA.id, "DM_SALON");
  ASSERT_EQ(dynModelCLA.lib, "dummyLib");
  ASSERT_EQ(dynModelCLA.nodeConnections.size(), 3);

  searched = "CLAToControlledLineState";
  found_connection = std::find_if(dynModelCLA.nodeConnections.begin(), dynModelCLA.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModelCLA.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LINE);

  // TFO
  ASSERT_NO_THROW(defs.models.at("DM_VL661"));
  const auto &dynModel_tfo = defs.models.at("DM_VL661");
  ASSERT_EQ(dynModel_tfo.id, "DM_VL661");
  ASSERT_EQ(dynModel_tfo.lib, "dummyLib");
  ASSERT_EQ(dynModel_tfo.nodeConnections.size(), 3);
  searched = "PhaseShifterToIMeasurement";
  found_connection = std::find_if(dynModel_tfo.nodeConnections.begin(), dynModel_tfo.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel_tfo.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "TFO1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO);

  // GENERATORS
  ASSERT_NO_THROW(defs.models.at("GeneratorAutomaton"));
  const auto &dynModel_gen = defs.models.at("GeneratorAutomaton");
  ASSERT_EQ(dynModel_gen.id, "GeneratorAutomaton");
  ASSERT_EQ(dynModel_gen.lib, "dummyLib");
  ASSERT_EQ(dynModel_gen.nodeConnections.size(), 3);
  searched = "SVCToUMeasurement";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE);
  searched = "SVCToGenerator";
  std::string searched2 = "G0";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "G0");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR);
  searched2 = "G3";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "G3");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR);
}

TEST(TestAlgoDynModel, noRegulationAndBusDisconnected) {
  using dfl::algo::DynamicModelDefinitions;
  using dfl::inputs::DynamicDataBaseManager;

  DynamicDataBaseManager manager(std::vector<boost::filesystem::path>(1, "res/setting.xml"),
                                 std::vector<boost::filesystem::path>(1, "res/assembling.xml"));
  DynamicModelDefinitions defs;
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  testServiceManager->addDisconnectedNode("VL1");
  testServiceManager->addDisconnectedNode("VL2");
  testServiceManager->addDisconnectedNode("VL3");
  testServiceManager->addDisconnectedNode("VL4");
  testServiceManager->addDisconnectedNode("VL5");

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VLb");
  std::vector<dfl::inputs::Shunt> shunts1 = {dfl::inputs::Shunt("1.1")};
  std::vector<dfl::inputs::Shunt> shunts2 = {dfl::inputs::Shunt("2.1"), dfl::inputs::Shunt("2.2")};
  std::vector<dfl::inputs::Shunt> shunts3 = {dfl::inputs::Shunt("3.1"), dfl::inputs::Shunt("3.2"), dfl::inputs::Shunt("3.3")};
  std::vector<dfl::inputs::Shunt> shunts4 = {dfl::inputs::Shunt("4.1"), dfl::inputs::Shunt("4.2"), dfl::inputs::Shunt("4.3"), dfl::inputs::Shunt("4.4")};
  std::vector<dfl::inputs::Shunt> shunts5 = {dfl::inputs::Shunt("5.1"), dfl::inputs::Shunt("5.2"), dfl::inputs::Shunt("5.3"), dfl::inputs::Shunt("5.4"),
                                             dfl::inputs::Shunt("5.5")};

  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("VL0", vl2, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("VL1", vl, 1.0, shunts1, false, testServiceManager),
      dfl::inputs::Node::build("VL2", vl, 2.0, shunts2, false, testServiceManager),
      dfl::inputs::Node::build("VL3", vl, 3.0, shunts3, false, testServiceManager),
      dfl::inputs::Node::build("VL4", vl, 4.0, shunts4, false, testServiceManager),
      dfl::inputs::Node::build("VL5", vl, 5.0, shunts5, false, testServiceManager),
      dfl::inputs::Node::build("VL6", vl, 0.0, {}, false, testServiceManager),
  };
  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{
      dfl::inputs::Line::build("0", nodes[0], nodes[1], "UNDEFINED", true, true), dfl::inputs::Line::build("1", nodes[0], nodes[2], "UNDEFINED", true, true),
      dfl::inputs::Line::build("2", nodes[0], nodes[3], "UNDEFINED", true, true), dfl::inputs::Line::build("3", nodes[3], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("4", nodes[2], nodes[4], "UNDEFINED", true, true), dfl::inputs::Line::build("5", nodes[1], nodes[4], "UNDEFINED", true, true),
      dfl::inputs::Line::build("6", nodes[5], nodes[6], "UNDEFINED", true, true),
  };

  auto tfo = dfl::inputs::Tfo::build("TFO1", nodes[2], nodes[3], "UNDEFINED", true, true);

  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  const std::string bus3 = "BUS_3";
  const std::string bus4 = "BUS_4";
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points0;
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(2, -10, -10));
  points0.push_back(dfl::inputs::Generator::ReactiveCurvePoint(1, 1, 17));

  nodes[1]->generators.emplace_back("G0", true, points0, 0, 0, 0, 0, 0, 0, 0, bus1, bus1);
  nodes[1]->generators.emplace_back("G3", true, points, -1, 1, -1, 1, 0, 0, 0, bus1, bus3);

  nodes[0]->generators.emplace_back("G1", true, points, -2, 2, -2, 2, 0, 0, 0, bus2, bus2);

  dfl::algo::DynModelAlgorithm algo(defs, manager, false);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());

  for (const auto &node : nodes) {
    algo(node, algoRes);
  }

  ASSERT_EQ(defs.usedMacroConnections.size(), 9);
  std::set<std::string> usedMacroConnections(defs.usedMacroConnections.begin(), defs.usedMacroConnections.end());
  const std::vector<std::string> usedMacroConnectionsRef = {"ToUMeasurement",
                                                            "CLAToIMeasurement",
                                                            "CLAToControlledLineState",
                                                            "CLAToAutomatonActivated",
                                                            "PhaseShifterToIMeasurement",
                                                            "PhaseShifterToTap",
                                                            "PhaseShifterToAutomatonActivated",
                                                            "SVCToUMeasurement",
                                                            "SVCToGenerator"};
  std::set<std::string> usedMacroConnectionsRefSet(usedMacroConnectionsRef.begin(), usedMacroConnectionsRef.end());
  ASSERT_EQ(usedMacroConnections, usedMacroConnectionsRefSet);

  ASSERT_EQ(defs.models.size(), 6);
  // bus
  ASSERT_NO_THROW(defs.models.at("MODELE_1_VL4"));
  const auto &dynModel = defs.models.at("MODELE_1_VL4");
  ASSERT_EQ(dynModel.id, "MODELE_1_VL4");
  ASSERT_EQ(dynModel.lib, "dummyLib");
  ASSERT_EQ(dynModel.nodeConnections.size(), 1);  // only bus Umesurement is connected

  std::string searched = "ToUMeasurement";
  auto found_connection = std::find_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                                       [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL6");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE);
  auto counter = std::count_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                               [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 1);

  searched = "ToControlledShunts";
  counter = std::count_if(dynModel.nodeConnections.begin(), dynModel.nodeConnections.end(),
                          [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 0);  // Not found here because shunt regulation is off

  // Line
  ASSERT_NO_THROW(defs.models.at("DM_SALON"));
  const auto &dynModelCLA = defs.models.at("DM_SALON");
  ASSERT_EQ(dynModelCLA.id, "DM_SALON");
  ASSERT_EQ(dynModelCLA.lib, "dummyLib");
  ASSERT_EQ(dynModelCLA.nodeConnections.size(), 3);

  searched = "CLAToControlledLineState";
  found_connection = std::find_if(dynModelCLA.nodeConnections.begin(), dynModelCLA.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModelCLA.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LINE);

  // TFO
  ASSERT_NO_THROW(defs.models.at("DM_VL661"));
  const auto &dynModel_tfo = defs.models.at("DM_VL661");
  ASSERT_EQ(dynModel_tfo.id, "DM_VL661");
  ASSERT_EQ(dynModel_tfo.lib, "dummyLib");
  ASSERT_EQ(dynModel_tfo.nodeConnections.size(), 3);
  searched = "PhaseShifterToIMeasurement";
  found_connection = std::find_if(dynModel_tfo.nodeConnections.begin(), dynModel_tfo.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel_tfo.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "TFO1");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO);

  // GENERATORS
  ASSERT_NO_THROW(defs.models.at("GeneratorAutomaton"));
  const auto &dynModel_gen = defs.models.at("GeneratorAutomaton");
  ASSERT_EQ(dynModel_gen.id, "GeneratorAutomaton");
  ASSERT_EQ(dynModel_gen.lib, "dummyLib");
  ASSERT_EQ(dynModel_gen.nodeConnections.size(), 3);
  searched = "SVCToUMeasurement";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL6");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE);
  searched = "SVCToGenerator";
  std::string searched2 = "G0";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "G0");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR);
  searched2 = "G3";
  found_connection = std::find_if(dynModel_gen.nodeConnections.begin(), dynModel_gen.nodeConnections.end(),
                                  [&searched, &searched2](const dfl::algo::DynamicModelDefinition::MacroConnection &connection) {
                                    return connection.id == searched && connection.connectedElementId == searched2;
                                  });
  ASSERT_NE(found_connection, dynModel_gen.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "G3");
  ASSERT_EQ(found_connection->elementType, dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR);
}
