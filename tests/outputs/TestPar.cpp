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

#include <DYNMultiProcessingContext.h>

testing::Environment *initXmlEnvironment();

testing::Environment *const env = initXmlEnvironment();

DYNAlgorithms::multiprocessing::Context mpiContext;

using dfl::algo::DynamicModelDefinitions;
using dfl::algo::GeneratorDefinition;
using dfl::algo::GeneratorDefinitionAlgorithm;
using dfl::algo::HVDCLineDefinitions;
using dfl::algo::LoadDefinition;
using dfl::algo::StaticVarCompensatorDefinition;

TEST(TestPar, write) {
  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

  std::string basename = "TestPar";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "04", {}, 3., 30., -33., 330., 0, 0, bus1),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::NETWORK, "04", {}, 3., 30., -33., 330., 0, 0, bus1),
      GeneratorDefinition("G6", GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "04", {}, 3., 30., -33., 330., 0, 100, bus1),
      GeneratorDefinition("G7", GeneratorDefinition::ModelType::PROP_SIGNALN_RECTANGULAR, "04", {}, 3., 30., -33., 330., 0, 0, bus1),
      GeneratorDefinition("G8", GeneratorDefinition::ModelType::SIGNALN_TFO_RECTANGULAR, "04", {}, 3., 30., -33., 330., 0, 0, bus1),
      GeneratorDefinition("G9", GeneratorDefinition::ModelType::SIGNALN_TFO_RECTANGULAR, "04", {}, 3., 30., -33., 330., 0, 0, bus1, true),
      GeneratorDefinition("G10", GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_SIGNALN, "04", {}, 3., 30., -33., 330., 0, 0, bus1)};

  HVDCLineDefinitions noHvdcDefs;
  dfl::inputs::NetworkManager::BusMapRegulating busesToNumberOfRegulationMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES}};
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), generators, noHvdcDefs,
                                                               busesToNumberOfRegulationMap, manager, {}, noModels, {}, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeRemote) {
  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

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
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "01", {}, 2., 20., 22., 220., 3, 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "03", {}, 4., 40., 44., 440., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "01", {}, 2., 20., 22., 220., 20., 100, bus2),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "01", {}, 2., 20., 22., 220., 0, 100, bus2),
      GeneratorDefinition("G6", GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR, "01", {}, 2., 20., 22., 220., 0, 100, bus2),
      GeneratorDefinition("G7", GeneratorDefinition::ModelType::REMOTE_SIGNALN_RECTANGULAR, "01", {}, 2., 20., 22., 220., 0, 0, bus2)};

  HVDCLineDefinitions noHvdcDefs;
  dfl::inputs::NetworkManager::BusMapRegulating busesToNumberOfRegulationMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                                                {bus2, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES}};
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), generators, noHvdcDefs,
                                                               busesToNumberOfRegulationMap, manager, {}, noModels, {}, {}, {}, {}));

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

  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  dfl::algo::VSCDefinition vscStation1("VSCStation1", 1., -1., 0., 0., {});
  dfl::algo::VSCDefinition vscStation2("VSCStation2", 2., -2., 0., 0., {});
  dfl::algo::VSCDefinition vscStation3("VSCStation3", 3., -3., 0., 0., {});

  auto hvdcLineLCC =
      HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___21_TN", false, "LCCStation2", "_BUS___22_TN", false,
                     HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., boost::none,
                     boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSC =
      HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2", "_BUS___11_TN", false,
                     HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., vscStation1,
                     vscStation2, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSC2 =
      HVDCDefinition("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2", "_BUS___11_TN", false,
                     HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0., 0.}, 0., vscStation1,
                     vscStation2, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSC3 =
      HVDCDefinition("HVDCVSCLine3", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3", "_BUS___12_TN", true,
                     HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0., 0.}, 0., vscStation2,
                     vscStation3, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSC4 =
      HVDCDefinition("HVDCVSCLine4", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3", "_BUS___12_TN", true,
                     HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulationSet, {0., 0.}, 0., vscStation2,
                     vscStation3, 5, 120., false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSC5 =
      HVDCDefinition("HVDCVSCLine5", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3", "_BUS___12_TN", false,
                     HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulationSet, {0., 0.}, 0., vscStation2,
                     vscStation3, 5, 120., true, 320, 322, 0.125, {0.01, 0.01}, true);
  double powerFactor = 1. / std::sqrt(5.);  // sqrt(1/(2^2+1)) => qMax = 2 with pMax = 1
  auto hvdcLineLCC2 =
      HVDCDefinition("HVDCLCCLine2", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", false, "LCCStation2", "_BUS___22_TN", false,
                     HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ, {powerFactor, powerFactor},
                     1., boost::none, boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  dfl::algo::HVDCLineDefinitions::HvdcLineMap hvdcLines = {
      std::make_pair(hvdcLineVSC.id, hvdcLineVSC),   std::make_pair(hvdcLineLCC.id, hvdcLineLCC),   std::make_pair(hvdcLineVSC2.id, hvdcLineVSC2),
      std::make_pair(hvdcLineVSC3.id, hvdcLineVSC3), std::make_pair(hvdcLineVSC4.id, hvdcLineVSC4), std::make_pair(hvdcLineVSC5.id, hvdcLineVSC5),
      std::make_pair(hvdcLineLCC2.id, hvdcLineLCC2),
  };
  HVDCLineDefinitions hvdcDefs{hvdcLines};

  dfl::inputs::NetworkManager::BusMapRegulating busesToNumberOfRegulationMap = {};
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), {}, hvdcDefs, busesToNumberOfRegulationMap,
                                                               manager, {}, noModels, {}, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, DynModel) {
  dfl::inputs::DynamicDataBaseManager manager(std::vector<boost::filesystem::path>(1, "res/setting.xml"),
                                              std::vector<boost::filesystem::path>(1, "res/assembling.xml"));

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

  DynamicModelDefinitions defs;
  dfl::algo::DynamicModelDefinition dynModel("DM_VL61", "DummyLib");
  dynModel.nodeConnections.insert(
      dfl::algo::DynamicModelDefinition::MacroConnection("MacroTest", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO, "TfoId", ""));
  defs.models.insert({dynModel.id, dynModel});

  defs.usedMacroConnections.insert("SVCToUMeasurement");
  defs.usedMacroConnections.insert("SVCToGenerator");
  defs.usedMacroConnections.insert("SVCToHvdc");
  defs.usedMacroConnections.insert("CLAToIMeasurement");
  defs.models.insert({"SVC", dfl::algo::DynamicModelDefinition("SVC", "SecondaryVoltageControlSimp")});
  defs.models.insert({"DM_TEST", dfl::algo::DynamicModelDefinition("DM_TEST", "PhaseShifterI")});
  auto macro =
      dfl::algo::DynamicModelDefinition::MacroConnection("SVCToUMeasurement", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::NODE, "0", "");
  defs.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToGenerator", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, "G5",
                                                             "");
  defs.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToGenerator", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, "G6",
                                                             "");
  defs.models.at("SVC").nodeConnections.insert(macro);
  macro =
      dfl::algo::DynamicModelDefinition::MacroConnection("SVCToHvdc", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::HVDC, "HVDCVSCLine", "");
  defs.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToGenerator", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, "G7",
                                                             "");
  defs.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("SVCToGenerator", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::GENERATOR, "G8",
                                                             "");
  defs.models.at("SVC").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("CLAToIMeasurement", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO, "TFOId",
                                                             "");
  defs.models.at("DM_TEST").nodeConnections.insert(macro);
  macro = dfl::algo::DynamicModelDefinition::MacroConnection("CLAToIMeasurement", dfl::algo::DynamicModelDefinition::MacroConnection::ElementType::TFO,
                                                             "TFOId2", "");
  defs.models.at("DM_TEST").nodeConnections.insert(macro);

  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "04", {}, 3., 30., -33., 330., 0, 0, bus1),
      GeneratorDefinition("G5", GeneratorDefinition::ModelType::SIGNALN_RPCL_INFINITE, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G6", GeneratorDefinition::ModelType::SIGNALN_TFO_RPCL_INFINITE, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G7", GeneratorDefinition::ModelType::DIAGRAM_PQ_RPCL_SIGNALN, "00", {}, 1., 10., -11., 110., 0, 0., bus1),
      GeneratorDefinition("G8", GeneratorDefinition::ModelType::DIAGRAM_PQ_TFO_RPCL_SIGNALN, "00", {}, 1., 10., -11., 110., 0, 0., bus1)};

  dfl::algo::VSCDefinition vscStation22("VSCStation22", 2., -2., 0., 0., {});
  dfl::algo::VSCDefinition vscStation33("VSCStation33", 2., -3., 0., 0., {});
  dfl::algo::VSCDefinition vscStation44("VSCStation44", 2., -4., 0., 0., {});
  auto dummyStationVSC = std::make_shared<dfl::inputs::VSCConverter>("StationN", "BUS_2", nullptr, false, 0., 0., 0.,
                                                                     std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto vscStation2 = std::make_shared<dfl::inputs::VSCConverter>("VSCStation2", "BUS_1", nullptr, false, 0., 0., 0.,
                                                                 std::vector<dfl::inputs::VSCConverter::ReactiveCurvePoint>{});
  auto hvdcLineVSC = dfl::algo::HVDCDefinition(
      "HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "BUS_1", true, "VSCStation2", "BUS_2", false,
      dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1, {}, 0.,
      dfl::algo::VSCDefinition(dummyStationVSC->converterId, dummyStationVSC->qMax, dummyStationVSC->qMin, dummyStationVSC->qMin, 10., dummyStationVSC->points),
      dfl::algo::VSCDefinition(vscStation2->converterId, vscStation2->qMax, vscStation2->qMin, vscStation2->qMin, 10., vscStation2->points), boost::none,
      boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSCACEmulation1 =
      dfl::algo::HVDCDefinition("HVDCVSCLine1", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation22", "BUS_22", true, "VSCStation33", "BUS_33", true,
                                dfl::algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSet, {0., 0.},
                                0., vscStation22, vscStation33, 5, 120., false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSCACEmulation2 =
      dfl::algo::HVDCDefinition("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation22", "BUS_22", true, "VSCStation44", "BUS_44", true,
                                dfl::algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSet, {0., 0.},
                                0., vscStation22, vscStation44, 5, 120., false, 320, 322, 0.125, {0.01, 0.01}, true);
  //  maybe watch out but you can't access the hdvLine from the converterInterface
  HVDCLineDefinitions::HvdcLineMap hvdcLines = {std::make_pair(hvdcLineVSC.id, hvdcLineVSC),
                                                std::make_pair(hvdcLineVSCACEmulation1.id, hvdcLineVSCACEmulation1),
                                                std::make_pair(hvdcLineVSCACEmulation2.id, hvdcLineVSCACEmulation2)};
  HVDCLineDefinitions hvdcDefs{hvdcLines};
  dfl::inputs::NetworkManager::BusMapRegulating noBuses;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto vl2 = std::make_shared<dfl::inputs::VoltageLevel>("VLb");
  std::vector<std::shared_ptr<dfl::inputs::Node>> nodes{dfl::inputs::Node::build("VL0", vl2, 0.0, {}), dfl::inputs::Node::build("VL1", vl, 1.0, {})};
  auto tfo = dfl::inputs::Tfo::build("TFOId", nodes[0], nodes[1], "SUMMER", true, true);
  auto tfo2 = dfl::inputs::Tfo::build("TFOId2", nodes[0], nodes[1], "SUMMER", true, true);
  dfl::algo::TransformersByIdDefinitions tfosById;
  tfosById.tfosMap.insert({"TFOId", *tfo});
  tfosById.tfosMap.insert({"TFOId2", *tfo});
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), generators, hvdcDefs, noBuses, manager, counters,
                                                               defs, {}, tfosById, {}, {}));
  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeStaticVarCompensator) {
  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

  std::string basename = "TestParSVarC";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

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

  HVDCLineDefinitions noHvdcDefs;
  dfl::inputs::NetworkManager::BusMapRegulating noBuses;
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  dfl::outputs::Par parWriter(
      dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), {}, noHvdcDefs, noBuses, manager, {}, noModels, {}, {}, svarcs, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeLoad) {
  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

  std::string basename = "TestParLoad";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<LoadDefinition> loads{LoadDefinition("LOAD1", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
                                    LoadDefinition("LOAD2", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
                                    LoadDefinition("LOAD3", LoadDefinition::ModelType::NETWORK, "Slack")};

  auto vl = std::make_shared<dfl::inputs::VoltageLevel>("VL");
  auto node = dfl::inputs::Node::build("Slack", vl, 100., {});

  HVDCLineDefinitions noHvdcDefs;
  dfl::inputs::NetworkManager::BusMapRegulating noBuses;
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  dfl::outputs::Par parWriter(
      dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), {}, noHvdcDefs, noBuses, manager, {}, noModels, {}, {}, {}, loads));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, writeVRRemote) {
  using dfl::algo::HVDCDefinition;
  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

  std::string basename = "TestParVRRemote";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  auto hvdcLineLCC = HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "BUS_3", false, "LCCStation2", "BUS_1", false,
                                    HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., boost::none,
                                    boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  auto hvdcLineVSC = HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "BUS_1", false, "VSCStation2", "BUS_3", true,
                                    HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPQPropDangling, {}, 0., boost::none,
                                    boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
  HVDCLineDefinitions::HvdcLineMap hvdcLines = {std::make_pair(hvdcLineVSC.id, hvdcLineVSC), std::make_pair(hvdcLineLCC.id, hvdcLineLCC)};
  HVDCLineDefinitions hvdcDefs{hvdcLines};

  std::string bus1 = "BUS_1";
  std::string bus2 = "BUS_2";
  std::string bus3 = "BUS_3";
  dfl::inputs::NetworkManager::BusMapRegulating busesToNumberOfRegulationMap = {{bus1, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                                                {bus2, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES},
                                                                                {bus3, dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES}};

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "04", {}, 3., 30., -33., 330., 0, 0, bus2)};
  DynamicModelDefinitions noModels;

  outputPath.append(filename);
  dfl::inputs::Configuration config("res/config_activepowercompensation_p.json");
  dfl::outputs::Par parWriter(dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), generators, hvdcDefs,
                                                               busesToNumberOfRegulationMap, manager, {}, noModels, {}, {}, {}, {}));

  parWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(TestPar, startingPointMode) {
  using dfl::algo::HVDCDefinition;

  std::vector<boost::filesystem::path> emptyPathList;
  dfl::inputs::DynamicDataBaseManager manager(emptyPathList, emptyPathList);

  std::string basename = "TestStartingPointMode";

  boost::filesystem::path directoryOutputPath(outputPathResults);
  directoryOutputPath.append(basename);

  if (!boost::filesystem::exists(directoryOutputPath)) {
    boost::filesystem::create_directories(directoryOutputPath);
  }

  std::map<std::string, std::string> startingPointModeArray = {{"Default", "config_no_startingpointmode.json"},
                                                               {"WarmNoIRL", "config_startingpointmode_warm.json"},
                                                               {"FlatNoIRL", "config_startingpointmode_flat.json"},
                                                               {"WarmWithIRL", "config_warm_infinite_reactive_limits.json"},
                                                               {"FlatWithIRL", "config_flat_infinite_reactive_limits.json"}};
  for (const auto &startingPointMode : startingPointModeArray) {
    std::string startingPointModeName = startingPointMode.first;
    std::string startingPointModeFile = startingPointMode.second;

    std::vector<LoadDefinition> loads{LoadDefinition("LOAD1", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
                                      LoadDefinition("LOAD2", LoadDefinition::ModelType::LOADRESTORATIVEWITHLIMITS, "Slack"),
                                      LoadDefinition("LOAD3", LoadDefinition::ModelType::NETWORK, "Slack")};
    const std::string bus1 = "BUS_1";
    std::vector<GeneratorDefinition> generators = {
        GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00", {}, 1., 10., 11., 110., 0, 100, bus1),
        GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330., 0, 100, bus1),
        GeneratorDefinition("G4", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "04", {}, 3., 30., -33., 330., 0, 0, bus1)};
    dfl::algo::VSCDefinition vscStation1("VSCStation1", 1., -1., 0., 0., {});
    dfl::algo::VSCDefinition vscStation2("VSCStation2", 2., -2., 0., 0., {});
    dfl::algo::VSCDefinition vscStation3("VSCStation3", 3., -3., 0., 0., {});

    auto hvdcLineLCC =
        HVDCDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___21_TN", false, "LCCStation2", "_BUS___22_TN", false,
                       HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., boost::none,
                       boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
    auto hvdcLineVSC =
        HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2", "_BUS___11_TN", false,
                       HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, {}, 0., vscStation1,
                       vscStation2, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
    auto hvdcLineVSC2 =
        HVDCDefinition("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___10_TN", true, "VSCStation2", "_BUS___11_TN", false,
                       HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0., 0.}, 0., vscStation1,
                       vscStation2, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
    auto hvdcLineVSC3 =
        HVDCDefinition("HVDCVSCLine3", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3", "_BUS___12_TN", true,
                       HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0., 0.}, 0., vscStation2,
                       vscStation3, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
    auto hvdcLineVSC4 =
        HVDCDefinition("HVDCVSCLine4", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3", "_BUS___12_TN", true,
                       HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulationSet, {0., 0.}, 0.,
                       vscStation2, vscStation3, 5, 120., false, 320, 322, 0.125, {0.01, 0.01}, true);
    auto hvdcLineVSC5 =
        HVDCDefinition("HVDCVSCLine5", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation2", "_BUS___11_TN", true, "VSCStation3", "_BUS___12_TN", false,
                       HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulationSet, {0., 0.}, 0.,
                       vscStation2, vscStation3, 5, 120., true, 320, 322, 0.125, {0.01, 0.01}, true);
    double powerFactor = 1. / std::sqrt(5.);  // sqrt(1/(2^2+1)) => qMax = 2 with pMax = 1
    auto hvdcLineLCC2 =
        HVDCDefinition("HVDCLCCLine2", dfl::inputs::HvdcLine::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", false, "LCCStation2", "_BUS___22_TN", false,
                       HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, dfl::algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ, {powerFactor, powerFactor},
                       1., boost::none, boost::none, boost::none, boost::none, false, 320, 322, 0.125, {0.01, 0.01}, true);
    dfl::algo::HVDCLineDefinitions::HvdcLineMap hvdcLines = {
        std::make_pair(hvdcLineVSC.id, hvdcLineVSC),   std::make_pair(hvdcLineLCC.id, hvdcLineLCC),   std::make_pair(hvdcLineVSC2.id, hvdcLineVSC2),
        std::make_pair(hvdcLineVSC3.id, hvdcLineVSC3), std::make_pair(hvdcLineVSC4.id, hvdcLineVSC4), std::make_pair(hvdcLineVSC5.id, hvdcLineVSC5),
        std::make_pair(hvdcLineLCC2.id, hvdcLineLCC2),
    };
    HVDCLineDefinitions hvdcDefs{hvdcLines};

    std::vector<StaticVarCompensatorDefinition> svarcs{
        StaticVarCompensatorDefinition("SVARC0", StaticVarCompensatorDefinition::ModelType::SVARCPV, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
        StaticVarCompensatorDefinition("SVARC1", StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10.,
                                       10.),
        StaticVarCompensatorDefinition("SVARC2", StaticVarCompensatorDefinition::ModelType::SVARCPVPROP, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
        StaticVarCompensatorDefinition("SVARC3", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245, 0.,
                                       10., 10.),
        StaticVarCompensatorDefinition("SVARC4", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10.,
                                       10.),
        StaticVarCompensatorDefinition("SVARC5", StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING, 0., 10., 100, 230, 215, 230, 235,
                                       245, 0., 10., 10.),
        StaticVarCompensatorDefinition("SVARC6", StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.),
        StaticVarCompensatorDefinition("SVARC7", StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING, 0., 10., 100, 230, 215, 230, 235, 245,
                                       0., 10., 10.),
        StaticVarCompensatorDefinition("SVARC8", StaticVarCompensatorDefinition::ModelType::NETWORK, 0., 10., 100, 230, 215, 230, 235, 245, 0., 10., 10.)};

    dfl::inputs::NetworkManager::BusMapRegulating noBuses;
    DynamicModelDefinitions noModels;

    boost::filesystem::path outputPath = directoryOutputPath;
    std::string filename = basename + startingPointModeName + ".par";
    outputPath.append(filename);

    std::string startingPointConfigFilePath = "res/" + startingPointModeFile;
    dfl::inputs::Configuration config(startingPointConfigFilePath);
    dfl::outputs::Par startingPointModeParWriter(dfl::outputs::Par::ParDefinition(basename, config, outputPath.generic_string(), generators, hvdcDefs, noBuses,
                                                                                  manager, {}, noModels, {}, {}, svarcs, loads));

    startingPointModeParWriter.write();

    boost::filesystem::path reference("reference");
    reference.append(basename);

    std::string referenceFilename;
    if (startingPointModeName != "Default") {
      referenceFilename = filename;
    } else {
      // When StartingPointMode isn't specified, its value should be warm
      referenceFilename = basename + "WarmNoIRL" + ".par";
    }
    reference.append(referenceFilename);

    dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
  }
}
