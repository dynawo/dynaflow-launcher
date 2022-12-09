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
 * @file  TestContingencyAlgo.cpp
 *
 * @brief Contingency validation Algo library test file
 *
 */

#include "ContingencyValidationAlgorithm.h"
#include "Tests.h"

static void
addContingency(std::vector<dfl::inputs::Contingency>& contingencies, const std::string& id, const std::string elementId,
               dfl::inputs::ContingencyElement::Type elementType) {
  contingencies.emplace_back(id);
  contingencies[contingencies.size() - 1].elements.emplace_back(elementId, elementType);
}

TEST(ContingencyValidation, base) {
  using Type = dfl::inputs::ContingencyElement::Type;

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VL2");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{
      dfl::inputs::Node::build("0", vl, 110.0, {dfl::inputs::Shunt("SHUNT")}),
      dfl::inputs::Node::build("1", vl, 110.0, {}),
      dfl::inputs::Node::build("2", vl, 110.0, {}),
      dfl::inputs::Node::build("3", vl, 110.0, {}),
      dfl::inputs::Node::build("4", vl2, 220.0, {}),
      dfl::inputs::Node::build("5", vl2, 220.0, {}),
      dfl::inputs::Node::build("6", vl2, 220.0, {}),
      dfl::inputs::Node::build("7", vl2, 220.0, {}),
      dfl::inputs::Node::build("8", vl2, 220.0, {}),
  };

  dfl::algo::LoadDefinitionAlgorithm::Loads loads = {dfl::algo::LoadDefinition("LOAD", dfl::algo::LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "3"),
                                                     dfl::algo::LoadDefinition("LOADNETWORK", dfl::algo::LoadDefinition::ModelType::NETWORK, "3")};

  std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points(
      {dfl::inputs::Generator::ReactiveCurvePoint(12., 44., 440.), dfl::inputs::Generator::ReactiveCurvePoint(65., 44., 440.)});
  dfl::algo::GeneratorDefinitionAlgorithm::Generators generators = {
      dfl::algo::GeneratorDefinition("GENERATOR", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE, "4", points, 0, 0, 0, 0, 0, "4"),
      dfl::algo::GeneratorDefinition("GENERATORNETWORK", dfl::algo::GeneratorDefinition::ModelType::NETWORK, "4", points, 0, 0, 0, 0, 0, "4")};

  dfl::algo::StaticVarCompensatorAlgorithm::SVarCDefinitions svarcs = {
      dfl::algo::StaticVarCompensatorDefinition("SVARC", dfl::algo::StaticVarCompensatorDefinition::ModelType::SVARCPV, 0., 10., 230, 230, 215, 230, 235, 245,
                                                0., 10, 230.),
      dfl::algo::StaticVarCompensatorDefinition("SVARCNETWORK", dfl::algo::StaticVarCompensatorDefinition::ModelType::NETWORK, 0., 10., 230, 230, 215, 230, 235,
                                                245, 0., 10, 230.)};

  nodes[7]->danglingLines.emplace_back("DANGLINGLINE");
  nodes[8]->busBarSections.emplace_back("BUSBAR");

  std::vector<std::shared_ptr<dfl::inputs::Line>> lines{dfl::inputs::Line::build("LINE", nodes[0], nodes[1], "ETE", true, true)};
  std::vector<std::shared_ptr<dfl::inputs::Tfo>> tfos{dfl::inputs::Tfo::build("TFO", nodes[0], nodes[1], true, true),
                                                      dfl::inputs::Tfo::build("TFO3", nodes[0], nodes[1], nodes[2], true, true, true)};

  auto conv1 = std::make_shared<dfl::inputs::LCCConverter>("LccStation1", "0", nullptr, 1.);
  auto conv2 = std::make_shared<dfl::inputs::LCCConverter>("LccStation2", "1", nullptr, 1.);
  nodes[0]->converters.push_back(conv1);
  nodes[1]->converters.push_back(conv1);
  auto hvdc_line = dfl::inputs::HvdcLine::build("HVDC_LINE", dfl::inputs::HvdcLine::ConverterType::LCC, conv1, conv2, boost::none, 3.4, false, 320, 322, 0.125,
                                                {0.01, 0.01});
  conv1->hvdcLine = hvdc_line;
  conv2->hvdcLine = hvdc_line;

  auto contingencies = std::vector<dfl::inputs::Contingency>();

  addContingency(contingencies, "load", "LOAD", Type::LOAD);
  addContingency(contingencies, "loadnetwork", "LOADNETWORK", Type::LOAD);
  addContingency(contingencies, "load_bad_id", "XXX", Type::LOAD);
  addContingency(contingencies, "load_bad_type", "LOAD", Type::GENERATOR);
  addContingency(contingencies, "load_multiple_elements_one_bad", "LOAD", Type::LOAD);
  contingencies[contingencies.size() - 1].elements.emplace_back("XXX", Type::LINE);

  addContingency(contingencies, "generator", "GENERATOR", Type::GENERATOR);
  addContingency(contingencies, "generatornetwork", "GENERATORNETWORK", Type::GENERATOR);
  addContingency(contingencies, "generator_bad_id", "XXX", Type::GENERATOR);

  addContingency(contingencies, "shunt_compensator", "SHUNT", Type::SHUNT_COMPENSATOR);
  addContingency(contingencies, "shunt_compensator_bad_id", "XXX", Type::SHUNT_COMPENSATOR);

  addContingency(contingencies, "static_var_compensator", "SVARC", Type::STATIC_VAR_COMPENSATOR);
  addContingency(contingencies, "static_var_compensator_network", "SVARCNETWORK", Type::STATIC_VAR_COMPENSATOR);
  addContingency(contingencies, "static_var_compensator_bad_id", "XXX", Type::STATIC_VAR_COMPENSATOR);

  addContingency(contingencies, "dangling_line", "DANGLINGLINE", Type::DANGLING_LINE);
  addContingency(contingencies, "dangling_line_bad_id", "XXX", Type::DANGLING_LINE);

  addContingency(contingencies, "busbarsection", "BUSBAR", Type::BUSBAR_SECTION);
  addContingency(contingencies, "busbarsection_bad_id", "XXX", Type::BUSBAR_SECTION);

  addContingency(contingencies, "line", "LINE", Type::LINE);
  addContingency(contingencies, "line_bad_id", "XXX", Type::LINE);
  addContingency(contingencies, "line_branch", "LINE", Type::BRANCH);
  addContingency(contingencies, "line_branch_bad_id", "XXX", Type::BRANCH);

  addContingency(contingencies, "2wt", "TFO", Type::TWO_WINDINGS_TRANSFORMER);
  addContingency(contingencies, "2wt_bad_id", "XXX", Type::TWO_WINDINGS_TRANSFORMER);
  addContingency(contingencies, "2wt_branch", "TFO", Type::BRANCH);
  addContingency(contingencies, "2wt_branch_bad_id", "XXX", Type::BRANCH);

  addContingency(contingencies, "3wt", "TFO3", Type::THREE_WINDINGS_TRANSFORMER);
  addContingency(contingencies, "3wt_bad_id", "XXX", Type::THREE_WINDINGS_TRANSFORMER);

  addContingency(contingencies, "hvdcline", "HVDC_LINE", Type::HVDC_LINE);
  addContingency(contingencies, "hvdcline_bad_id", "XXX", Type::HVDC_LINE);

  auto validContingencies = dfl::algo::ValidContingencies(contingencies);
  auto algoOnInputs = dfl::algo::ContingencyValidationAlgorithmOnNodes(validContingencies);

  std::shared_ptr<dfl::algo::AlgorithmsResults> algoRes(new dfl::algo::AlgorithmsResults());
  for (const auto& node : nodes) {
    algoOnInputs(node, algoRes);
  }
  auto algoOnDefs = dfl::algo::ContingencyValidationAlgorithmOnDefs(validContingencies);
  algoOnDefs.fillValidContingenciesOnDefs(loads, generators, svarcs);

  validContingencies.keepContingenciesWithAllElementsValid();

  auto expectedValidContingencies = std::set<std::string>();
  expectedValidContingencies.emplace("load");
  expectedValidContingencies.emplace("loadnetwork");
  expectedValidContingencies.emplace("generator");
  expectedValidContingencies.emplace("generatornetwork");
  expectedValidContingencies.emplace("shunt_compensator");
  expectedValidContingencies.emplace("static_var_compensator");
  expectedValidContingencies.emplace("static_var_compensator_network");
  expectedValidContingencies.emplace("dangling_line");
  expectedValidContingencies.emplace("busbarsection");
  expectedValidContingencies.emplace("line");
  expectedValidContingencies.emplace("line_branch");
  expectedValidContingencies.emplace("2wt");
  expectedValidContingencies.emplace("2wt_branch");
  expectedValidContingencies.emplace("3wt");
  expectedValidContingencies.emplace("hvdcline");

  for (auto validContingency : validContingencies.get()) {
    auto expected = expectedValidContingencies.find(validContingency.get().id);
    ASSERT_TRUE(expected != expectedValidContingencies.end());
    expectedValidContingencies.erase(expected);
  }

  ASSERT_TRUE(expectedValidContingencies.empty());
  auto elementsNetworkType = validContingencies.getNetworkElements();
  ASSERT_EQ(elementsNetworkType.size(), 3);
  ASSERT_FALSE(elementsNetworkType.find("LOAD") != elementsNetworkType.end());
  ASSERT_TRUE(elementsNetworkType.find("LOADNETWORK") != elementsNetworkType.end());
  ASSERT_TRUE(elementsNetworkType.find("GENERATORNETWORK") != elementsNetworkType.end());
  ASSERT_TRUE(elementsNetworkType.find("SVARCNETWORK") != elementsNetworkType.end());
}
