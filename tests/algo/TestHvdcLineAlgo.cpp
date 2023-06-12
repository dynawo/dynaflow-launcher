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
 * @file  TestHvdcLineAlgo.cpp
 *
 * @brief HVDCDefinition library test file
 */

#include "HVDCDefinitionAlgorithm.h"
#include "Tests.h"

#include <DYNMPIContext.h>

// Required for testing unit tests
testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::mpi::Context mpiContext;

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

static bool hvdcLineDefinitionEqual(const dfl::algo::HVDCDefinition &lhs, const dfl::algo::HVDCDefinition &rhs) {
  // we do not check the model here as a dedicated test will check the compliance
  return lhs.id == rhs.id && lhs.converterType == rhs.converterType && lhs.converter1Id == rhs.converter1Id && lhs.converter1BusId == rhs.converter1BusId &&
         lhs.converter2VoltageRegulationOn == rhs.converter2VoltageRegulationOn && lhs.converter2Id == rhs.converter2Id &&
         lhs.converter2BusId == rhs.converter2BusId && lhs.converter2VoltageRegulationOn == rhs.converter2VoltageRegulationOn && lhs.position == rhs.position &&
         lhs.pMax == rhs.pMax && lhs.powerFactors == rhs.powerFactors && boost::equal_pointees(lhs.vscDefinition1, rhs.vscDefinition1) &&
         boost::equal_pointees(lhs.vscDefinition2, rhs.vscDefinition2);
}

TEST(HvdcLine, base) {
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  dfl::inputs::DynamicDataBaseManager manager("", "");
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0, {}, false, testServiceManager), dfl::inputs::Node::build("1", vl, 111.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 24.0, {}, false, testServiceManager), dfl::inputs::Node::build("3", vl, 63.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl, 56.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl, 46.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl, 0.0, {}, false, testServiceManager),
  };
  auto dummyStation = std::make_shared<dfl::inputs::LCCConverter>("StationN", "_BUS___99_TN", nullptr, 99.);
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "_BUS___99_TN", nullptr, false, 0., 0., 0.,
                                                                     std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto lccStation1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation1", "_BUS___11_TN", nullptr, 1.);
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "_BUS___11_TN", nullptr, false, 0., 0., 0.,
                                                                 std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto lccStationMain1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStationMain1", "_BUS__11_TN", nullptr, 1.);

  auto lccStationMain2 = std::make_shared<dfl::inputs::LCCConverter>("LCCStationMain2", "_BUS__11_TN", nullptr, 2.);

  auto hvdcLineLCC = dfl::inputs::HvdcLine::build("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation1, dummyStation, boost::none, 0.0, false,
                                                  320, 322, 0.125, {0.01, 0.01});
  auto hvdcLineVSC = dfl::inputs::HvdcLine::build("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, dummyStationVSC, vscStation2, boost::none, 10.,
                                                  false, 320, 322, 0.125, {0.01, 0.01});
  auto hvdcLineBothInMainComponent = dfl::inputs::HvdcLine::build("HVDCLineBothInMain", dfl::inputs::HvdcLine::ConverterType::LCC, lccStationMain1,
                                                                  lccStationMain2, boost::none, 20., false, 320, 322, 0.125, {0.01, 0.01});

  // model not checked in this test : see the dedicated test
  std::vector<dfl::algo::HVDCDefinition> expected_hvdcLines = {
      dfl::algo::HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", boost::none, "StationN",
                                "_BUS___99_TN", boost::none, dfl::algo::HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT,
                                dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling, {1., 99.}, 0., boost::none, boost::none, boost::none, boost::none, false,
                                320, 322, 0.125, {0.01, 0.01}),
      dfl::algo::HVDCDefinition(
          "HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "StationN", "_BUS___99_TN", false, "VSCStation2", "_BUS___11_TN", false,
          dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling, {0., 0.}, 10.,
          dfl::algo::VSCDefinition(dummyStationVSC->converterId, dummyStationVSC->qMax, dummyStationVSC->qMin, dummyStationVSC->qMin, 10.,
                                   dummyStationVSC->points),
          dfl::algo::VSCDefinition(vscStation2->converterId, vscStation2->qMax, vscStation2->qMin, vscStation2->qMin, 10., vscStation2->points), boost::none,
          boost::none, false, 320, 322, 0.125, {0.01, 0.01}),
      dfl::algo::HVDCDefinition("HVDCLineBothInMain", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStationMain1", "_BUS__11_TN", boost::none,
                                "LCCStationMain2", "_BUS__11_TN", boost::none, dfl::algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT,
                                dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling, {1., 2.}, 20., boost::none, boost::none, boost::none, boost::none, false,
                                320, 322, 0.125, {0.01, 0.01}),
  };

  nodes[0]->converters.emplace_back(lccStation1);
  nodes[2]->converters.emplace_back(lccStationMain1);
  nodes[0]->converters.emplace_back(lccStationMain2);
  nodes[4]->converters.emplace_back(vscStation2);

  dfl::algo::HVDCLineDefinitions hvdcDefs;
  constexpr bool useReactiveLimits = true;
  dfl::inputs::NetworkManager::BusMapRegulating map;
  std::unordered_set<std::shared_ptr<dfl::inputs::Converter>> set{vscStation2};
  dfl::algo::HVDCDefinitionAlgorithm algo(hvdcDefs, useReactiveLimits, set, map, manager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo(node, algoRes);
  }

  const auto &hvdcLines = hvdcDefs.hvdcLines;
  ASSERT_EQ(3, hvdcLines.size());
  size_t index = 0;
  for (const auto &expected_hvdcLine : expected_hvdcLines) {
    auto it = hvdcLines.find(expected_hvdcLine.id);
    ASSERT_NE(hvdcLines.end(), it);
    ASSERT_TRUE(hvdcLineDefinitionEqual(expected_hvdcLine, it->second)) << " Fail for " << expected_hvdcLine.id;
  }
  ASSERT_EQ(hvdcDefs.vscBusVSCDefinitionsMap.size(), 0);
}

