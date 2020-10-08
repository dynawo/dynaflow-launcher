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

TEST(Dyd, write) {
  using dfl::algo::GeneratorDefinition;
  using dfl::algo::LoadDefinition;

  std::string basename = "TestDyd";
  std::string outputDir = "results/" + basename;
  std::string filename = basename + ".dyd";

  if (!boost::filesystem::exists(outputDir)) {
    boost::filesystem::create_directories(outputDir);
  }

  std::vector<LoadDefinition> loads = {LoadDefinition("L0", "00"), LoadDefinition("L1", "01"), LoadDefinition("L2", "02"), LoadDefinition("L3", "03")};

  std::vector<GeneratorDefinition> generators = {
      GeneratorDefinition("G0", GeneratorDefinition::ModelType::SIGNALN, "00", {}, 1., 10., 11., 110.),
      GeneratorDefinition("G1", GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN, "01", {}, 2., 20., 22., 220.),
      GeneratorDefinition("G2", GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN, "02", {}, 3., 30., 33., 330.),
      GeneratorDefinition("G3", GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN, "03", {}, 4., 40., 44., 440.)};

  auto node = std::make_shared<dfl::inputs::Node>("Slack", 100.);
  dfl::outputs::Dyd dydWriter(dfl::outputs::Dyd::DydDefinition(basename, outputDir + "/" + filename, generators, loads, node));

  dydWriter.write();

  dfl::test::checkFilesEqual(outputDir + "/" + filename, "reference/" + basename + "/" + filename);
}
