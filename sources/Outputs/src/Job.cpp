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
#include <xml/sax/formatter/AttributeList.h>
#include <xml/sax/formatter/Formatter.h>

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
  if (def_.contingencyId.empty()) {
    modeler->setCompileDir("outputs/compilation");
  } else {
    modeler->setCompileDir("outputs-" + def_.contingencyId + "/compilation");
  }

  auto models = job::DynModelsEntryFactory::newInstance();
  models->setDydFile(def_.filename + ".dyd");
  modeler->addDynModelsEntry(models);
  if (!def_.contingencyId.empty()) {
    auto modelsBase = job::DynModelsEntryFactory::newInstance();
    modelsBase->setDydFile(def_.baseFilename + ".dyd");
    modeler->addDynModelsEntry(modelsBase);
  }

  auto network = job::NetworkEntryFactory::newInstance();
  network->setIidmFile("");  // not providing IIDM file here as data interface will be provided to simulation
  network->setNetworkParFile("Network.par");
  network->setNetworkParId("Network");
  modeler->setNetworkEntry(network);

  auto premodels = job::ModelsDirEntryFactory::newInstance();
  premodels->setUseStandardModels(useStandardModels_);
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
  if (def_.contingencyId.empty()) {
    output->setOutputsDirectory("outputs");
  } else {
    output->setOutputsDirectory("outputs-" + def_.contingencyId);
  }

  auto log = job::LogsEntryFactory::newInstance();
  auto appender = job::AppenderEntryFactory::newInstance();
  appender->setTag("");
  appender->setFilePath("dynawo.log");
  appender->setLvlFilter(def_.dynawoLogLevel);
  log->addAppenderEntry(appender);

  output->setLogsEntry(log);

  auto final_state = job::FinalStateEntryFactory::newInstance();
  final_state->setExportIIDMFile(exportIIDMFile_);
  final_state->setExportDumpFile(exportDumpFile_);
  output->setFinalStateEntry(final_state);

  return output;
}

void
Job::exportJob(const boost::shared_ptr<job::JobEntry>& jobEntry, const std::string& networkFileEntry, const std::string& outputDir) {
  boost::filesystem::path path(outputDir);

  if (!boost::filesystem::is_directory(path)) {
    boost::filesystem::create_directories(path);
  }

  path.append(jobEntry->getName() + ".jobs");
  std::ofstream os(path.c_str());

  auto formatter = xml::sax::formatter::Formatter::createFormatter(os);
  formatter->addNamespace("dyn", "http://www.rte-france.com/dynawo");

  formatter->startDocument();
  xml::sax::formatter::AttributeList attrs;

  formatter->startElement("dyn", "jobs", attrs);

  attrs.add("name", jobEntry->getName());
  formatter->startElement("dyn", "job", attrs);
  attrs.clear();

  // solver

  auto solver = jobEntry->getSolverEntry();
  attrs.add("lib", solver->getLib());
  attrs.add("parFile", solver->getParametersFile());
  attrs.add("parId", solver->getParametersId());
  formatter->startElement("dyn", "solver", attrs);
  attrs.clear();
  formatter->endElement();  // solver

  // modeler

  auto modeler = jobEntry->getModelerEntry();
  attrs.add("compileDir", modeler->getCompileDir());
  formatter->startElement("dyn", "modeler", attrs);
  attrs.clear();

  auto network = modeler->getNetworkEntry();
  attrs.add("iidmFile", networkFileEntry);
  attrs.add("parFile", network->getNetworkParFile());
  attrs.add("parId", network->getNetworkParId());
  formatter->startElement("dyn", "network", attrs);
  attrs.clear();
  formatter->endElement();  // network

  auto models = modeler->getDynModelsEntries();
  for (auto model : models) {
    attrs.add("dydFile", model->getDydFile());
    formatter->startElement("dyn", "dynModels", attrs);
    attrs.clear();
    formatter->endElement();  // model
  }

  auto pre_models = modeler->getPreCompiledModelsDirEntry();
  attrs.add("useStandardModels", pre_models->getUseStandardModels());
  formatter->startElement("dyn", "precompiledModels", attrs);
  attrs.clear();
  formatter->endElement();  // precompiledModels

  attrs.add("useStandardModels", useStandardModels_);
  formatter->startElement("dyn", "modelicaModels", attrs);
  attrs.clear();
  formatter->endElement();  // precompiledModels

  formatter->endElement();  // modeler

  // simu

  auto simu = jobEntry->getSimulationEntry();
  attrs.add("startTime", simu->getStartTime());
  attrs.add("stopTime", simu->getStopTime());
  formatter->startElement("dyn", "simulation", attrs);
  attrs.clear();
  formatter->endElement();  // simulation

  // outputs
  auto outputs = jobEntry->getOutputsEntry();
  attrs.add("directory", outputs->getOutputsDirectory());
  formatter->startElement("dyn", "outputs", attrs);
  attrs.clear();

  auto logs = outputs->getLogsEntry();
  formatter->startElement("dyn", "logs");
  auto appenders = logs->getAppenderEntries();
  for (auto appender : appenders) {
    attrs.add("tag", appender->getTag());
    attrs.add("file", appender->getFilePath());
    attrs.add("lvlFilter", appender->getLvlFilter());
    formatter->startElement("dyn", "appender", attrs);
    attrs.clear();
    formatter->endElement();  // appender
  }
  formatter->endElement();  // logs

  // final state

  auto finalState = outputs->getFinalStateEntry();
  attrs.add("exportIIDMFile", exportIIDMFile_);
  attrs.add("exportDumpFile", exportDumpFile_);
  formatter->startElement("dyn", "finalState", attrs);
  attrs.clear();
  formatter->endElement();  // finalState

  formatter->endElement();  // outputs

  formatter->endElement();  // job

  formatter->endElement();  // jobs
  formatter->endDocument();
}

}  // namespace outputs
}  // namespace dfl
