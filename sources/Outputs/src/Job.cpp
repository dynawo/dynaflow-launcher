//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  Job.cpp
 *
 * @brief Dynaflow launcher job exporter implementation file
 *
 */

#include "Job.h"

#include <JOBAppenderEntry.h>
#include <JOBAppenderEntryFactory.h>
#include <JOBDynModelsEntry.h>
#include <JOBDynModelsEntryFactory.h>
#include <JOBFinalStateEntry.h>
#include <JOBFinalStateEntryFactory.h>
#include <JOBJobEntry.h>
#include <JOBJobEntryFactory.h>
#include <JOBLogsEntry.h>
#include <JOBLogsEntryFactory.h>
#include <JOBModelerEntry.h>
#include <JOBModelerEntryFactory.h>
#include <JOBModelsDirEntry.h>
#include <JOBModelsDirEntryFactory.h>
#include <JOBNetworkEntry.h>
#include <JOBNetworkEntryFactory.h>
#include <JOBOutputsEntry.h>
#include <JOBOutputsEntryFactory.h>
#include <JOBSimulationEntry.h>
#include <JOBSimulationEntryFactory.h>
#include <JOBSolverEntry.h>
#include <JOBSolverEntryFactory.h>
#include <boost/make_shared.hpp>
#include <fstream>

namespace dfl {
namespace outputs {

const std::chrono::seconds Job::timeStart_{0};
const std::chrono::seconds Job::durationSimu_{100};
const std::string Job::solverFilename_ = "solver.par";
const std::string Job::solverName_ = "dynawo_SolverSIM";
const std::string Job::solverParId_ = "SimplifiedSolver";

Job::Job(JobDefinition&& def) : def_{std::forward<JobDefinition>(def)} {}

boost::shared_ptr<job::JobEntry>
Job::write() {
  auto job = job::JobEntryFactory::newInstance();
  job->setName(def_.filename);

  job->setSolverEntry(writeSolver());
  job->setModelerEntry(writeModeler());
  job->setSimulationEntry(writeSimulation());
  job->setOutputsEntry(writeOutputs());

  return job;
}

boost::shared_ptr<job::SolverEntry>
Job::writeSolver() {
  auto solver = job::SolverEntryFactory::newInstance();
  solver->setLib(solverName_);
  solver->setParametersFile(solverFilename_);
  solver->setParametersId(solverParId_);

  return solver;
}

boost::shared_ptr<job::ModelerEntry>
Job::writeModeler() {
  auto modeler = job::ModelerEntryFactory::newInstance();
  modeler->setCompileDir("outputs/compilation");

  auto models = job::DynModelsEntryFactory::newInstance();
  models->setDydFile(def_.filename + ".dyd");
  modeler->addDynModelsEntry(models);

  auto network = job::NetworkEntryFactory::newInstance();
  network->setIidmFile(def_.filename + ".iidm");
  network->setNetworkParFile("Network.par");
  network->setNetworkParId("Network");
  modeler->setNetworkEntry(network);

  auto premodels = job::ModelsDirEntryFactory::newInstance();
  premodels->setUseStandardModels(true);
  modeler->setPreCompiledModelsDirEntry(premodels);
  modeler->setModelicaModelsDirEntry(premodels);

  return modeler;
}

boost::shared_ptr<job::SimulationEntry>
Job::writeSimulation() {
  auto simu = job::SimulationEntryFactory::newInstance();
  simu->setStartTime(timeStart_.count());
  simu->setStopTime((timeStart_ + durationSimu_).count());

  return simu;
}

boost::shared_ptr<job::OutputsEntry>
Job::writeOutputs() {
  auto output = job::OutputsEntryFactory::newInstance();
  output->setOutputsDirectory("outputs");

  auto log = job::LogsEntryFactory::newInstance();
  auto appender = job::AppenderEntryFactory::newInstance();
  appender->setTag("");
  appender->setFilePath("dynawo.log");
  appender->setLvlFilter(def_.dynawoLogLevel);
  log->addAppenderEntry(appender);

  output->setLogsEntry(log);

  auto final_state = job::FinalStateEntryFactory::newInstance();
  final_state->setExportIIDMFile(true);
  final_state->setExportDumpFile(false);
  output->setFinalStateEntry(final_state);

  return output;
}

}  // namespace outputs
}  // namespace dfl
