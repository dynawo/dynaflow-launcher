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
 * @file  TestShuntCounterAlgo.cpp
 *
 * @brief ShuntCounterDefinitions library test file
 */

#include "ShuntDefinitionAlgorithm.h"
#include "Tests.h"

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
