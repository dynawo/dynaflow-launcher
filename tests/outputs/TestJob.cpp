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

#include <DYNExecUtils.h>
#include <DYNFileSystemUtils.h>
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
  dfl::inputs::Configuration config("res/config_empty.json");
  dfl::outputs::Job job(dfl::outputs::Job::JobDefinition("TestJob", "INFO", config));

  auto jobEntry = job.write();

  ASSERT_EQ("TestJob", jobEntry->getName());

  auto solverEntry = jobEntry->getSolverEntry();
  ASSERT_EQ("dynawo_SolverSIM", solverEntry->getLib());
  ASSERT_EQ("solver.par", solverEntry->getParametersFile());
  ASSERT_EQ("SimplifiedSolver", solverEntry->getParametersId());

  auto modelerEntry = jobEntry->getModelerEntry();
  ASSERT_EQ("outputs/compilation", modelerEntry->getCompileDir());
  auto network = modelerEntry->getNetworkEntry();
  ASSERT_EQ("", network->getIidmFile());
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
#if _DEBUG_
  // On debug we always ask for a timeline and a constraints output
  ASSERT_NE(nullptr, outputs->getTimelineEntry());
  ASSERT_EQ("TXT", outputs->getTimelineEntry()->getExportMode());
  ASSERT_EQ("", outputs->getTimelineEntry()->getOutputFile());
  ASSERT_EQ(true, outputs->getTimelineEntry()->getExportWithTime());
  ASSERT_NE(nullptr, outputs->getConstraintsEntry());
  ASSERT_EQ("XML", outputs->getConstraintsEntry()->getExportMode());
  ASSERT_EQ("", outputs->getConstraintsEntry()->getOutputFile());
#else
  ASSERT_EQ(nullptr, outputs->getTimelineEntry());
  ASSERT_EQ(nullptr, outputs->getConstraintsEntry());
#endif
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

  auto finalstates = outputs->getFinalStateEntries();
  ASSERT_EQ(finalstates.size(), 1);
  ASSERT_EQ(true, finalstates.front()->getExportIIDMFile());
  ASSERT_EQ(false, finalstates.front()->getExportDumpFile());

  std::string basename = "TestJob";
  std::string filename = basename + ".jobs";
  std::string filename_reference;
#if _DEBUG_
  filename_reference = basename + "_debug.jobs";
#else
  filename_reference = basename + "_release.jobs";
#endif
  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);
  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }
  job.exportJob(jobEntry, "TestIIDM.iidm", outputPath);

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename_reference);
  outputPath.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
  ASSERT_TRUE(hasEnvVar("DYNAWO_HOME"));
  std::string xsd_file = getEnvVar("DYNAWO_HOME") + "/share/xsd/jobs.xsd";
  ASSERT_TRUE(exists(xsd_file));
  std::stringstream ssVal;
  std::string command = "xmllint --schema " + xsd_file + " " + outputPath.generic_string() + " --noout";
  executeCommand(command, ssVal);
  std::string commandRes = "Executing command : " + command + "\n" + outputPath.generic_string() + " validates\n";
  ASSERT_EQ(ssVal.str(), commandRes);
}

TEST(Job, writePrecision) {
  const std::string basename = "TestJobPrecision";

  dfl::inputs::Configuration config("res/config.json");
  dfl::outputs::Job job(dfl::outputs::Job::JobDefinition(basename, "INFO", config));

  auto jobEntry = job.write();

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  boost::filesystem::path reference("reference");
  reference.append(basename);

  job.exportJob(jobEntry, "TestIIDM.iidm", outputPath);

  const std::string filename_output = basename + ".jobs";
  outputPath.append(filename_output);

  std::string filename_reference;
#if _DEBUG_
  filename_reference = basename + "_debug.jobs";
#else
  filename_reference = basename + "_release.jobs";
#endif
  reference.append(filename_reference);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
