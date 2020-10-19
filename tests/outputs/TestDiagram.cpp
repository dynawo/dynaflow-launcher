//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Diagram.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

TEST(Diagram, writeWithCurvePoint) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string outputDir = "results/" + basename;
  std::string filename = basename + ".txt";

  if (!boost::filesystem::exists(outputDir)) {
    boost::filesystem::create_directories(outputDir);
  }

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
  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDir + "/" + filename, generators));

  DiagramWriter.write();

  dfl::test::checkFilesEqual(outputDir + "/" + filename, "reference/" + basename + "/" + filename);
}

TEST(Diagram, writeWithCurveAndDefaultPoints) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string outputDir = "results/" + basename;
  std::string filename = "TestDiagramMixed.txt";

  if (!boost::filesystem::exists(outputDir)) {
    boost::filesystem::create_directories(outputDir);
  }

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
  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDir + "/" + filename, generators));

  DiagramWriter.write();

  dfl::test::checkFilesEqual(outputDir + "/" + filename, "reference/" + basename + "/" + filename);
}

TEST(Diagram, writeEmpty) {
  using dfl::algo::GeneratorDefinition;

  std::string basename = "TestDiagram";
  std::string outputDir = "results/" + basename;
  std::string filename = "TestDiagramEmpty.txt";

  if (!boost::filesystem::exists(outputDir)) {
    boost::filesystem::create_directories(outputDir);
  }

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::SIGNALN, "01", {}, -20., -2., 22., 220.),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "03", {}, 4., 40., 44., 440.)};
  dfl::outputs::Diagram DiagramWriter(dfl::outputs::Diagram::DiagramDefinition(basename, outputDir + "/" + filename, generators));

  DiagramWriter.write();
  std::string filePath = outputDir + "/" + filename;
  try {
    std::ifstream stream(filePath);
    ASSERT_TRUE(stream.fail()) << "The Diagram file has been written to even though there are no generators which model requires it." << std::endl;
  } catch (std::exception& e) {
    ASSERT_TRUE(false) << "An IO error has occured\n" << std::endl;
  }
}