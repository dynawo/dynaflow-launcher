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
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN, "00", {}, 1., 10., 11., 110., 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "04", {}, 3., 30., -33., 330., 0, bus1),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::NETWORK, "04", {}, 3., 30., -33., 330., 0, bus1)};

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, outputPathResults, outputPath.generic_string(), generators,
                                                              {}, activePowerCompensation, {}, manager, {}, {}, {}, {}, {}));

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
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
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
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, outputPathResults, outputPath.generic_string(), generators,
                                                              {}, activePowerCompensation, busesWithDynamicModel, manager, {}, {}, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeHdvc) {
  using dfl::algo::HVDCDefinition;
  std::string basename = "TestParHvdc";
  std::string filename = basename + ".par";

  dfl::inputs::DynamicDataBaseManager manager("", "");

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  dfl::algo::VSCDefinition vscStation1("VSCStation1", 1., -1., 0., {});
  dfl::algo::VSCDefinition vscStation2("VSCStation2", 2., -2., 0., {});
  dfl::algo::VSCDefinition vscStation3("VSCStation3", 3., -3., 0., {});

  auto hvdcLineLCC = HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___21_TN", false, "LCCStation2",
                                    "_BUS___22_TN", false, HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT,
                                    dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., boost::none, boost::none, boost::none);
  auto hvdcLineVSC = HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2",
                                    "_BUS___11_TN", false, HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT,
                                    dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., vscStation1, vscStation2, boost::none);
  auto hvdcLineVSC2 = HVDCDefinition("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2",
                                     "_BUS___11_TN", false, HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT,
                                     dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0., 0.}, 0., vscStation1, vscStation2, boost::none);
  auto hvdcLineVSC3 = HVDCDefinition("HVDCVSCLine3", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3",
                                     "_BUS___12_TN", false, HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT,
                                     dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0., 0.}, 0., vscStation2, vscStation3, boost::none);
  auto hvdcLineVSC4 = HVDCDefinition("HVDCVSCLine4", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3",
                                     "_BUS___12_TN", false, HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT,
                                     dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulation, {0., 0.}, 0., vscStation2, vscStation3, 5);
  double powerFactor = 1. / std::sqrt(5.);  // sqrt(1/(2^2+1)) => qMax = 2 with pMax = 1
  auto hvdcLineLCC2 =
      HVDCDefinition("HVDCLCCLine2", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", true, "LCCStation2", "_BUS___22_TN", false,
                     HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {powerFactor, powerFactor},
                     1., boost::none, boost::none, boost::none);
  dfl::algo::HVDCLineDefinitions::HvdcLineMap hvdcLines = {
      std::make_pair(hvdcLineVSC.id, hvdcLineVSC),   std::make_pair(hvdcLineLCC.id, hvdcLineLCC),   std::make_pair(hvdcLineVSC2.id, hvdcLineVSC2),
      std::make_pair(hvdcLineVSC3.id, hvdcLineVSC3), std::make_pair(hvdcLineVSC4.id, hvdcLineVSC4), std::make_pair(hvdcLineLCC2.id, hvdcLineLCC2),
  };
  dfl::algo::HVDCLineDefinitions::BusVSCMap vscIds = {
      std::make_pair("_BUS___10_TN", vscStation1),
      std::make_pair("_BUS___11_TN", vscStation2),
      std::make_pair("_BUS___12_TN", vscStation3),
  };
  dfl::algo::HVDCLineDefinitions hvdcDefs{hvdcLines, vscIds};

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(
      dfl::outputs::Par::ParDefinition(basename, outputPathResults, outputPath.generic_string(), {},
                                  hvdcDefs, activePowerCompensation, {}, manager, {}, {}, {}, {}, {}));

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
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
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
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, outputPathResults, outputPath.generic_string(), generators,
                                                              {}, activePowerCompensation, {}, manager, counters, defs, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeStaticVarCompensator) {
  using dfl::algo::StaticVarCompensatorDefinition;

  dfl::inputs::DynamicDataBaseManager manager("", "");

  std::string basename = "TestParSVarC";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<StaticVarCompensatorDefinition> svarcs{
      StaticVarCompensatorDefinition("SVARC0", StaticVarCompensatorDefinition::ModelType::SVARCPV,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC1", StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC2", StaticVarCompensatorDefinition::ModelType::SVARCPVPROP,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC3", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC4", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC5", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC6", StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC7", StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC8", StaticVarCompensatorDefinition::ModelType::NETWORK,
      0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.)
  };
  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, outputPathResults, outputPath.generic_string(), {}, {}, activePowerCompensation, {},
                                                               manager, {}, {}, {}, svarcs, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeLoad) {
  using dfl::algo::LoadDefinition;

  dfl::inputs::DynamicDataBaseManager manager("", "");

  std::string basename = "TestParLoad";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<LoadDefinition> loads {
    LoadDefinition("LOAD1", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
    LoadDefinition("LOAD2", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
    LoadDefinition("LOAD3", LoadDefinition::ModelType::NETWORK, "Slack")
  };


  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  outputPath.append(filename);
  dfl::inputs::Configuration::ActivePowerCompensation activePowerCompensation(dfl::inputs::Configuration::ActivePowerCompensation::P);
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, outputPathResults, outputPath.generic_string(), {}, {}, activePowerCompensation, {},
                                                               manager, {}, {}, {}, {}, loads));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