static bool compareVSCDefinition(const dfl::algo::VSCDefinition &lhs, const dfl::algo::VSCDefinition &rhs) {
  return lhs.id == rhs.id && lhs.qmax == rhs.qmax && lhs.qmin == rhs.qmin && lhs.points.size() == rhs.points.size() &&
         std::equal(lhs.points.begin(), lhs.points.end(), rhs.points.begin(),
                    [](const dfl::algo::VSCDefinition::ReactiveCurvePoint &lhs, const dfl::algo::VSCDefinition::ReactiveCurvePoint &rhs) {
                      return lhs.p == rhs.p && lhs.qmax == rhs.qmax && lhs.qmin == rhs.qmin;
                    });
}

static void checkVSCIds(const dfl::algo::HVDCLineDefinitions &hvdcDefs) {
  static const dfl::algo::HVDCLineDefinitions::BusVSCMap expectedMap = {std::make_pair("2", "VSCStation2"), std::make_pair("7", "VSCStation7"),
                                                                        std::make_pair("8", "VSCStation8"), std::make_pair("11", "VSCStation11"),
                                                                        std::make_pair("12", "VSCStation12")};
  ASSERT_EQ(hvdcDefs.vscBusVSCDefinitionsMap.size(), expectedMap.size());
  for (const auto &pair : expectedMap) {
    ASSERT_GT(hvdcDefs.vscBusVSCDefinitionsMap.count(pair.first), 0);
    ASSERT_EQ(hvdcDefs.vscBusVSCDefinitionsMap.at(pair.first), pair.second);
  }
}

