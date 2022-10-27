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
 * @file  TestLinesAlgo.cpp
 *
 * @brief LinesByIdAlgorithm library test file
 */

#include "LineDefinitionAlgorithm.h"
#include "Tests.h"

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
