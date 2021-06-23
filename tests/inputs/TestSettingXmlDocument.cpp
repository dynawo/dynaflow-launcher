//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "SettingXmlDocument.h"
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

TEST(SettingXmlDocument, readFile) {
  using dfl::inputs::SettingXmlDocument;

  SettingXmlDocument doc;
  parser::ParserFactory factory;
  const std::string filepath = "res/setting.xml";

  auto parser = factory.createParser();
  std::ifstream in(filepath.c_str());
  ASSERT_TRUE(in);
  ASSERT_NO_THROW(parser->parse(in, doc, false));

  const auto& sets = doc.sets();
  ASSERT_EQ(sets.size(), 14);

  auto set = sets.front();
  ASSERT_EQ(set.id, "MODELE_1_VL4");
  ASSERT_EQ(set.counts.size(), 1);
  auto count = set.counts.front();
  ASSERT_EQ(count.id, "SHUNTS_MODELE_1_VL4");
  ASSERT_EQ(count.name, "nbShunts");
  ASSERT_EQ(set.boolParameters.size(), 0);
  ASSERT_EQ(set.integerParameters.size(), 0);
  ASSERT_EQ(set.doubleParameters.size(), 3);
  auto param = set.doubleParameters[0];
  ASSERT_EQ(param.name, "uMin");
  ASSERT_EQ(param.value, 236.0);
  param = set.doubleParameters[1];
  ASSERT_EQ(param.name, "uMax");
  ASSERT_EQ(param.value, 243.5);
  param = set.doubleParameters[2];
  ASSERT_EQ(param.name, "delay");
  ASSERT_EQ(param.value, 10.0);
  ASSERT_EQ(set.references.size(), 0);
  ASSERT_EQ(set.refs.size(), 0);

  set = sets[5];
  ASSERT_EQ(set.id, "MODELE_2_TZE");
  ASSERT_EQ(set.references.size(), 0);
  ASSERT_EQ(set.refs.size(), 0);
  ASSERT_EQ(set.counts.size(), 2);
  count = set.counts[0];
  ASSERT_EQ(count.id, "SHUNTS_HAUTS_MODELE_2_TZE");
  ASSERT_EQ(count.name, "nbShuntsHV");
  count = set.counts[1];
  ASSERT_EQ(count.id, "SHUNTS_BAS_MODELE_2_TZE");
  ASSERT_EQ(count.name, "nbShuntsLV");
  ASSERT_EQ(set.boolParameters.size(), 0);
  ASSERT_EQ(set.integerParameters.size(), 1);
  auto param_int = set.integerParameters.front();
  ASSERT_EQ(param_int.name, "modeRegulation");
  ASSERT_EQ(param_int.value, 1);
  ASSERT_EQ(set.doubleParameters.size(), 16);
  std::array<std::pair<std::string, double>, 16> values = {std::make_pair("closingSecurityThresholdHV", 228.0),
                                                           std::make_pair("closingRegulationThresholdHV", 230.0),
                                                           std::make_pair("noClosingThresholdHV", 235.0),
                                                           std::make_pair("openingRegulationThresholdHV", 245.0),
                                                           std::make_pair("mutualOpeningThresholdHV", 246.0),
                                                           std::make_pair("openingSecurityThresholdHV", 247.0),
                                                           std::make_pair("closingDelayHV", 5.0),
                                                           std::make_pair("openingDelayHV", 10.0),
                                                           std::make_pair("closingSecurityThresholdLV", 228.0),
                                                           std::make_pair("closingRegulationThresholdLV", 230.0),
                                                           std::make_pair("noClosingThresholdLV", 235.0),
                                                           std::make_pair("openingRegulationThresholdLV", 245.0),
                                                           std::make_pair("mutualOpeningThresholdLV", 246.0),
                                                           std::make_pair("openingSecurityThresholdLV", 247.0),
                                                           std::make_pair("closingDelayLV", 10.0),
                                                           std::make_pair("openingDelayLV", 8.0)};
  for (unsigned int i = 0; i < values.size(); ++i) {
    ASSERT_EQ(set.doubleParameters[i].name, values[i].first);
    ASSERT_EQ(set.doubleParameters[i].value, values[i].second);
  }

  set = sets[6];
  ASSERT_EQ(set.id, "DM_M661");
  ASSERT_EQ(set.counts.size(), 0);
  ASSERT_EQ(set.refs.size(), 0);
  ASSERT_EQ(set.boolParameters.size(), 0);
  ASSERT_EQ(set.integerParameters.size(), 0);
  ASSERT_EQ(set.doubleParameters.size(), 3);
  std::array<std::pair<std::string, double>, 3> values6 = {
      std::make_pair("phaseShifter_sign", 1),
      std::make_pair("phaseShifter_t1st", 2.7),
      std::make_pair("phaseShifter_tNext", 2.7),
  };
  for (unsigned int i = 0; i < values6.size(); ++i) {
    ASSERT_EQ(set.doubleParameters[i].name, values6[i].first);
    ASSERT_EQ(set.doubleParameters[i].value, values6[i].second);
  }
  ASSERT_EQ(set.references.size(), 8);
  std::array<std::tuple<std::string, std::string, SettingXmlDocument::Reference::DataType>, 8> values8 = {
      std::make_tuple("phaseShifter_I0", "i1", SettingXmlDocument::Reference::DataType::DOUBLE),
      std::make_tuple("phaseShifter_tap0", "tap0", SettingXmlDocument::Reference::DataType::INT),
      std::make_tuple("phaseShifter_tapMin", "tapMin", SettingXmlDocument::Reference::DataType::INT),
      std::make_tuple("phaseShifter_tapMax", "tapMax", SettingXmlDocument::Reference::DataType::INT),
      std::make_tuple("phaseShifter_regulating0", "regulating", SettingXmlDocument::Reference::DataType::DOUBLE),
      std::make_tuple("phaseShifter_iMax", "iMax", SettingXmlDocument::Reference::DataType::DOUBLE),
      std::make_tuple("phaseShifter_iStop", "iStop", SettingXmlDocument::Reference::DataType::DOUBLE),
      std::make_tuple("phaseShifter_increasePhase", "increasePhase", SettingXmlDocument::Reference::DataType::INT)};
  for (unsigned int i = 0; i < values8.size(); ++i) {
    ASSERT_TRUE(set.references[i].componentId.has_value());
    ASSERT_EQ(set.references[i].componentId.get(), "@TFO@");
    ASSERT_EQ(set.references[i].name, std::get<0>(values8[i]));
    ASSERT_EQ(set.references[i].origName, std::get<1>(values8[i]));
    ASSERT_EQ(set.references[i].dataType, std::get<2>(values8[i]));
  }

  set = sets[9];
  ASSERT_EQ(set.id, "DM_TAILLE");
  ASSERT_EQ(set.counts.size(), 0);
  ASSERT_EQ(set.references.size(), 0);
  ASSERT_EQ(set.refs.size(), 1);
  const auto& ref = set.refs.front();
  ASSERT_EQ(ref.id, "MESURE_I_TAILLE");
  ASSERT_EQ(ref.name, "currentLimitAutomaton_Season");
  ASSERT_EQ(ref.tag, "@SAISON@");
  ASSERT_EQ(set.doubleParameters.size(), 6);
  std::array<std::pair<std::string, double>, 6> values11_d = {
      std::make_pair("currentLimitAutomaton_IThresholdSummer", 590),        std::make_pair("currentLimitAutomaton_IThresholdWinter1", 689),
      std::make_pair("currentLimitAutomaton_IThresholdWinter2", 759),       std::make_pair("currentLimitAutomaton_IThresholdIntermediate1", 637),
      std::make_pair("currentLimitAutomaton_IThresholdIntermediate2", 637), std::make_pair("currentLimitAutomaton_tLagBeforeActing", 50)};
  for (unsigned int i = 0; i < values11_d.size(); ++i) {
    ASSERT_EQ(set.doubleParameters[i].name, values11_d[i].first);
    ASSERT_EQ(set.doubleParameters[i].value, values11_d[i].second);
  }
  ASSERT_EQ(set.integerParameters.size(), 1);
  ASSERT_EQ(set.integerParameters.front().name, "currentLimitAutomaton_OrderToEmit");
  ASSERT_EQ(set.integerParameters.front().value, 3);
  ASSERT_EQ(set.boolParameters.size(), 1);
  ASSERT_EQ(set.boolParameters.front().name, "currentLimitAutomaton_Running");
  ASSERT_EQ(set.boolParameters.front().value, true);
}

TEST(SettingXmlDocument, error) {
  using dfl::inputs::SettingXmlDocument;

  SettingXmlDocument doc;
  parser::ParserFactory factory;
  const std::string filepath = "res/setting_error.xml";

  auto parser = factory.createParser();
  std::ifstream in(filepath.c_str());
  ASSERT_TRUE(in);
  ASSERT_ANY_THROW(parser->parse(in, doc, false));
}
