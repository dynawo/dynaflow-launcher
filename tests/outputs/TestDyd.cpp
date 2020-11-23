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

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

TEST(Dyd, write) {
  using dfl::algo::GeneratorDefinition;
  using dfl::algo::LoadDefinition;

  std::string basename = "TestDyd";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath("results");
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  std::vector<LoadDefinition> loads = {LoadDefinition("L0", "00"), LoadDefinition("L1", "01"), LoadDefinition("L2", "02"), LoadDefinition("L3", "03")};

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN, "00", {}, 1., 10., 11., 110.),
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "01", {}, 2., 20., 22., 220.),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330.),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "03", {}, 4., 40., 44., 440.)};

  auto node = std::make_shared<dfl::inputs::Node>("Slack", 100.);

  outputPath.append(filename);

  dfl::outputs::Dyd dydWriter(dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), generators, loads, node, {}));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(Dyd, writeHvdc) {
  using dfl::algo::HvdcLineDefinition;

  std::string basename = "TestDydHvdc";
  std::string filename = basename + ".dyd";
  boost::filesystem::path outputPath("results");
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }
  auto lccStation1 = dfl::inputs::ConverterInterface("LCCStation1", "_BUS___11_TN", boost::optional<bool>());
  auto lccStation2 = dfl::inputs::ConverterInterface("LCCStation2", "_BUS___10_TN", boost::optional<bool>());
  auto vscStation1 = dfl::inputs::ConverterInterface("VSCStation1", "_BUS___10_TN", true);
  auto vscStation2 = dfl::inputs::ConverterInterface("VSCStation2", "_BUS___11_TN", false);
  
  auto hvdcLineLCC = dfl::algo::HvdcLineDefinition("HVDCLCCLine", dfl::inputs::HvdcLine::ConverterType::LCC, lccStation1, lccStation2, dfl::algo::HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT);
  auto hvdcLineVSC = dfl::algo::HvdcLineDefinition("HVDCVSCLine", dfl::inputs::HvdcLine::ConverterType::VSC, vscStation1, vscStation2, dfl::algo::HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT);
//maybe watch out but you can't access the hdvLine from the converterInterface
  std::vector<dfl::algo::HvdcLineDefinition> hvdcLines = {hvdcLineVSC, hvdcLineLCC};

  auto node = std::make_shared<dfl::inputs::Node>("Slack", 100.);

  outputPath.append(filename);

  dfl::outputs::Dyd dydWriter(dfl::outputs::Dyd::DydDefinition(basename, outputPath.generic_string(), {}, {}, node, hvdcLines));

  dydWriter.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}