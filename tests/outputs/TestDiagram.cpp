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
testMultiplesFilesEquality(const std::vector<dfl::algo::GeneratorDefinition>& generators, boost::filesystem::path outputDirectory, std::string basename,
                           std::string prefixDir) {
  using dfl::algo::GeneratorDefinition;

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);
  for (const GeneratorDefinition& gen : generators) {
    if (gen.model != dfl::algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN &&
        gen.model != dfl::algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN)
      continue;
    boost::filesystem::path ref(reference);
    boost::filesystem::path outputDir(outputDirectory);
    dfl::test::checkFilesEqual(outputDir.append(dfl::outputs::constants::diagramFilename(gen)).generic_string(),
                               ref.append(dfl::outputs::constants::diagramFilename(gen)).generic_string());
  }
}

TEST(Diagram, writeWithCurvePoint) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "WithCurvePoint";
  boost::filesystem::path outputDirectory("results");
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);

  std::vector<GeneratorDefinition> generators = {GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN, "00",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),

                                                                     },
                                                                     1., 10., 11., 110.),
                                                 GeneratorDefinition("G1", GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "01",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                     },
                                                                     2., 20., 22., 220.),
                                                 GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2.7, 22., 220.),

                                                                     },
                                                                     3., 30., 33., 330.),
                                                 GeneratorDefinition("G3", GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "03",
                                                                     {
                                                                         GeneratorDefinition::ReactiveCurvePoint(1., -11., 110.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                                                                         GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                                                                     },
                                                                     4., 40., 44., 440.)};

  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators));

  DiagramWriter.write();
  testMultiplesFilesEquality(generators, outputDirectory, basename, prefixDir);
}

TEST(Diagram, writeWithCurveAndDefaultPoints) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "Mixed";
  boost::filesystem::path outputDirectory("results");
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "00",
                          {
                              GeneratorDefinition::ReactiveCurvePoint(1., 11., 110.),
                              GeneratorDefinition::ReactiveCurvePoint(3., 33., 330.),
                              GeneratorDefinition::ReactiveCurvePoint(4., 44., 440.),
                              GeneratorDefinition::ReactiveCurvePoint(2., 22., 220.),

                          },
                          1., 10., -11., 110.),
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "01", {}, -20., -2., 22., 220.),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02",
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
                          3., 30., 33., 330.),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "03", {}, 4., 40., 44., 440.)};

  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators));

  DiagramWriter.write();
  testMultiplesFilesEquality(generators, outputDirectory, basename, prefixDir);
}

TEST(Diagram, writeEmpty) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string prefixDir = "Empty";
  boost::filesystem::path outputDirectory("results");
  outputDirectory.append(basename);

  if (!boost::filesystem::exists(outputDirectory)) {
    boost::filesystem::create_directories(outputDirectory);
  }
  outputDirectory.append(prefixDir + dfl::outputs::constants::diagramDirectorySuffix);

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::SIGNALN, "01", {}, -20., -2., 22., 220.),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "03", {}, 4., 40., 44., 440.)};

  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDirectory.generic_string(), generators));

  DiagramWriter.write();
  std::string directoryPath = outputDirectory.generic_string();
  try {
    std::ifstream stream(directoryPath);
    ASSERT_TRUE(stream.fail()) << "The Diagram directory " << directoryPath << " has been created to even though there are no generators which model requires it."
                               << std::endl;
  } catch (std::exception& e) {
    ASSERT_TRUE(false) << "An IO error has occured" << std::endl;
  }
}
