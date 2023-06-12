//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Dyd.h"
#include "Tests.h"

#include <DYNMPIContext.h>

#include <boost/filesystem.hpp>

testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::mpi::Context mpiContext;

using dfl::algo::DynamicModelDefinitions;
using dfl::algo::GeneratorDefinition;
using dfl::algo::GeneratorDefinitionAlgorithm;
using dfl::algo::HVDCLineDefinitions;
using dfl::algo::LoadDefinition;
using dfl::algo::StaticVarCompensatorDefinition;

TEST(Dyd, write) {
  std::string basename = "TestDyd";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("", "");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<LoadDefinition> loads = {LoadDefinition("L0", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "00"),
                                       LoadDefinition("L1", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "01"),
                                       LoadDefinition("L2", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "02"),
                                       LoadDefinition("L3", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "03")};

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::NETWORK, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G6", GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G7", GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_SIGNALN, "00", {}, 1., 10., -11., 110., 0, 0., bus1)};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  HVDCLineDefinitions noHvdcDefs;
  GeneratorDefinitionAlgorithm::BusGenMap noBuses;
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(
      dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), generators, loads, node, noHvdcDefs, noBuses, manager, noModels, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeRemote) {
  std::string basename = "TestDydRemote";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("", "");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "01", {}, 2., 20., 22., 220., 0, 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "03", {}, 4., 40., 44., 440., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "01", {}, 2., 20., 22., 220., 0, 100, bus2),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "01", {}, 2., 20., 22., 220., 0, 100, bus2)};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  HVDCLineDefinitions noHvdcDefs;
  GeneratorDefinitionAlgorithm::BusGenMap busesRegulatedBySeveralGenerators = {{bus1, "G1"}, {bus2, "G4"}};
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), generators, {}, node, noHvdcDefs,
                                                               busesRegulatedBySeveralGenerators, manager, noModels, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeHvdc) {
  using dfl::algo::HVDCDefinition;

  std::string basename = "TestDydHvdc";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("", "");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  auto hvdcLineLCC = HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", false, "LCCStation2",
                                    "_BUS___10_TN", false, HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPTanPhiDangling,
                                    {}, 0., boost::none, boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01});
  auto hvdcLineVSC = HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2",
                                    "_BUS___11_TN", false, HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPVDangling, {},
                                    0., boost::none, boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01});
  //  maybe watch out but you can't access the hdvLine from the converterInterface
  HVDCLineDefinitions::HvdcLineMap hvdcLines = {std::make_pair(hvdcLineVSC.id, hvdcLineVSC), std::make_pair(hvdcLineLCC.id, hvdcLineLCC)};
  HVDCLineDefinitions::BusVSCMap vscIds = {
      std::make_pair("_BUS___10_TN", "VSCStation1"),
  };
  HVDCLineDefinitions hvdcDefs{hvdcLines, vscIds};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  GeneratorDefinitionAlgorithm::BusGenMap noBuses;
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), {}, {}, node, hvdcDefs, noBuses, manager, noModels, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeDynamicModel) {
  std::string basename = "TestDydDynModel";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("res/setting.xml", "res/assembling.xml");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  DynamicModelDefinitions models;
  models.usedMacroConnections.insert("ToUMeasurement");
  models.usedMacroConnections.insert("ToControlledShunts");
  models.usedMacroConnections.insert("TestLoadConnection");
  models.models.insert({"SVC", dfl::algo::DynamicModelDefinition("SVC", "SecondaryVoltageControlSimp")});
  models.models.insert({"MODELE_1_VL4", dfl::algo::DynamicModelDefinition("MODELE_1_VL4", "DYNModel1")});

  auto macro =
      dfl::algo::DynamicModelDefinition::MacroConnection("ToUMeasurement", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE, "0", "");
  models.models.at("MODELE_1_VL4").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("ToControlledShunts", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::SHUNT,
                                                             "1.1", "");
  models.models.at("MODELE_1_VL4").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("ToControlledShunts", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::SHUNT,
                                                             "1.2", "");
  models.models.at("MODELE_1_VL4").nodeConnections.insert(macro);
  macro =
      dfl::algo::DynamicModelDefinition::MacroConnection("TestLoadConnection", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LOAD, "L0", "");
  models.models.at("MODELE_1_VL4").nodeConnections.insert(macro);
  macro =
      dfl::algo::DynamicModelDefinition::MacroConnection("TestLoadConnection", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::LOAD, "L1", "");
  models.models.at("MODELE_1_VL4").nodeConnections.insert(macro);

  models.usedMacroConnections.insert("SVCToUMeasurement");
  models.usedMacroConnections.insert("SVCToGenerator");
  models.usedMacroConnections.insert("SVCToHVDC");

  macro =
      dfl::algo::DynamicModelDefinition::MacroConnection("SVCToUMeasurement", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE, "0", "");
  models.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToGenerator", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, "G5",
                                                             "SVCToComponent");
  models.models.at("SVC").nodeConnections.insert(macro);

  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToHVDC", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::HVDC, "HVDCVSCLine",
                                                             "SVCToComponent");
  models.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToGenerator", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, "G6",
                                                             "SVCToComponent");
  models.models.at("SVC").nodeConnections.insert(macro);

  std::vector<LoadDefinition> loads = {LoadDefinition("L0", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "00"),
                                       LoadDefinition("L1", LoadDefinition::ModelType::NETWORK, "01"),
                                       LoadDefinition("L2", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "02"),
                                       LoadDefinition("L3", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "03")};

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G6", GeneratorDefinition::ModelType::NETWORK, "00", {}, 1., 10., -11., 110., 0, 0., bus1)};

  auto hvdcLineVSC =
      dfl::algo::HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "BUS_1", true, "VSCStation2", "BUS_2", false,
                                dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1,
                                {}, 0., boost::none, boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01});
  //  maybe watch out but you can't access the hdvLine from the converterInterface
  HVDCLineDefinitions::HvdcLineMap hvdcLines = {std::make_pair(hvdcLineVSC.id, hvdcLineVSC)};
  HVDCLineDefinitions::BusVSCMap vscIds = {};
  HVDCLineDefinitions hvdcDefs{hvdcLines, vscIds};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  GeneratorDefinitionAlgorithm::BusGenMap noBuses;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(
      dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), generators, loads, node, hvdcDefs, noBuses, manager, models, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeStaticVarCompensator) {
  std::string basename = "TestDydSVarC";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("", "");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<StaticVarCompensatorDefinition> svarcs{
      StaticVarCompensatorDefinition("SVARC0", StaticVarCompensatorDefinition::ModelType::SVARCPV, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC1", StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10.,
                                     10.),
      StaticVarCompensatorDefinition("SVARC2", StaticVarCompensatorDefinition::ModelType::SVARCPVPROP, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC3", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0.,
                                     10., 10.),
      StaticVarCompensatorDefinition("SVARC4", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10.,
                                     10.),
      StaticVarCompensatorDefinition("SVARC5", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245,
                                     0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC6", StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
      StaticVarCompensatorDefinition("SVARC7", StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0.,
                                     10., 10.),
      StaticVarCompensatorDefinition("SVARC8", StaticVarCompensatorDefinition::ModelType::NETWORK, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.)};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  HVDCLineDefinitions noHvdcDefs;
  GeneratorDefinitionAlgorithm::BusGenMap noBuses;
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(
      dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), {}, {}, node, noHvdcDefs, noBuses, manager, noModels, svarcs));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeLoad) {
  std::string basename = "TestDydLoad";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("", "");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<LoadDefinition> loads{LoadDefinition("LOAD1", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
                                    LoadDefinition("LOAD2", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
                                    LoadDefinition("LOAD3", LoadDefinition::ModelType::NETWORK, "Slack")};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  HVDCLineDefinitions noHvdcDefs;
  GeneratorDefinitionAlgorithm::BusGenMap noBuses;
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(
      dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), {}, loads, node, noHvdcDefs, noBuses, manager, noModels, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeVRRemote) {
  using dfl::algo::HVDCDefinition;

  std::string basename = "TestDydVRRemote";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  dfl::inputs::DynamicDataBaseManager manager("", "");

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::shared_ptr<dfl::inputs::VoltageLevel> vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  std::shared_ptr<dfl::inputs::Node> node = dfl::inputs::Node::build("Slack", vl, 100., {});

  auto hvdcLineLCC = HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "BUS_3", false, "LCCStation2", "BUS_1", false,
                                    HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., boost::none,
                                    boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01});
  auto hvdcLineVSC = HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "BUS_1", true, "VSCStation2", "BUS_3", false,
                                    HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPVDangling, {}, 0., boost::none,
                                    boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01});
  HVDCLineDefinitions::HvdcLineMap hvdcLines = {std::make_pair(hvdcLineVSC.id, hvdcLineVSC), std::make_pair(hvdcLineLCC.id, hvdcLineLCC)};
  HVDCLineDefinitions::BusVSCMap vscIds = {std::make_pair("BUS_1", "VSCStation1"), std::make_pair("BUS_3", "VSCStation2")};
  HVDCLineDefinitions hvdcDefs{hvdcLines, vscIds};

  const std::string bus1 = "BUS_1";
  const std::string bus2 = "BUS_2";
  GeneratorDefinitionAlgorithm::BusGenMap busesRegulatedBySeveralGenerators = {{bus1, "G1"}, {bus2, "G4"}};
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::outputs::Dyd dydWriter(dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), {}, {}, node, hvdcDefs, busesRegulatedBySeveralGenerators,
                                                               manager, noModels, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
