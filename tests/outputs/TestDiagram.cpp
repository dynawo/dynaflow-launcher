//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Constants.h"
#include "Diagram.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

static void
testMultiplesFilesEquality(const std::vector<dfl::algo::GeneratorDefinition>& generators, const boost::filesystem::path& outputDirectory,
                           const std::string& basename, const std::string& prefixDir) {
  using dfl::algo::GeneratorDefinition;

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  for (const GeneratorDefinition& gen : generators) {
    if (!gen.isUsingDiagram())
      continue;
    boost::filesystem::path ref(reference);
    boost::filesystem::path outputDir(outputDirectory);
    dfl::test::checkFilesEqual(outputDir.append(dfl::outputs::constants::diagramFilename(gen.id)).generic_string(),
                               ref.append(dfl::outputs::constants::diagramFilename(gen.id)).generic_string());
  }
}

TEST(Diagram, writeWithCurvePoint) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "WithCurvePoint";
  boost::filesystem::path outputDirectory(outputPathResults);
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  const std::string bus1 = "BUS_1";
  std::vector<GeneratorDefinition> generators = {GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "00",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                     },
                                                                     1., 10., 11., 110., 100, bus1),
                                                 GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2.7, 22., 220.),
                                                                     },
                                                                     3., 30., 33., 330., 100, bus1)};

  dfl::algo::HVDCLineDefinitions defs;

  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators, defs));

  DiagramWriter.write();
  testMultiplesFilesEquality(generators, outputDirectory, basename, prefixDir);
}

TEST(Diagram, writeWithCurveAndDefaultPoints) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "Mixed";
  boost::filesystem::path outputDirectory(outputPathResults);
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  const std::string bus1 = "BUS_1";

  std::vector<GeneratorDefinition> generators = {GeneratorDefinition("G0", GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN, "00",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                     },
                                                                     1., 10., -11., 110., 100, bus1),
                                                 GeneratorDefinition("G2", GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN, "02",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(8., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(7., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(10., 987., 2394.43),
                                                                         GeneratorDefinition::ReactiveCurvePoint(6., 44., 31.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(5., 42., 49.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(59.8, 484., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2.7, 22., 220.),
                                                                     },
                                                                     3., 30., 33., 330., 100, bus1)};

  dfl::algo::HVDCLineDefinitions defs;
  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators, defs));

  DiagramWriter.write();
  testMultiplesFilesEquality(generators, outputDirectory, basename, prefixDir);
}

TEST(Diagram, writeEmpty) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "Empty";
  boost::filesystem::path outputDirectory(outputPathResults);
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  const std::string bus1 = "BUS_1";

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::SIGNALN_INFINITE, "01", {}, -20., -2., 22., 220., 100, bus1),
      GeneratorDefinition("G6", GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE, "63", {}, 4., 40., 44., 440., 100, bus1),
      GeneratorDefinition("G4", GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE, "04", {}, -20., -2., 22., 220., 100, bus1)};

  dfl::algo::HVDCLineDefinitions defs;
  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators, defs));

  DiagramWriter.write();
  std::string directoryPath = outputDirectory.generic_string();
  try {
    std::ifstream stream(directoryPath);
    ASSERT_TRUE(stream.fail()) << "The Diagram directory " << directoryPath << " has been created even though it is not used by any generator model";
  } catch (std::exception& e) {
    ASSERT_TRUE(false) << "An IO error has occured: " << e.what();
  }
}

