//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "OutputsConstants.h"
#include "Solver.h"
#include "Tests.h"

TEST(SolverPar, writeDefaultValues) {
  const std::string basename = "TestSolverParDefault";

  dfl::inputs::Configuration config("res/config_solver_default.json");
  dfl::outputs::Solver solverWriter{dfl::outputs::Solver::SolverDefinition(config)};
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);
  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }
  solverWriter.write();
  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(dfl::outputs::constants::solverParFileName);
  outputPath.append(dfl::outputs::constants::solverParFileName);
  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}

TEST(SolverPar, write) {
  const std::string basename = "TestSolverPar";

  dfl::inputs::Configuration config("res/config_solver.json");
  dfl::outputs::Solver solverWriter{dfl::outputs::Solver::SolverDefinition(config)};
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);
  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }
  solverWriter.write();
  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(dfl::outputs::constants::solverParFileName);
  outputPath.append(dfl::outputs::constants::solverParFileName);
  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
