//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "AssemblyXmlDocument.h"
#include "Tests.h"

#include <array>
#include <fstream>
#include <iostream>
#include <tuple>
#include <xml/sax/parser/Parser.h>
#include <xml/sax/parser/ParserException.h>
#include <xml/sax/parser/ParserFactory.h>

namespace parser = xml::sax::parser;

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(AssemblyXmlDocument, readFile) {
  using dfl::inputs::AssemblyXmlDocument;

  AssemblyXmlDocument doc;
  parser::ParserFactory factory;
  const std::string filepath = "res/assembling.xml";

  auto parser = factory.createParser();
  std::ifstream in(filepath.c_str());
  ASSERT_TRUE(in);
  ASSERT_NO_THROW(parser->parse(in, doc, false));

  const auto& macroConnections = doc.macroConnections();
  ASSERT_EQ(macroConnections.size(), 2);
  auto macro = macroConnections[0];
  ASSERT_EQ(macro.id, "VCSToUMeasurement");
  ASSERT_FALSE(macro.index1);
  ASSERT_EQ(macro.name2, true);
  ASSERT_EQ(macro.connections.size(), 1);
  ASSERT_EQ(macro.connections.front().var1, "U_IMPIN");
  ASSERT_EQ(macro.connections.front().var2, "@NAME@_U");
  ASSERT_EQ(macro.connections.front().required, true);
  ASSERT_EQ(macro.connections.front().internal, false);
  macro = macroConnections[1];
  ASSERT_EQ(macro.id, "VCSToControlledShunts");
  ASSERT_EQ(*macro.index1, true);
  ASSERT_EQ(macro.connections.size(), 3);
  std::array<std::tuple<std::string, std::string, bool, bool>, 3> connect_values = {
      std::make_tuple("shunt_state_@INDEX@", "@NAME@_state", true, false), std::make_tuple("shunt_isCapacitor_@INDEX@", "@NAME@_isCapacitor", true, true),
      std::make_tuple("shunt_isAvailable_@INDEX@", "@NAME@_isAvailable", true, false)};
  for (unsigned int i = 0; i < macro.connections.size(); ++i) {
    ASSERT_EQ(macro.connections[i].var1, std::get<0>(connect_values[i]));
    ASSERT_EQ(macro.connections[i].var2, std::get<1>(connect_values[i]));
    ASSERT_EQ(macro.connections[i].required, std::get<2>(connect_values[i]));
    ASSERT_EQ(macro.connections[i].internal, std::get<3>(connect_values[i]));
  }

  const auto& singleAssociations = doc.singleAssociations();
  ASSERT_EQ(singleAssociations.size(), 6);
  auto singleAssoc = singleAssociations.front();
  ASSERT_EQ(singleAssoc.id, "MESURE_MODELE_1_B.EPIP4");
  ASSERT_FALSE(singleAssoc.line);
  ASSERT_FALSE(singleAssoc.tfo);
  ASSERT_EQ(singleAssoc.buses.size(), 2);
  ASSERT_EQ(singleAssoc.busebars.size(), 2);
  ASSERT_EQ(singleAssoc.buses[0].name, "1");
  ASSERT_EQ(singleAssoc.buses[0].voltageLevel, "B.EPIP6");
  ASSERT_EQ(singleAssoc.buses[1].name, "2");
  ASSERT_EQ(singleAssoc.buses[1].voltageLevel, "B.EPIP6");
  ASSERT_EQ(singleAssoc.busebars[0].id, "B.EPIP6_1");
  ASSERT_EQ(singleAssoc.busebars[1].id, "B.EPIP6_2");
  singleAssoc = singleAssociations[2];
  ASSERT_EQ(singleAssoc.id, "MESURE_I_BOUTR TR 661");
  ASSERT_EQ(singleAssoc.busebars.size(), 0);
  ASSERT_EQ(singleAssoc.buses.size(), 0);
  ASSERT_FALSE(singleAssoc.line);
  ASSERT_TRUE(singleAssoc.tfo);
  ASSERT_EQ(singleAssoc.tfo->name, "BOUTR TR 661");
  singleAssoc = singleAssociations[4];
  ASSERT_EQ(singleAssoc.id, "MESURE_I_ADA_SALON");
  ASSERT_EQ(singleAssoc.busebars.size(), 0);
  ASSERT_EQ(singleAssoc.buses.size(), 0);
  ASSERT_FALSE(singleAssoc.tfo);
  ASSERT_TRUE(singleAssoc.line);
  ASSERT_EQ(singleAssoc.line->name, "RQROUL31S.BLA");

  const auto& multiAssociations = doc.multipleAssociations();
  ASSERT_EQ(multiAssociations.size(), 2);
  ASSERT_EQ(multiAssociations.front().id, "SHUNTS_MODELE_1_B.EPIP4");
  ASSERT_EQ(multiAssociations.front().shunt.voltageLevel, "B.EPIP4");

  const auto& dynamicAutomatons = doc.dynamicAutomatons();
  ASSERT_EQ(dynamicAutomatons.size(), 2);
  ASSERT_EQ(dynamicAutomatons.front().id, "MODELE_1_B.EPIP4");
  ASSERT_EQ(dynamicAutomatons.front().lib, "DYNModel1");
  ASSERT_EQ(dynamicAutomatons.front().type, "VoltageControlShunt");
  ASSERT_EQ(dynamicAutomatons.front().access, "T0|TFIN");
  const auto& macroConnects = dynamicAutomatons.front().macroConnects;
  ASSERT_EQ(macroConnects.size(), 2);
  ASSERT_EQ(macroConnects[0].id, "MESURE_MODELE_1_B.EPIP4");
  ASSERT_EQ(macroConnects[0].macroConnection, "VCSToUMeasurement");
  ASSERT_EQ(macroConnects[1].id, "SHUNTS_MODELE_1_B.EPIP4");
  ASSERT_EQ(macroConnects[1].macroConnection, "VCSToControlledShunts");
}
