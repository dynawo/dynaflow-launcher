//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Par.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestPar, write) {
  using dfl::algo::GeneratorDefinition;
  using dfl::algo::LoadDefinition;

  dfl::inputs::DynamicDataBaseManager manager("", "");

  std::string basename = "TestPar";
  std::string dirname = "results";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN, "00", {}, 1., 10., 11., 110., 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "04", {}, 3., 30., -33., 330., 0, bus1)};

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(
      dfl::outputs::Par::ParDefinition(basename, dirname, outputPath.generic_string(), generators, {}, activePowerCompensation, {}, manager, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeRemote) {
  using dfl::algo::GeneratorDefinition;
  using dfl::algo::LoadDefinition;

  dfl::inputs::DynamicDataBaseManager manager("", "");

  std::string basename = "TestParRemote";
  std::string dirname = "results";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::string bus1 = "BUS_1";
  std::string bus2 = "BUS_2";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::REMOTE_SIGNALN, "00", {}, 1., 10., 11., 110., 100, bus1),
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::PROP_SIGNALN, "01", {}, 2., 20., 22., 220., 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 100, bus1),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "03", {}, 4., 40., 44., 440., 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::PROP_SIGNALN, "01", {}, 2., 20., 22., 220., 100, bus2),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::PROP_SIGNALN, "01", {}, 2., 20., 22., 220., 100, bus2)};

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::algo::GeneratorDefinitionAlgorithm::BusGenMap busesWithDynamicModel = {{bus1, "G1"}, {bus2, "G4"}};
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, dirname, outputPath.generic_string(), generators, {}, activePowerCompensation,
                                                               busesWithDynamicModel, manager, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeHdvc) {
  using dfl::algo::HvdcLineDefinition;
  std::string basename = "TestParHvdc";
  std::string dirname = "results";
  std::string filename = basename + ".par";

  dfl::inputs::DynamicDataBaseManager manager("", "");

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  auto hvdcLineLCC = HvdcLineDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", boost::optional<bool>(),
                                        "LCCStation2", "_BUS___10_TN", boost::optional<bool>(), HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT);
  auto hvdcLineVSC = HvdcLineDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2",
                                        "_BUS___11_TN", false, HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT);
  dfl::algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap hvdcLines = {std::make_pair(hvdcLineVSC.id, hvdcLineVSC),
                                                                              std::make_pair(hvdcLineLCC.id, hvdcLineLCC)};

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(
      dfl::outputs::Par::ParDefinition(basename, dirname, outputPath.generic_string(), {}, hvdcLines, activePowerCompensation, {}, manager, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, DynModel) {
  using dfl::algo::GeneratorDefinition;
  using dfl::algo::LoadDefinition;

  dfl::inputs::DynamicDataBaseManager manager("res/setting.xml", "res/assembling.xml");

  std::string basename = "TestParDynModel";
  std::string dirname = "results";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  dfl::algo::ShuntCounterDefinitions counters;
  counters.nbShunts["VL"] = 2;
  counters.nbShunts["VL2"] = 1;

  dfl::algo::DynamicModelDefinitions defs;
  dfl::algo::DynamicModelDefinition dynModel("DM_VL61", "DummyLib");
  dynModel.nodeConnections.insert(
      dfl::algo::DynamicModelDefinition::MacroConnection("MacroTest", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO, "TfoId"));
  defs.models.insert({dynModel.id, dynModel});

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN, "00", {}, 1., 10., 11., 110., 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "04", {}, 3., 30., -33., 330., 0, bus1)};

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, dirname, outputPath.generic_string(), generators, {}, activePowerCompensation, {},
                                                               manager, counters, defs, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
