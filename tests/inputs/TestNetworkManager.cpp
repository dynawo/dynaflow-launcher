//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "NetworkManager.h"
#include "Tests.h"

static size_t count = 0;

static void
checkNode(const std::shared_ptr<dfl::inputs::Node>& node) {
  // Pattern = _BUS_+[0-9]+_TN
  ASSERT_EQ(0, node->id.compare(0, 5, "_BUS_"));

  size_t index = node->id.find_first_not_of('_', 5);
  ASSERT_NE(index, std::string::npos);
  size_t index2 = node->id.find_first_of('_', index + 1);
  ASSERT_TRUE(index2 == node->id.length() - 3);
  for (size_t i = index + 1; i < index2; i++) {
    ASSERT_TRUE(std::isdigit(node->id.at(i)));
  }

  ASSERT_EQ(0, node->id.compare(node->id.length() - 3, 3, "_TN"));
  ++count;

  // 1 VL <=> 1 node in this example
  ASSERT_FALSE(node->voltageLevel.expired());
  auto vl = node->voltageLevel.lock();
  ASSERT_EQ(1, vl->nodes.size());
  ASSERT_EQ(node->id, vl->nodes.front()->id);
}

TEST(NetworkManager, walk) {
  using dfl::inputs::NetworkManager;

  NetworkManager manager("res/IEEE14.iidm");

  manager.onNode(&checkNode);

  count = 0;
  manager.walkNodes();
  ASSERT_EQ(14, count);
}

static bool
hvdcLineEqual(const dfl::inputs::HvdcLine& lhs, const dfl::inputs::HvdcLine& rhs) {
  return lhs.id == rhs.id && lhs.converterType == rhs.converterType && lhs.pMax == rhs.pMax;
}

TEST(NetworkManager, hvdcLines) {
  using dfl::inputs::NetworkManager;
  auto dummyStation = std::make_shared<dfl::inputs::LCCConverter>("StationN", "_BUS___99_TN", nullptr, 99.);
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "_BUS___99_TN", nullptr, false, 0., 0.,
                                                                     std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  std::vector<std::shared_ptr<dfl::inputs::HvdcLine>> expected_hvdcLines = {
      dfl::inputs::HvdcLine::build("HVDCLCCLine",
                                    dfl::inputs::HvdcLine::ConverterType::LCC,
                                    dummyStation,
                                    dummyStation,
                                    boost::none,
                                    2000,
                                    false,
                                    320,
                                    322,
                                    0.125,
                                    0.01,
                                    0.01),
      dfl::inputs::HvdcLine::build("HVDCVSCLine",
                                    dfl::inputs::HvdcLine::ConverterType::VSC,
                                    dummyStationVSC,
                                    dummyStationVSC,
                                    boost::none,
                                    2000,
                                    false,
                                    320,
                                    322,
                                    0.125,
                                    0.01,
                                    0.01)};
  NetworkManager manager("res/HvdcDangling.iidm");
  const auto& hvdcLines = manager.getHvdcLine();
  for (int index = 0; index < 2; ++index) {
    ASSERT_TRUE(hvdcLineEqual(*hvdcLines[index], *expected_hvdcLines[index]));
  }

  auto converters = manager.computeVSCConverters();
  ASSERT_EQ(converters.size(), 2);
  std::vector<dfl::inputs::Converter::ConverterId> ids;
  std::transform(converters.begin(), converters.end(), std::back_inserter(ids),
                 [](const std::shared_ptr<dfl::inputs::Converter>& converter) { return converter->converterId; });
  std::sort(ids.begin(), ids.end());
  ASSERT_EQ(ids.at(0), "VSCStation1");
  ASSERT_EQ(ids.at(1), "VSCStation2");
}

TEST(NetworkManager, generators) {
  using dfl::inputs::NetworkManager;
  NetworkManager manager("res/Generators.iidm");
  NetworkManager::BusMapRegulating expected_busMap = {{"_BUS____1_TN", NetworkManager::NbOfRegulating::ONE},
                                                      {"_BUS____5_TN", NetworkManager::NbOfRegulating::MULTIPLES},
                                                      {"_BUS___10_TN", NetworkManager::NbOfRegulating::ONE}};
  auto busMapId = manager.getMapBusGeneratorsBusId();
  for (auto it_expected : expected_busMap) {
    auto it_map = busMapId.find(it_expected.first);
    ASSERT_FALSE(it_map == busMapId.end());
    ASSERT_EQ(it_expected.second, it_map->second);
  }
}

TEST(NetworkManager, shunts) {
  using dfl::inputs::NetworkManager;
  NetworkManager manager("res/IEEE14.iidm");

  unsigned int nbShunts = 0;
  manager.onNode([&nbShunts](const std::shared_ptr<dfl::inputs::Node>& node) { nbShunts += node->shunts.size(); });
  manager.walkNodes();
  ASSERT_EQ(nbShunts, 1);
}
