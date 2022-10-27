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
 * @file  TestStaticVarCompensatorAlgo.cpp
 *
 * @brief StaticVarCompensatorAlgorithm library test file
 */

#include "SVarCDefinitionAlgorithm.h"
#include "Tests.h"

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