TEST(hvdcLine, models) {
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0, {}, false, testServiceManager), dfl::inputs::Node::build("1", vl, 111.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 24.0, {}, false, testServiceManager), dfl::inputs::Node::build("3", vl, 63.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl, 56.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl, 46.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("7", vl, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("8", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("9", vl, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("10", vl, 0.0, {}, false, testServiceManager), dfl::inputs::Node::build("11", vl, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("12", vl, 0.0, {}, false, testServiceManager),
  };
  std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint> emptyPoints{};

  auto activeControl = boost::optional<dfl::inputs::HvdcLine::ActivePowerControl>(dfl::inputs::HvdcLine::ActivePowerControl(10., 5.));

  auto dummyStation = std::make_shared<dfl::inputs::LCCConverter>("StationN", "_BUS___99_TN", nullptr, 1.);
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "_BUS___99_TN", nullptr, false, 0., 0., 0., emptyPoints);
  auto lccStation1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation1", "0", nullptr, 1.);
  auto lccStation3 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation3", "3", nullptr, 1.);
  auto lccStation4 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation4", "4", nullptr, 1.);
  auto vscStation1 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation1", "1", nullptr, false, 1.1, 1., 1., emptyPoints);
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "2", nullptr, true, 2.1, 2., 2., emptyPoints);
  auto vscStation21 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation21", "2", nullptr, true, 2.1, 2., 2, emptyPoints);
  auto vscStation22 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation22", "2", nullptr, true, 2.1, 2., 2, emptyPoints);
  auto vscStation23 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation23", "2", nullptr, true, 2.1, 2., 2, emptyPoints);
  auto vscStation5 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation5", "5", nullptr, true, 5.1, 5., 5, emptyPoints);
  auto vscStation6 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation6", "6", nullptr, true, 6.1, 6., 6, emptyPoints);
  auto vscStation7 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation7", "7", nullptr, true, 7.1, 7., 7, emptyPoints);
  auto vscStation8 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation8", "8", nullptr, true, 8.1, 8., 8, emptyPoints);
  auto vscStation9 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation9", "9", nullptr, true, 9.1, 9., 9, emptyPoints);
  auto vscStation10 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation10", "10", nullptr, true, 10.1, 10., 10, emptyPoints);
  auto vscStation11 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation11", "11", nullptr, true, 11.1, 11., 11, emptyPoints);
  auto vscStation12 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation12", "12", nullptr, true, 12.1, 12., 12, emptyPoints);
  auto hvdcLineLCC = dfl::inputs::HvdcLine::build("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation1, dummyStation, boost::none, 0, false,
                                                  320, 322, 0.125, {0.01, 0.01});  // first is in main cc
  auto hvdcLineVSC = dfl::inputs::HvdcLine::build("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation1, dummyStationVSC, boost::none, 1, false,
                                                  320, 322, 0.125, {0.01, 0.01});  // first in main cc
  auto hvdcLineVSC2 = dfl::inputs::HvdcLine::build("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, dummyStationVSC, vscStation2, boost::none, 2,
                                                   false, 320, 322, 0.125, {0.01, 0.01});  // second in main cc
  auto hvdcLineVSC3 = dfl::inputs::HvdcLine::build("HVDCVSCLine3", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation21, dummyStationVSC, boost::none, 2,
                                                   false, 320, 322, 0.125, {0.01, 0.01});  // first in main cc
  auto hvdcLineBothInMainComponent = dfl::inputs::HvdcLine::build("HVDCLineBothInMain1", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation3, lccStation4,
                                                                  boost::none, 3.4, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent2 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain2", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation5, vscStation6,
                                                                   activeControl, 5.6, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent3 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain3", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation22, vscStation7,
                                                                   activeControl, 2.7, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent4 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain4", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation9, vscStation10,
                                                                   boost::none, 9.10, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent5 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain5", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation23, vscStation8,
                                                                   boost::none, 2.8, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineVSCSwitch1 = dfl::inputs::HvdcLine::build("HVDCVSCLineSwitch1", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation11, dummyStationVSC,
                                                         boost::none, 11, false, 320, 322, 0.125, {0.01, 0.01});  // first in main cc
  auto hvdcLineVSCSwitch2 = dfl::inputs::HvdcLine::build("HVDCVSCLineSwitch2", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation12, dummyStationVSC,
                                                         boost::none, 12, false, 320, 322, 0.125, {0.01, 0.01});  // first in main cc
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
  dfl::inputs::DynamicDataBaseManager manager("", "");
  dfl::algo::HVDCDefinitionAlgorithm algo(hvdcDefs, useReactiveLimits, set, busMap, manager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo(node, algoRes);
  }

  auto &hvdcLines = hvdcDefs.hvdcLines;
  ASSERT_EQ(hvdcLines.size(), 11);
  ASSERT_EQ(hvdcLines.at("HVDCLCCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhi);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSet);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulationSet);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain4").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPV);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain5").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQProp);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling);

  checkVSCIds(hvdcDefs);

  hvdcLines.clear();
  hvdcDefs.vscBusVSCDefinitionsMap.clear();
  // case diagrams
  useReactiveLimits = false;
  dfl::algo::HVDCDefinitionAlgorithm algo2(hvdcDefs, useReactiveLimits, set, busMap, manager);
  for (const auto &node : nodes) {
    algo2(node, algoRes);
  }

  ASSERT_EQ(hvdcLines.size(), 11);
  ASSERT_EQ(hvdcLines.at("HVDCLCCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulationSet);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulationSet);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain4").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain5").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ);

  checkVSCIds(hvdcDefs);
}

TEST(hvdcLineSVC, models) {
  auto testServiceManager = boost::make_shared<test::TestAlgoServiceManagerInterface>();
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 98.0, {}, false, testServiceManager), dfl::inputs::Node::build("1", vl, 111.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("2", vl, 24.0, {}, false, testServiceManager), dfl::inputs::Node::build("3", vl, 63.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("4", vl, 56.0, {}, false, testServiceManager), dfl::inputs::Node::build("5", vl, 46.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("6", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("7", vl, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("8", vl, 0.0, {}, false, testServiceManager),  dfl::inputs::Node::build("9", vl, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("10", vl, 0.0, {}, false, testServiceManager), dfl::inputs::Node::build("11", vl, 0.0, {}, false, testServiceManager),
      dfl::inputs::Node::build("12", vl, 0.0, {}, false, testServiceManager),
  };
  std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint> emptyPoints{};

  auto activeControl = boost::optional<dfl::inputs::HvdcLine::ActivePowerControl>(dfl::inputs::HvdcLine::ActivePowerControl(10., 5.));

  auto dummyStation = std::make_shared<dfl::inputs::LCCConverter>("StationN", "_BUS___99_TN", nullptr, 1.);
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "_BUS___99_TN", nullptr, false, 0., 0., 0., emptyPoints);
  auto lccStation1 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation1", "0", nullptr, 1.);
  auto lccStation3 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation3", "3", nullptr, 1.);
  auto lccStation4 = std::make_shared<dfl::inputs::LCCConverter>("LCCStation4", "4", nullptr, 1.);
  auto vscStation1 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation1", "1", nullptr, false, 1.1, 1., 1., emptyPoints);
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "2", nullptr, true, 2.1, 2., 2., emptyPoints);
  auto vscStation21 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation21", "2", nullptr, true, 2.1, 2., 2., emptyPoints);
  auto vscStation22 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation22", "2", nullptr, true, 2.1, 2., 2., emptyPoints);
  auto vscStation23 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation23", "2", nullptr, true, 2.1, 2., 2., emptyPoints);
  auto vscStation5 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation5", "5", nullptr, true, 5.1, 5., 5., emptyPoints);
  auto vscStation6 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation6", "6", nullptr, true, 6.1, 6., 6., emptyPoints);
  auto vscStation7 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation7", "7", nullptr, true, 7.1, 7., 7., emptyPoints);
  auto vscStation8 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation8", "8", nullptr, true, 8.1, 8., 8., emptyPoints);
  auto vscStation9 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation9", "9", nullptr, true, 9.1, 9., 9., emptyPoints);
  auto vscStation10 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation10", "10", nullptr, true, 10.1, 10., 10., emptyPoints);
  auto vscStation11 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation11", "11", nullptr, true, 11.1, 11., 11., emptyPoints);
  auto vscStation12 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation12", "12", nullptr, true, 12.1, 12., 12., emptyPoints);
  auto hvdcLineLCC = dfl::inputs::HvdcLine::build("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation1, dummyStation, boost::none, 0, false,
                                                  320, 322, 0.125, {0.01, 0.01});  // first is in main cc
  auto hvdcLineVSC = dfl::inputs::HvdcLine::build("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation1, dummyStationVSC, boost::none, 1, false,
                                                  320, 322, 0.125, {0.01, 0.01});  // first in main cc
  auto hvdcLineVSC2 = dfl::inputs::HvdcLine::build("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, dummyStationVSC, vscStation2, boost::none, 2,
                                                   false, 320, 322, 0.125, {0.01, 0.01});  // second in main cc
  auto hvdcLineVSC3 = dfl::inputs::HvdcLine::build("HVDCVSCLine3", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation21, dummyStationVSC, boost::none, 2,
                                                   false, 320, 322, 0.125, {0.01, 0.01});  // first in main cc
  auto hvdcLineBothInMainComponent = dfl::inputs::HvdcLine::build("HVDCLineBothInMain1", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation3, lccStation4,
                                                                  boost::none, 3.4, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent2 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain2", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation5, vscStation6,
                                                                   activeControl, 5.6, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent3 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain3", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation22, vscStation7,
                                                                   activeControl, 2.7, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent4 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain4", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation9, vscStation10,
                                                                   boost::none, 9.10, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineBothInMainComponent5 = dfl::inputs::HvdcLine::build("HVDCLineBothInMain5", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation23, vscStation8,
                                                                   boost::none, 2.8, false, 320, 322, 0.125, {0.01, 0.01});  // both in main cc
  auto hvdcLineVSCSwitch1 = dfl::inputs::HvdcLine::build("HVDCVSCLineSwitch1", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation11, dummyStationVSC,
                                                         boost::none, 11, false, 320, 322, 0.125, {0.01, 0.01});  // first in main cc
  auto hvdcLineVSCSwitch2 = dfl::inputs::HvdcLine::build("HVDCVSCLineSwitch2", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation12, dummyStationVSC,
                                                         boost::none, 12, false, 320, 322, 0.125, {0.01, 0.01});  // first in main cc
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
  dfl::inputs::DynamicDataBaseManager manager("", "res/assembling_test_hvdc.xml");
  dfl::algo::HVDCDefinitionAlgorithm algo(hvdcDefs, useReactiveLimits, set, busMap, manager);
  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto &node : nodes) {
    algo(node, algoRes);
  }

  auto &hvdcLines = hvdcDefs.hvdcLines;
  ASSERT_EQ(hvdcLines.size(), 11);
  ASSERT_EQ(hvdcLines.at("HVDCLCCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhi);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSetRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSetRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain4").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain5").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1);

  ASSERT_TRUE(hvdcDefs.vscBusVSCDefinitionsMap.empty());

  hvdcLines.clear();
  hvdcDefs.vscBusVSCDefinitionsMap.clear();
  // case diagrams
  useReactiveLimits = false;
  dfl::algo::HVDCDefinitionAlgorithm algo2(hvdcDefs, useReactiveLimits, set, busMap, manager);
  for (const auto &node : nodes) {
    algo2(node, algoRes);
  }

  ASSERT_EQ(hvdcLines.size(), 11);
  ASSERT_EQ(hvdcLines.at("HVDCLCCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLine3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain3").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain4").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCLineBothInMain5").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch1").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1);
  ASSERT_EQ(hvdcLines.at("HVDCVSCLineSwitch2").model, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1);

  ASSERT_TRUE(hvdcDefs.vscBusVSCDefinitionsMap.empty());
}
