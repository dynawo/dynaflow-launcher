//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include <memory>

#include <DYNMultiProcessingContext.h>

#include "Constants.h"
#include "DynModelFilterAlgorithm.h"
#include "Tests.h"

testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::multiprocessing::Context mpiContext;

using dfl::algo::DynamicModelDefinition;

TEST(DynModelFilter, RemoveRPCLIfMissingConnexionToSvc) {
  enum class TestConfig { ConnexionToSvc, MissingConnexionToSvc };
  const std::vector<TestConfig> testConfigs = {TestConfig::ConnexionToSvc, TestConfig::MissingConnexionToSvc};

  for (TestConfig testConfig : testConfigs) {
    // Definition of a dynamic model and its nodeConnections
    const std::string dynModelId = "SVC";
    DynamicModelDefinition dynModel(dynModelId, dfl::common::constants::svcModelName);
    std::vector<dfl::algo::GeneratorDefinition> generators;
    dfl::algo::HVDCLineDefinitions hvdcLineDefinitions;
    std::array<std::pair<std::string, dfl::algo::GeneratorDefinition::ModelType>, 12> gensArray = {
        std::make_pair("_GEN____1_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE),
        std::make_pair("_GEN____2_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL_RECTANGULAR),
        std::make_pair("_GEN____3_SM", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL_SIGNALN),
        std::make_pair("_GEN____4_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_INFINITE),
        std::make_pair("_GEN____5_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_RECTANGULAR),
        std::make_pair("_GEN____6_SM", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL_SIGNALN),
        std::make_pair("_GEN____7_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_INFINITE),
        std::make_pair("_GEN____8_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_RPCL2_RECTANGULAR),
        std::make_pair("_GEN____9_SM", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL2_SIGNALN),
        std::make_pair("_GEN____10_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_INFINITE),
        std::make_pair("_GEN____11_SM", dfl::algo::GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR),
        std::make_pair("_GEN____12_SM", dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL2_SIGNALN)};
    std::vector<dfl::inputs::Generator::ReactiveCurvePoint> points = {dfl::inputs::Generator::ReactiveCurvePoint(1, 11, 111),
                                                                      dfl::inputs::Generator::ReactiveCurvePoint(2, 22, 222)};
    for (const auto &gen : gensArray) {
      generators.push_back(dfl::algo::GeneratorDefinition(gen.first, gen.second, "0", points, 0, 0, 0, 0, 0, 0, "4"));
      DynamicModelDefinition::MacroConnection genMacroConnection =
          DynamicModelDefinition::MacroConnection("SVCToGenerator", DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, gen.first, "");
      dynModel.nodeConnections.insert(genMacroConnection);
    }

    std::unique_ptr<DynamicModelDefinition::MacroConnection> busMacroConnection;
    switch (testConfig) {
    case TestConfig::ConnexionToSvc:
      busMacroConnection = std::unique_ptr<DynamicModelDefinition::MacroConnection>(
          new DynamicModelDefinition::MacroConnection("SVCToUMeasurement", DynamicModelDefinition::MacroConnection::ElementType::NODE, "_BUS____1_TN", ""));
      break;
    case TestConfig::MissingConnexionToSvc:
      busMacroConnection = std::unique_ptr<DynamicModelDefinition::MacroConnection>(new DynamicModelDefinition::MacroConnection(
          "SVCToUMeasurementNotFound", DynamicModelDefinition::MacroConnection::ElementType::NODE, "_BUS____1_TN", ""));
      break;
    }
    dynModel.nodeConnections.insert(*busMacroConnection);

    const std::string dynModelId2 = "PhaseShifter_5_6";
    DynamicModelDefinition dynModel2(dynModelId2, "PhaseShifterI");
    DynamicModelDefinition::MacroConnection busMacroConnection2("PhaseShifterToP",
                                                                DynamicModelDefinition::MacroConnection::ElementType::TFO,
                                                                "_BUS____5-BUS____6-1_PS",
                                                                "");
    dynModel2.nodeConnections.insert(busMacroConnection2);
    DynamicModelDefinition::MacroConnection busMacroConnection3("PhaseShifterToIMeasurement",
                                                                DynamicModelDefinition::MacroConnection::ElementType::TFO,
                                                                "_BUS____5-BUS____6-1_PS",
                                                                "");
    dynModel2.nodeConnections.insert(busMacroConnection3);

    std::map<DynamicModelDefinition::DynModelId, DynamicModelDefinition> models;
    models.insert({dynModelId, dynModel});
    models.insert({dynModelId2, dynModel2});
    dfl::inputs::AssemblingDataBase assembling("res/assembling_svc.xml");
    dfl::algo::DynModelFilterAlgorithm dynModelFilterAlgorithm(assembling, generators, hvdcLineDefinitions, models);
    dynModelFilterAlgorithm.filter();
    auto modelsIt = models.find(dynModelId);
    switch (testConfig) {
    case TestConfig::ConnexionToSvc:
      ASSERT_NE(modelsIt, models.end());
      ASSERT_EQ(models.size(), 1);
      for (dfl::algo::GeneratorDefinition gen : generators)
        ASSERT_TRUE(gen.hasRpcl());
      break;
    case TestConfig::MissingConnexionToSvc:
      ASSERT_EQ(modelsIt, models.end());
      ASSERT_EQ(models.size(), 0);
      for (dfl::algo::GeneratorDefinition gen : generators)
        ASSERT_FALSE(gen.hasRpcl());
      break;
    }
    auto modelsIt2 = models.find(dynModelId2);
    ASSERT_EQ(modelsIt2, models.end());
  }
}
