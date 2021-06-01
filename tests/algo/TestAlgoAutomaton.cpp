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
 * @file  TestAlgoAutomaton.cpp
 *
 * @brief Automaton Algo library test file
 *
 */

#include "Algo.h"
#include "Tests.h"

#include <algorithm>
#include <vector>

// Required for testing unit tests
testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestAlgoAutomaton, base) {
  using dfl::algo::AutomatonDefinitions;
  using dfl::inputs::AutomatonConfigurationManager;

  AutomatonConfigurationManager manager("res/setting.xml", "res/assembling.xml");
  AutomatonDefinitions defs;

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("VL0", vl, 0.0, 0), dfl::inputs::Node::build("VL1", vl, 1.0, 1), dfl::inputs::Node::build("VL2", vl, 2.0, 2),
      dfl::inputs::Node::build("VL3", vl, 3.0, 3), dfl::inputs::Node::build("VL4", vl, 4.0, 4), dfl::inputs::Node::build("VL5", vl, 5.0, 5),
      dfl::inputs::Node::build("VL6", vl, 0.0, 0),
  };
  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{
      dfl::inputs::Line::build("0", nodes[0], nodes[1]), dfl::inputs::Line::build("1", nodes[0], nodes[2]), dfl::inputs::Line::build("2", nodes[0], nodes[3]),
      dfl::inputs::Line::build("3", nodes[3], nodes[4]), dfl::inputs::Line::build("4", nodes[2], nodes[4]), dfl::inputs::Line::build("5", nodes[1], nodes[4]),
      dfl::inputs::Line::build("6", nodes[5], nodes[6]),
  };

  auto tfo = dfl::inputs::Tfo::build("TFO1", nodes[2], nodes[3]);

  dfl::algo::AutomatonAlgorithm algo(defs, manager);

  std::for_each(nodes.begin(), nodes.end(), algo);

  ASSERT_EQ(defs.usedMacroConnections.size(), 8);
  std::set<std::string> usedMacroConnections(defs.usedMacroConnections.begin(), defs.usedMacroConnections.end());
  const std::vector<std::string> usedMacroConnectionsRef = {
      "VCSToUMeasurement",       "VCSToControlledShunts",      "CLAToIMeasurement", "CLAToControlledLineState",
      "CLAToAutomatonActivated", "PhaseShifterToIMeasurement", "PhaseShifterToTap", "PhaseShifterrToAutomatonActivated",
  };
  std::set<std::string> usedMacroConnectionsRefSet(usedMacroConnectionsRef.begin(), usedMacroConnectionsRef.end());
  ASSERT_EQ(usedMacroConnections, usedMacroConnectionsRefSet);

  ASSERT_EQ(defs.automatons.size(), 3);
  // bus
  ASSERT_NO_THROW(defs.automatons.at("MODELE_1_B.EPIP4"));
  const auto& automaton = defs.automatons.at("MODELE_1_B.EPIP4");
  ASSERT_EQ(automaton.id, "MODELE_1_B.EPIP4");
  ASSERT_EQ(automaton.lib, "libdummyLib");
  ASSERT_EQ(automaton.nodeConnections.size(), 6);

  std::string searched = "VCSToUMeasurement";
  auto found_connection = std::find_if(automaton.nodeConnections.begin(), automaton.nodeConnections.end(),
                                       [&searched](const dfl::algo::AutomatonDefinition::MacroConnection& connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, automaton.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "VL1");
  auto counter = std::count_if(automaton.nodeConnections.begin(), automaton.nodeConnections.end(),
                               [&searched](const dfl::algo::AutomatonDefinition::MacroConnection& connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 1);

  searched = "VCSToControlledShunts";
  counter = std::count_if(automaton.nodeConnections.begin(), automaton.nodeConnections.end(),
                          [&searched](const dfl::algo::AutomatonDefinition::MacroConnection& connection) { return connection.id == searched; });
  ASSERT_EQ(counter, 5);

  // Line
  ASSERT_NO_THROW(defs.automatons.at("DM_ADA_SALON"));
  const auto& automaton_ada = defs.automatons.at("DM_ADA_SALON");
  ASSERT_EQ(automaton_ada.id, "DM_ADA_SALON");
  ASSERT_EQ(automaton_ada.lib, "libdummyLib");
  ASSERT_EQ(automaton_ada.nodeConnections.size(), 3);

  searched = "CLAToControlledLineState";
  found_connection = std::find_if(automaton_ada.nodeConnections.begin(), automaton_ada.nodeConnections.end(),
                                  [&searched](const dfl::algo::AutomatonDefinition::MacroConnection& connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, automaton_ada.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "1");

  // TFO
  ASSERT_NO_THROW(defs.automatons.at("DM_BOUTR TR 661"));
  const auto& automaton_tfo = defs.automatons.at("DM_BOUTR TR 661");
  ASSERT_EQ(automaton_tfo.id, "DM_BOUTR TR 661");
  ASSERT_EQ(automaton_tfo.lib, "libdummyLib");
  ASSERT_EQ(automaton_tfo.nodeConnections.size(), 3);
  searched = "PhaseShifterToIMeasurement";
  found_connection = std::find_if(automaton_tfo.nodeConnections.begin(), automaton_tfo.nodeConnections.end(),
                                  [&searched](const dfl::algo::AutomatonDefinition::MacroConnection& connection) { return connection.id == searched; });
  ASSERT_NE(found_connection, automaton_ada.nodeConnections.end());
  ASSERT_EQ(found_connection->connectedElementId, "TFO1");
}
