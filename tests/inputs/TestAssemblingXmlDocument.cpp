//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "AssemblingDataBase.h"
#include "Log.h"
#include "Tests.h"

#include <DYNMultiProcessingContext.h>

#include <array>
#include <fstream>
#include <gtest_dynawo.h>
#include <iostream>
#include <tuple>
#include <xml/sax/parser/Parser.h>
#include <xml/sax/parser/ParserException.h>
#include <xml/sax/parser/ParserFactory.h>

namespace parser = xml::sax::parser;

testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::multiprocessing::Context mpiContext;

TEST(AssemblingXmlDocument, readFile) {
  const std::string filepath = "res/assembling.xml";
  dfl::inputs::AssemblingDataBase assembling(filepath);

  ASSERT_THROW_DYNAWO(assembling.getMultipleAssociation("dummy"), DYN::Error::GENERAL, dfl::KeyError_t::UnknownMultiAssoc);

  auto macro = assembling.getMacroConnection("ToUMeasurement");
  ASSERT_EQ(macro.id, "ToUMeasurement");
  ASSERT_TRUE(macro.indexId.empty());
  ASSERT_EQ(macro.connections.size(), 1);
  ASSERT_EQ(macro.connections.front().var1, "U_IMPIN");
  ASSERT_EQ(macro.connections.front().var2, "@NAME@_U");
  ASSERT_FALSE(macro.network);
  macro = assembling.getMacroConnection("ToUMeasurement", true);
  ASSERT_EQ(macro.id, "ToUMeasurement");
  ASSERT_EQ(macro.connections.size(), 1);
  ASSERT_EQ(macro.connections.front().var1, "Network_U_IMPIN");
  ASSERT_EQ(macro.connections.front().var2, "@NAME@_U");
  ASSERT_TRUE(macro.network);
  ASSERT_TRUE(assembling.hasNetworkMacroConnection("ToUMeasurement"));
  macro = assembling.getMacroConnection("ToControlledShunts");
  ASSERT_EQ(macro.id, "ToControlledShunts");
  ASSERT_FALSE(assembling.hasNetworkMacroConnection("ToControlledShunts"));
  ASSERT_EQ(macro.indexId, "MyIndexId");
  ASSERT_EQ(macro.connections.size(), 3);
  std::array<std::tuple<std::string, std::string>, 3> connect_values = {std::make_tuple("shunt_state_@INDEX@", "@NAME@_state"),
                                                                        std::make_tuple("shunt_isCapacitor_@INDEX@", "@NAME@_isCapacitor"),
                                                                        std::make_tuple("shunt_isAvailable_@INDEX@", "@NAME@_isAvailable")};
  for (unsigned int i = 0; i < macro.connections.size(); ++i) {
    ASSERT_EQ(macro.connections[i].var1, std::get<0>(connect_values[i]));
    ASSERT_EQ(macro.connections[i].var2, std::get<1>(connect_values[i]));
  }

  ASSERT_THROW_DYNAWO(assembling.getSingleAssociation("dummy"), DYN::Error::GENERAL, dfl::KeyError_t::UnknownSingleAssoc);
  ASSERT_NO_THROW(assembling.getSingleAssociation("MESURE_MODELE_1_VL4"));
  ASSERT_NO_THROW(assembling.getSingleAssociation("MESURE_MODELE_1_VL6"));
  ASSERT_NO_THROW(assembling.getSingleAssociation("MESURE_I_VL661"));
  ASSERT_NO_THROW(assembling.getSingleAssociation("TAP_VL661"));
  ASSERT_NO_THROW(assembling.getSingleAssociation("MESURE_I_SALON"));
  ASSERT_NO_THROW(assembling.getSingleAssociation("ORDER_SALON"));
  ASSERT_NO_THROW(assembling.getSingleAssociation("HVDC_LINE"));
  auto singleAssoc = assembling.getSingleAssociation("MESURE_MODELE_1_VL4");
  ASSERT_EQ(singleAssoc.id, "MESURE_MODELE_1_VL4");
  ASSERT_FALSE(singleAssoc.line);
  ASSERT_FALSE(singleAssoc.hvdcLine);
  ASSERT_FALSE(singleAssoc.tfo);
  ASSERT_FALSE(singleAssoc.shunt);
  ASSERT_TRUE(singleAssoc.bus);
  ASSERT_EQ(singleAssoc.bus->voltageLevel, "VLP6");
  singleAssoc = assembling.getSingleAssociation("MESURE_I_VL661");
  ASSERT_EQ(singleAssoc.id, "MESURE_I_VL661");
  ASSERT_FALSE(singleAssoc.bus);
  ASSERT_FALSE(singleAssoc.hvdcLine);
  ASSERT_FALSE(singleAssoc.line);
  ASSERT_FALSE(singleAssoc.shunt);
  ASSERT_TRUE(singleAssoc.tfo);
  ASSERT_EQ(singleAssoc.tfo->name, "VL661");
  singleAssoc = assembling.getSingleAssociation("MESURE_I_SALON");
  ASSERT_EQ(singleAssoc.id, "MESURE_I_SALON");
  ASSERT_FALSE(singleAssoc.bus);
  ASSERT_FALSE(singleAssoc.hvdcLine);
  ASSERT_FALSE(singleAssoc.tfo);
  ASSERT_FALSE(singleAssoc.shunt);
  ASSERT_TRUE(singleAssoc.line);
  ASSERT_EQ(singleAssoc.line->name, "QBLA");
  singleAssoc = assembling.getSingleAssociation("SHUNT_MODELE_VL6");
  ASSERT_EQ(singleAssoc.id, "SHUNT_MODELE_VL6");
  ASSERT_FALSE(singleAssoc.bus);
  ASSERT_FALSE(singleAssoc.hvdcLine);
  ASSERT_FALSE(singleAssoc.tfo);
  ASSERT_FALSE(singleAssoc.line);
  ASSERT_TRUE(singleAssoc.shunt);
  ASSERT_EQ(singleAssoc.shunt->name, "VL6");
  singleAssoc = assembling.getSingleAssociation("HVDC_LINE");
  ASSERT_EQ(singleAssoc.id, "HVDC_LINE");
  ASSERT_FALSE(singleAssoc.bus);
  ASSERT_TRUE(singleAssoc.hvdcLine);
  ASSERT_FALSE(singleAssoc.tfo);
  ASSERT_FALSE(singleAssoc.line);
  ASSERT_FALSE(singleAssoc.shunt);
  ASSERT_EQ(singleAssoc.hvdcLine->name, "MyHvdc");

  ASSERT_THROW_DYNAWO(assembling.getMultipleAssociation("dummy"), DYN::Error::GENERAL, dfl::KeyError_t::UnknownMultiAssoc);
  ASSERT_NO_THROW(assembling.getMultipleAssociation("SHUNTS_MODELE_1_VL4"));
  ASSERT_NO_THROW(assembling.getMultipleAssociation("SHUNTS_MODELE_1_VL6"));
  auto multipleAssoc = assembling.getMultipleAssociation("SHUNTS_MODELE_1_VL4");
  ASSERT_EQ(multipleAssoc.id, "SHUNTS_MODELE_1_VL4");
  ASSERT_TRUE(multipleAssoc.shunt);
  ASSERT_EQ(multipleAssoc.shunt->voltageLevel, "VL4");

  singleAssoc = assembling.getSingleAssociation("GeneratorId");
  ASSERT_EQ(singleAssoc.id, "GeneratorId");
  ASSERT_EQ(singleAssoc.generators.size(), 2);
  ASSERT_EQ(singleAssoc.generators[0].name, "GeneratorId1");
  ASSERT_EQ(singleAssoc.generators[1].name, "GeneratorId_1");
  singleAssoc = assembling.getSingleAssociation("GeneratorId2");
  ASSERT_EQ(singleAssoc.id, "GeneratorId2");
  ASSERT_EQ(singleAssoc.generators.size(), 2);
  ASSERT_EQ(singleAssoc.generators[0].name, "GeneratorId2");
  ASSERT_EQ(singleAssoc.generators[1].name, "GeneratorId_2");
  singleAssoc = assembling.getSingleAssociation("LoadId");
  ASSERT_EQ(singleAssoc.id, "LoadId");
  ASSERT_EQ(singleAssoc.loads.size(), 2);
  ASSERT_EQ(singleAssoc.loads[0].name, "LoadId");
  ASSERT_EQ(singleAssoc.loads[1].name, "LoadId_0");

  const auto &dynamicAutomatons = assembling.dynamicAutomatons();
  ASSERT_EQ(dynamicAutomatons.size(), 4);
  ASSERT_EQ(dynamicAutomatons.find("MODELE_1_VL4")->second.id, "MODELE_1_VL4");
  ASSERT_EQ(dynamicAutomatons.find("MODELE_1_VL4")->second.lib, "DYNModel1");
  const auto &macroConnects = dynamicAutomatons.find("MODELE_1_VL4")->second.macroConnects;
  ASSERT_EQ(macroConnects.size(), 3);
  ASSERT_EQ(macroConnects[0].id, "MESURE_MODELE_1_VL4");
  ASSERT_EQ(macroConnects[0].macroConnection, "ToUMeasurement");
  ASSERT_EQ(macroConnects[0].mandatory, false);
  ASSERT_EQ(macroConnects[1].id, "SHUNTS_MODELE_1_VL4");
  ASSERT_EQ(macroConnects[1].macroConnection, "ToControlledShunts");
  ASSERT_EQ(macroConnects[1].mandatory, true);
  ASSERT_EQ(macroConnects[2].id, "GEN1");
  ASSERT_EQ(macroConnects[2].macroConnection, "SVCToGenerator");
  ASSERT_EQ(macroConnects[2].mandatory, false);
  // Check model model connections
  ASSERT_EQ(dynamicAutomatons.find("VIRTUAL_MODEL")->second.id, "VIRTUAL_MODEL");
  ASSERT_EQ(dynamicAutomatons.find("VIRTUAL_MODEL")->second.lib, "DYNModelVirtual");
  const auto &macroConnectsModelModel = dynamicAutomatons.find("VIRTUAL_MODEL")->second.macroConnects;
  ASSERT_EQ(macroConnectsModelModel.size(), 1);
  ASSERT_EQ(macroConnectsModelModel[0].id, "MODELE_1_VL6");
  ASSERT_EQ(macroConnectsModelModel[0].macroConnection, "ModelModelConnection");

  ASSERT_FALSE(assembling.containsSVC());

  ASSERT_THROW_DYNAWO(assembling.getProperty("dummy"), DYN::Error::GENERAL, dfl::KeyError_t::UnknownProperty);
  ASSERT_TRUE(assembling.isProperty("MyProp"));
  const auto &prop = assembling.getProperty("MyProp");
  ASSERT_EQ(prop.devices.size(), 2);
  ASSERT_EQ(prop.devices[0].id, "MyDevice");
  ASSERT_EQ(prop.devices[1].id, "MyDevice2");
}

TEST(AssemblingXmlDocument, AssemblingContainsSVC) {
  const std::string filepath = "res/assembling_svc.xml";
  dfl::inputs::AssemblingDataBase assembling(filepath);
  ASSERT_TRUE(assembling.containsSVC());
}
