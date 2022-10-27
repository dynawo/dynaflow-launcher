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
 * @file  TestLoadsAlgo.cpp
 *
 * @brief LoadDefinitionAlgorithm library test file
 */

#include "LoadDefinitionAlgorithm.h"
#include "Tests.h"

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