TEST(Diagram, writeVSC) {
  using dfl::algo::HVDCDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "VSC";
  boost::filesystem::path outputDirectory(outputPathResults);
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);

  dfl::algo::HVDCLineDefinitions::HvdcLineMap map{
      std::make_pair(
          "0", HVDCDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation1", "_BUS___11_TN", false, "VSCStation99", "_BUS___99_TN",
                              false, HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {}, 0.,
                              dfl::algo::VSCDefinition("VSCStation1", 51, -51, 21, {}), boost::none, boost::none)),
      std::make_pair("1", HVDCDefinition("HVDCVSCLine1", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation99", "_BUS___99_TN", false, "VSCStation2",
                                         "_BUS___12_TN", false, HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT,
                                         HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulation, {}, 0., boost::none,
                                         dfl::algo::VSCDefinition("VSCStation2", 52, -52, 22,
                                                                  {
                                                                      dfl::algo::VSCDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                      dfl::algo::VSCDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                      dfl::algo::VSCDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                      dfl::algo::VSCDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                  }),
                                         boost::none)),
      std::make_pair(
          "2", HVDCDefinition("HVDCVSCLine2", dfl::inputs::HvdcLine::ConverterType::VSC, "VSCStation3", "_BUS___13_TN", false, "VSCStation4", "_BUS___14_TN",
                              false, HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ, {}, 0.,
                              dfl::algo::VSCDefinition("VSCStation3", 53, -53, 23, {}), dfl::algo::VSCDefinition("VSCStation4", 54, -54, 24, {}), boost::none)),
  };
  dfl::algo::HVDCLineDefinitions::BusVSCMap vscIds{};
  dfl::algo::HVDCLineDefinitions defs{map, vscIds};
  std::vector<dfl::algo::GeneratorDefinition> generators;

  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators, defs));

  DiagramWriter.write();

  // check reference
  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  for (const auto& vscPair : vscIds) {
    boost::filesystem::path ref(reference);
    boost::filesystem::path outputDir(outputDirectory);
    dfl::test::checkFilesEqual(outputDir.append(dfl::outputs::constants::diagramFilename(vscPair.second.id)).generic_string(),
                               ref.append(dfl::outputs::constants::diagramFilename(vscPair.second.id)).generic_string());
  }
}

TEST(Diagram, writeLCC) {
  using dfl::algo::HVDCDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "LCC";
  boost::filesystem::path outputDirectory(outputPathResults);
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);

  const std::vector<std::string> lccIds{"LCCStation1", "LCCStation11", "LCCStation12", "LCCStation2"};

  dfl::algo::HVDCLineDefinitions::HvdcLineMap map{
      std::make_pair("0", HVDCDefinition("HVDCLCCLine", HVDCDefinition::ConverterType::LCC, "LCCStation1", "_BUS___11_TN", false, "LCCStation99",
                                         "_BUS___99_TN", false, HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT,
                                         HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0.5, 0.5}, 0., boost::none, boost::none, boost::none)),
      std::make_pair("1", HVDCDefinition("HVDCLCCLine1", HVDCDefinition::ConverterType::LCC, "LCCStation99", "_BUS___99_TN", false, "LCCStation11",
                                         "_BUS___11_TN", false, HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT,
                                         HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0.5, 0.5}, 10., boost::none, boost::none, boost::none)),
      std::make_pair("2", HVDCDefinition("HVDCLCCLine2", HVDCDefinition::ConverterType::LCC, "LCCStation12", "_BUS___12_TN", false, "LCCStation2",
                                         "_BUS___22_TN", false, HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT,
                                         HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, {0.5, 0.5}, 10., boost::none, boost::none, boost::none)),
  };

  dfl::algo::HVDCLineDefinitions::BusVSCMap vscIds{};
  dfl::algo::HVDCLineDefinitions defs{map, vscIds};
  std::vector<dfl::algo::GeneratorDefinition> generators;

  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators, defs));

  DiagramWriter.write();

  // check reference
  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  for (const auto& id : lccIds) {
    boost::filesystem::path ref(reference);
    boost::filesystem::path outputDir(outputDirectory);
    dfl::test::checkFilesEqual(outputDir.append(dfl::outputs::constants::diagramFilename(id)).generic_string(),
                               ref.append(dfl::outputs::constants::diagramFilename(id)).generic_string());
  }
}
