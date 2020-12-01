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
  return lhs.id == rhs.id && lhs.converterType == rhs.converterType && lhs.converter1_id == rhs.converter1_id && lhs.converter1_busId == rhs.converter1_busId &&
         lhs.converter2_voltageRegulationOn == rhs.converter2_voltageRegulationOn && lhs.converter2_id == rhs.converter2_id &&
         lhs.converter2_busId == rhs.converter2_busId && lhs.converter2_voltageRegulationOn == rhs.converter2_voltageRegulationOn;
}

TEST(NetworkManager, hvdcLines) {
  using dfl::inputs::NetworkManager;
  std::vector<dfl::inputs::HvdcLine> expected_hvdcLines = {
      dfl::inputs::HvdcLine("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___10_TN", boost::optional<bool>(), "LCCStation2",
                            "_BUS___11_TN", boost::optional<bool>()),
      dfl::inputs::HvdcLine("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", false, "VSCStation1", "_BUS___10_TN",
                            true)};

  NetworkManager manager("res/HvdcDangling.iidm");
  const auto& hvdcLines = manager.getHvdcLine();
  for (int index = 0; index < 2; ++index) {
    ASSERT_TRUE(hvdcLineEqual(*hvdcLines[index], expected_hvdcLines[index]));
  }
}
