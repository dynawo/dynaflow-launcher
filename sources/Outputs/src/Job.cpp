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
#include <JOBInitialStateEntryFactory.h>
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
#include <JOBTimelineEntry.h>
#include <boost/make_shared.hpp>
#include <fstream>
#include <xml/sax/formatter/AttributeList.h>
#include <xml/sax/formatter/Formatter.h>

namespace dfl {
namespace outputs {

const std::string Job::solverFilename_ = "solver.par";
const std::string Job::solverName_ = "dynawo_SolverSIM";
const std::string Job::solverParId_ = "SimplifiedSolver";

Job::Job(JobDefinition &&def) : def_{std::move(def)} {}

boost::shared_ptr<job::JobEntry> Job::write() const {
  auto job = job::JobEntryFactory::newInstance();
  job->setName(def_.filename);

  job->setSolverEntry(writeSolver());
  job->setModelerEntry(writeModeler());
  job->setSimulationEntry(writeSimulation());
  job->setOutputsEntry(writeOutputs());

  return job;
}

boost::shared_ptr<job::SolverEntry> Job::writeSolver() const {
  auto solver = job::SolverEntryFactory::newInstance();
  solver->setLib(solverName_);
  solver->setParametersFile(solverFilename_);
  solver->setParametersId(solverParId_);

  return solver;
}

boost::shared_ptr<job::ModelerEntry> Job::writeModeler() const {
  auto modeler = job::ModelerEntryFactory::newInstance();
  if (def_.contingencyId) {
    modeler->setCompileDir("outputs-" + def_.contingencyId.get() + "/compilation");
  } else {
    modeler->setCompileDir("outputs/compilation");
  }

  auto models = job::DynModelsEntryFactory::newInstance();
  models->setDydFile(def_.filename + ".dyd");
  modeler->addDynModelsEntry(models);
  if (def_.baseFilename) {
    auto modelsBase = job::DynModelsEntryFactory::newInstance();
    modelsBase->setDydFile(def_.baseFilename.get() + ".dyd");
    modeler->addDynModelsEntry(modelsBase);
  }

  if (!def_.configuration.startingDumpFilePath().empty()) {
    auto initialState = job::InitialStateEntryFactory::newInstance();
    initialState->setInitialStateFile(def_.configuration.startingDumpFilePath().generic_string());
    modeler->setInitialStateEntry(initialState);
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

boost::shared_ptr<job::SimulationEntry> Job::writeSimulation() const {
  auto simu = job::SimulationEntryFactory::newInstance();
  simu->setStartTime(def_.configuration.getStartTime());
  simu->setStopTime(def_.configuration.getStopTime());
  simu->setPrecision(def_.configuration.getPrecision().value_or(1e-4));

  return simu;
}

boost::shared_ptr<job::OutputsEntry> Job::writeOutputs() const {
  auto output = job::OutputsEntryFactory::newInstance();
  if (def_.contingencyId) {
    output->setOutputsDirectory("outputs-" + def_.contingencyId.get());
  } else {
    output->setOutputsDirectory("outputs");
  }

  auto log = job::LogsEntryFactory::newInstance();
  auto appender = job::AppenderEntryFactory::newInstance();
  appender->setTag("");
  appender->setFilePath("dynawo.log");
  appender->setLvlFilter(def_.dynawoLogLevel);
  log->addAppenderEntry(appender);

  output->setLogsEntry(log);

  auto final_state = job::FinalStateEntryFactory::newInstance();

  final_state->setExportIIDMFile(def_.configuration.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
  final_state->setExportDumpFile(def_.configuration.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::DUMPSTATE));
  output->addFinalStateEntry(final_state);

  if (def_.configuration.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS)) {
    auto constraints = boost::shared_ptr<job::ConstraintsEntry>(new job::ConstraintsEntry());
    constraints->setExportMode("XML");
    output->setConstraintsEntry(constraints);
  }

  if (def_.configuration.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ)) {
    auto lostEquipments = boost::shared_ptr<job::LostEquipmentsEntry>(new job::LostEquipmentsEntry());
    lostEquipments->setDumpLostEquipments(true);
    output->setLostEquipmentsEntry(lostEquipments);
  }

  if (def_.configuration.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE)) {
    auto timeline = boost::shared_ptr<job::TimelineEntry>(new job::TimelineEntry());
    timeline->setExportMode("XML");
    output->setTimelineEntry(timeline);
  }

  return output;
}

void Job::exportJob(const boost::shared_ptr<job::JobEntry> &jobEntry, const boost::filesystem::path &networkFileEntry,
                    const dfl::inputs::Configuration &config) {
  boost::filesystem::path path(config.outputDir());

  if (!boost::filesystem::is_directory(path)) {
    boost::filesystem::create_directories(path);
  }

  path.append(jobEntry->getName() + ".jobs");
  std::ofstream os(path.c_str(), std::ios::binary);

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
  attrs.add("iidmFile", networkFileEntry.generic_string());
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

  auto initialStateEntry = modeler->getInitialStateEntry();
  if (initialStateEntry && !initialStateEntry->getInitialStateFile().empty()) {
    attrs.add("file", initialStateEntry->getInitialStateFile());
    formatter->startElement("dyn", "initialState", attrs);
    attrs.clear();
    formatter->endElement();  // initialState
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
  attrs.add("precision", simu->getPrecision());
  formatter->startElement("dyn", "simulation", attrs);
  attrs.clear();
  formatter->endElement();  // simulation

  // outputs
  auto outputs = jobEntry->getOutputsEntry();
  attrs.add("directory", outputs->getOutputsDirectory());
  formatter->startElement("dyn", "outputs", attrs);
  attrs.clear();

  // final state

  auto constraints = outputs->getConstraintsEntry();
  if (constraints) {
    attrs.add("exportMode", constraints->getExportMode());
    formatter->startElement("dyn", "constraints", attrs);
    attrs.clear();
    formatter->endElement();
  }

  auto timeline = outputs->getTimelineEntry();
  if (timeline) {
    attrs.add("exportMode", timeline->getExportMode());
    formatter->startElement("dyn", "timeline", attrs);
    attrs.clear();
    formatter->endElement();
  }

  attrs.add("exportIIDMFile", config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
  attrs.add("exportDumpFile", config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::DUMPSTATE));
  formatter->startElement("dyn", "finalState", attrs);
  attrs.clear();
  formatter->endElement();  // finalState

  auto lostEquipments = outputs->getLostEquipmentsEntry();
  if (lostEquipments) {
    formatter->startElement("dyn", "lostEquipments", attrs);
    attrs.clear();
    formatter->endElement();
  }

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

  formatter->endElement();  // outputs

  formatter->endElement();  // job

  formatter->endElement();  // jobs
  formatter->endDocument();
}

}  // namespace outputs
}  // namespace dfl
