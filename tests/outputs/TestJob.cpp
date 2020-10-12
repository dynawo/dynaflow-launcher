//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Job.h"
#include "Tests.h"

#include <JOBAppenderEntry.h>
#include <JOBDynModelsEntry.h>
#include <JOBFinalStateEntry.h>
#include <JOBLogsEntry.h>
#include <JOBModelerEntry.h>
#include <JOBModelsDirEntry.h>
#include <JOBNetworkEntry.h>
#include <JOBOutputsEntry.h>
#include <JOBSimulationEntry.h>
#include <JOBSolverEntry.h>

TEST(Job, write) {
  dfl::outputs::Job job(dfl::outputs::Job::JobDefinition("TestJob", "INFO"));

  auto jobEntry = job.write();

  ASSERT_EQ("TestJob", jobEntry->getName());

  auto solverEntry = jobEntry->getSolverEntry();
  ASSERT_EQ("dynawo_SolverSIM", solverEntry->getLib());
  ASSERT_EQ("solver.par", solverEntry->getParametersFile());
  ASSERT_EQ("SimplifiedSolver", solverEntry->getParametersId());

  auto modelerEntry = jobEntry->getModelerEntry();
  ASSERT_EQ("outputs/compilation", modelerEntry->getCompileDir());
  auto network = modelerEntry->getNetworkEntry();
  ASSERT_EQ("TestJob.iidm", network->getIidmFile());
  ASSERT_EQ("Network.par", network->getNetworkParFile());
  ASSERT_EQ("Network", network->getNetworkParId());
  auto models = modelerEntry->getDynModelsEntries();
  ASSERT_EQ(1, models.size());
  ASSERT_EQ("TestJob.dyd", (*models.begin())->getDydFile());
  auto premodels = modelerEntry->getPreCompiledModelsDirEntry();
  ASSERT_EQ(true, premodels->getUseStandardModels());
  ASSERT_EQ(0, premodels->getDirectories().size());
  ASSERT_EQ("", premodels->getModelExtension());
  premodels = modelerEntry->getModelicaModelsDirEntry();
  ASSERT_EQ(true, premodels->getUseStandardModels());
  ASSERT_EQ(0, premodels->getDirectories().size());
  ASSERT_EQ("", premodels->getModelExtension());

  auto simulationEntry = jobEntry->getSimulationEntry();
  ASSERT_EQ(0, simulationEntry->getStartTime());
  ASSERT_EQ(100, simulationEntry->getStopTime());

  auto outputs = jobEntry->getOutputsEntry();
  ASSERT_EQ("outputs", outputs->getOutputsDirectory());
  ASSERT_EQ(nullptr, outputs->getTimelineEntry());
  ASSERT_EQ(nullptr, outputs->getInitValuesEntry());
  auto appenders = outputs->getLogsEntry()->getAppenderEntries();
  ASSERT_EQ(1, appenders.size());

  auto appender_it = appenders.begin();
  auto& appender = *appender_it;
  ASSERT_EQ("INFO", appender->getLvlFilter());
  ASSERT_EQ("", appender->getTag());
  ASSERT_EQ(" | ", appender->getSeparator());
  ASSERT_EQ(true, appender->getShowLevelTag());
  ASSERT_EQ("dynawo.log", appender->getFilePath());

  auto finalstate = outputs->getFinalStateEntry();
  ASSERT_EQ(true, finalstate->getExportIIDMFile());
  ASSERT_EQ(false, finalstate->getExportDumpFile());
}
