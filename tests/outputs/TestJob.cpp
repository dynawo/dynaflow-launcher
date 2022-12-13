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
#include "Log.h"
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
  dfl::inputs::Configuration::SimulationKind simulationKindsArray[2] = {dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION,
                                                                        dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS};
  for (dfl::inputs::Configuration::SimulationKind simulationKind : simulationKindsArray) {
    dfl::inputs::Configuration config("res/config_jobs.json", simulationKind);
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
    ASSERT_TRUE(premodels->getUseStandardModels());
    ASSERT_EQ(0, premodels->getDirectories().size());
    ASSERT_EQ("", premodels->getModelExtension());
    premodels = modelerEntry->getModelicaModelsDirEntry();
    ASSERT_TRUE(premodels->getUseStandardModels());
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
    ASSERT_TRUE(outputs->getTimelineEntry()->getExportWithTime());
    ASSERT_NE(nullptr, outputs->getConstraintsEntry());
    ASSERT_EQ("XML", outputs->getConstraintsEntry()->getExportMode());
    ASSERT_EQ("", outputs->getConstraintsEntry()->getOutputFile());
    ASSERT_NE(nullptr, outputs->getLostEquipmentsEntry());
    ASSERT_TRUE(outputs->getLostEquipmentsEntry()->getDumpLostEquipments());
#else

    switch (simulationKind) {
    case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION:
      ASSERT_EQ(nullptr, outputs->getConstraintsEntry());
      ASSERT_EQ(nullptr, outputs->getLostEquipmentsEntry());
      break;
    case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
      ASSERT_NE(nullptr, outputs->getConstraintsEntry());
      ASSERT_NE(nullptr, outputs->getLostEquipmentsEntry());

      ASSERT_EQ("XML", outputs->getConstraintsEntry()->getExportMode());
      ASSERT_EQ("", outputs->getConstraintsEntry()->getOutputFile());
      ASSERT_NE(nullptr, outputs->getLostEquipmentsEntry());
      ASSERT_TRUE(outputs->getLostEquipmentsEntry()->getDumpLostEquipments());
      break;
    }

    ASSERT_EQ(nullptr, outputs->getTimelineEntry());
#endif

    ASSERT_EQ(nullptr, outputs->getInitValuesEntry());
    auto appenders = outputs->getLogsEntry()->getAppenderEntries();
    ASSERT_EQ(1, appenders.size());

    auto appender_it = appenders.begin();
    auto& appender = *appender_it;
    ASSERT_EQ("INFO", appender->getLvlFilter());
    ASSERT_EQ("", appender->getTag());
    ASSERT_EQ(" | ", appender->getSeparator());
    ASSERT_TRUE(appender->getShowLevelTag());
    ASSERT_EQ("dynawo.log", appender->getFilePath());

    auto finalstates = outputs->getFinalStateEntries();
#if _DEBUG_
    ASSERT_EQ(finalstates.size(), 1);
    ASSERT_TRUE(finalstates.front()->getExportIIDMFile());
    ASSERT_FALSE(finalstates.front()->getExportDumpFile());
#else
    switch (simulationKind) {
    case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION:
      ASSERT_EQ(finalstates.size(), 1);
      ASSERT_TRUE(finalstates.front()->getExportIIDMFile());
      ASSERT_FALSE(finalstates.front()->getExportDumpFile());
      break;
    case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
      ASSERT_FALSE(finalstates.front()->getExportIIDMFile());
      break;
    }
#endif

    std::string basename = "TestJob";
    std::string filename = basename + ".jobs";
    std::string filename_reference;
#if _DEBUG_
    filename_reference = basename + "_debug.jobs";
#else
    switch (simulationKind) {
    case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION:
      filename_reference = basename + "_release.jobs";
      break;
    case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
      filename_reference = basename + "_release_sa.jobs";
      break;
    }
#endif
    boost::filesystem::path outputPath(outputPathResults);
    outputPath.append(basename);
    if (!boost::filesystem::exists(outputPath)) {
      boost::filesystem::create_directories(outputPath);
    }
    job.exportJob(jobEntry, "TestIIDM.iidm", config);

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

  job.exportJob(jobEntry, "TestIIDM.iidm", config);

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

TEST(Job, ChosenOutputs) {
  /*
  To check which field should be displayed we use four masks :
  STEADYSTATE corresponds to 1000
  CONSTRAINTS corresponds to 0100
  TIMELINE corresponds to 0010
  LOSTEQ corresponds to 0001
  */

  dfl::inputs::Configuration::SimulationKind simulationKindsArray[2] = {dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION,
                                                                        dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS};
  for (dfl::inputs::Configuration::SimulationKind simulationKind : simulationKindsArray) {
    std::map<const std::string, const int> configFileToChosenOutputMap;
#if _DEBUG_
    configFileToChosenOutputMap = {
        {"res/config_empty.json", 0b1111}, {"res/config_only_lost_equipments_chosen.json", 0b1111}, {"res/config_all_chosen_outputs.json", 0b1111}};
#else
    switch (simulationKind) {
    case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION:
      configFileToChosenOutputMap = {
          {"res/config_empty.json", 0b1000}, {"res/config_only_lost_equipments_chosen.json", 0b1001}, {"res/config_all_chosen_outputs.json", 0b1111}};
      break;
    case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
      configFileToChosenOutputMap = {
          {"res/config_empty.json", 0b0101}, {"res/config_only_timeline_chosen.json", 0b0111}, {"res/config_all_chosen_outputs.json", 0b1111}};
      break;
    }
#endif

    ASSERT_FALSE(configFileToChosenOutputMap.empty());

    for (std::pair<const std::string, const int> chosenOutput : configFileToChosenOutputMap) {
      const boost::filesystem::path chosenOutputsPath = static_cast<boost::filesystem::path>(chosenOutput.first);
      dfl::inputs::Configuration config(chosenOutputsPath, simulationKind);
      dfl::outputs::Job job(dfl::outputs::Job::JobDefinition("TestJobChosenOutputs", "INFO", config));
      boost::shared_ptr<job::JobEntry> jobEntry = job.write();
      const boost::shared_ptr<job::OutputsEntry>& outputsEntry = jobEntry->getOutputsEntry();
      const std::vector<boost::shared_ptr<job::FinalStateEntry>>& finalStateEntry = outputsEntry->getFinalStateEntries();
      const boost::shared_ptr<job::ConstraintsEntry>& constraintsEntry = outputsEntry->getConstraintsEntry();
      const boost::shared_ptr<job::TimelineEntry>& timelineEntry = outputsEntry->getTimelineEntry();
      const boost::shared_ptr<job::LostEquipmentsEntry>& lostEquipmentsEntry = outputsEntry->getLostEquipmentsEntry();

      if (chosenOutput.second & 0b1000) {
        ASSERT_TRUE(finalStateEntry.front()->getExportIIDMFile());
      } else {
        ASSERT_FALSE(finalStateEntry.front()->getExportIIDMFile());
      }

      if (chosenOutput.second & 0b0100) {
        ASSERT_NE(constraintsEntry, nullptr);
      } else {
        ASSERT_EQ(constraintsEntry, nullptr);
      }

      if (chosenOutput.second & 0b0010) {
        ASSERT_NE(timelineEntry, nullptr);
      } else {
        ASSERT_EQ(timelineEntry, nullptr);
      }

      if (chosenOutput.second & 0b0001) {
        ASSERT_NE(lostEquipmentsEntry, nullptr);
      } else {
        ASSERT_EQ(lostEquipmentsEntry, nullptr);
      }
    }
  }
}
