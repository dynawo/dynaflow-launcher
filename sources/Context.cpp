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
 * @file  Context.cpp
 *
 * @brief Dynaflow launcher context implementation file
 *
 */

#include "Context.h"

#include "Algo.h"
#include "Constants.h"
#include "Contingencies.h"
#include "Diagram.h"
#include "Dyd.h"
#include "DydEvent.h"
#include "Job.h"
#include "Log.h"
#include "Message.hpp"
#include "Par.h"
#include "ParEvent.h"

#include <DYNMultipleJobsFactory.h>
#include <DYNScenario.h>
#include <DYNScenarios.h>
#include <DYNSimulation.h>
#include <DYNSimulationContext.h>
#include <DYNSystematicAnalysisLauncher.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <tuple>

namespace file = boost::filesystem;

namespace dfl {
Context::Context(const ContextDef& def, const inputs::Configuration& config) :
    def_(def),
    networkManager_(def.networkFilepath),
    dynamicDataBaseManager_(def.settingFilePath, def.assemblingFilePath),
    contingenciesManager_(def.contingenciesFilePath),
    config_(config),
    basename_{},
    slackNode_{},
    slackNodeOrigin_{SlackNodeOrigin::ALGORITHM},
    generators_{},
    loads_{},
    jobEntry_{},
    jobsEvents_{} {
  file::path path(def.networkFilepath);
  basename_ = path.filename().replace_extension().generic_string();

  auto found_slack_node = networkManager_.getSlackNode();
  if (found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
    slackNode_ = *found_slack_node;
    slackNodeOrigin_ = SlackNodeOrigin::FILE;
  } else {
    // slack node not given in iidm or not requested: it is computed internally
    slackNodeOrigin_ = SlackNodeOrigin::ALGORITHM;
    if (!found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
      // case slack node is requested to be extracted from IIDM but is not present in IIDM: we will compute it internally but a warning is sent
      LOG(warn) << MESS(NetworkSlackNodeNotFound, def.networkFilepath) << LOG_ENDL;
    }
    networkManager_.onNode(algo::SlackNodeAlgorithm(slackNode_));
  }

  networkManager_.onNode(algo::MainConnexComponentAlgorithm(mainConnexNodes_));
  networkManager_.onNode(algo::DynModelAlgorithm(dynamicModels_, dynamicDataBaseManager_));
  networkManager_.onNode(algo::ShuntCounterAlgorithm(counters_));
  networkManager_.onNode(algo::LinesByIdAlgorithm(linesById_));
}

bool
Context::checkConnexity() const {
  // The slack node must be in the main connex component
  return std::find_if(mainConnexNodes_.begin(), mainConnexNodes_.end(),
                      [this](const std::shared_ptr<inputs::Node>& node) { return node->id == slackNode_->id; }) != mainConnexNodes_.end();
}

bool
Context::process() {
  // Process all algorithms on nodes
  networkManager_.walkNodes();

  // Check models generated with algorithm
  filterPartiallyConnectedDynamicModels();

  LOG(info) << MESS(SlackNode, slackNode_->id, static_cast<unsigned int>(slackNodeOrigin_)) << LOG_ENDL;

  if (!checkConnexity()) {
    if (slackNodeOrigin_ == SlackNodeOrigin::FILE) {
      LOG(error) << MESS(ConnexityError, slackNode_->id) << LOG_ENDL;
      return false;
    } else {
      LOG(warn) << MESS(ConnexityErrorReCompute, slackNode_->id) << LOG_ENDL;
      // Compute slack node only on main connex component
      slackNode_.reset();
      std::for_each(mainConnexNodes_.begin(), mainConnexNodes_.end(), algo::SlackNodeAlgorithm(slackNode_));

      // By construction, the new slack node is in the main connex component
      LOG(info) << MESS(SlackNode, slackNode_->id, static_cast<unsigned int>(slackNodeOrigin_)) << LOG_ENDL;
    }
  }

  onNodeOnMainConnexComponent(algo::GeneratorDefinitionAlgorithm(generators_, busesWithDynamicModel_, networkManager_.getMapBusGeneratorsBusId(),
                                                                 config_.useInfiniteReactiveLimits(), networkManager_.dataInterface()->getServiceManager()));
  onNodeOnMainConnexComponent(algo::LoadDefinitionAlgorithm(loads_, config_.getDsoVoltageLevel()));
  onNodeOnMainConnexComponent(algo::HVDCDefinitionAlgorithm(hvdcLineDefinitions_, config_.useInfiniteReactiveLimits(), networkManager_.computeVSCConverters(),
                                                            networkManager_.getMapBusVSCConvertersBusId(),
                                                            networkManager_.dataInterface()->getServiceManager()));
  onNodeOnMainConnexComponent(algo::StaticVarCompensatorAlgorithm(svarcsDefinitions_));
  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    const auto& contingencies = contingenciesManager_.get();
    if (!contingencies.empty()) {
      validContingencies_ = boost::make_optional(algo::ValidContingencies(contingencies));
      onNodeOnMainConnexComponent(algo::ContingencyValidationAlgorithm(*validContingencies_));
    }
  }
  walkNodesMain();

  if (generators_.empty()) {
    // no generator is regulating the voltage in the main connex component : do not simulate
    LOG(error) << MESS(NetworkHasNoRegulatingGenerator, def_.networkFilepath) << LOG_ENDL;
    return false;
  }
  if (validContingencies_) {
    validContingencies_->keepContingenciesWithAllElementsValid();
  }

  return true;
}

void
Context::filterPartiallyConnectedDynamicModels() {
  const auto& automatonsConfig = dynamicDataBaseManager_.assemblingDocument().dynamicAutomatons();
  for (const auto& automaton : automatonsConfig) {
    if (dynamicModels_.models.count(automaton.id) == 0) {
      continue;
    }

    const auto& modelDef = dynamicModels_.models.at(automaton.id);
    for (const auto& macroConnect : automaton.macroConnects) {
      auto found = std::find_if(
          modelDef.nodeConnections.begin(), modelDef.nodeConnections.end(),
          [&macroConnect](const algo::DynamicModelDefinition::MacroConnection& macroConnection) { return macroConnection.id == macroConnect.macroConnection; });
      if (found == modelDef.nodeConnections.end()) {
        LOG(debug) << "Dynamic model " << automaton.id << " is only partially connected to network so it is removed from exported models" << LOG_ENDL;
        dynamicModels_.models.erase(automaton.id);
        break;  // element doesn't exist any more, go to next automaton
      }
    }
  }
}

void
Context::exportOutputs() {
  LOG(info) << MESS(ExportInfo, basename_) << LOG_ENDL;

  // create output directory
  file::path outputDir(config_.outputDir());

  // Job
  exportOutputJob();

  // Dyd
  file::path dydOutput(config_.outputDir());
  dydOutput.append(basename_ + ".dyd");
  outputs::Dyd dydWriter(outputs::Dyd::DydDefinition(basename_, dydOutput.generic_string(), generators_, loads_, slackNode_, hvdcLineDefinitions_,
                                                     busesWithDynamicModel_, dynamicDataBaseManager_, dynamicModels_, svarcsDefinitions_));
  dydWriter.write();

  // Par
  // copy constants files
  for (auto& entry : boost::make_iterator_range(file::directory_iterator(def_.parFileDir))) {
    if (entry.path().extension() == ".par") {
      file::path dest(outputDir);
      dest.append(entry.path().filename().generic_string());
      file::copy_file(entry.path(), dest, file::copy_option::overwrite_if_exists);
    }
  }
  // create specific par
  file::path parOutput(config_.outputDir());
  parOutput.append(basename_ + ".par");
  outputs::Par parWriter(outputs::Par::ParDefinition(basename_, config_.outputDir(), parOutput, generators_, hvdcLineDefinitions_,
                                                     config_.getActivePowerCompensation(), busesWithDynamicModel_, dynamicDataBaseManager_, counters_,
                                                     dynamicModels_, linesById_, svarcsDefinitions_));
  parWriter.write();

  // Diagram
  file::path diagramDirectory(config_.outputDir());
  diagramDirectory.append(basename_ + outputs::constants::diagramDirectorySuffix);
  outputs::Diagram diagramWriter(outputs::Diagram::DiagramDefinition(basename_, diagramDirectory.generic_string(), generators_, hvdcLineDefinitions_));
  diagramWriter.write();

  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    exportOutputsContingencies();
  }
}

void
Context::exportOutputJob() {
  outputs::Job jobWriter(outputs::Job::JobDefinition(basename_, def_.dynawoLogLevel, config_));
  jobEntry_ = jobWriter.write();

  switch (def_.simulationKind) {
  case SimulationKind::SECURITY_ANALYSIS:
    // For security analysis always export the main jobs file, as dynawo-algorithms will need it
    outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath), config_.outputDir());
    break;
  default:
    // For the rest of calculations, only export the jobs file when in DEBUG mode
#if _DEBUG_
    outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath), config_.outputDir());
#endif
    break;
  }
}

void
Context::exportOutputsContingencies() {
  if (validContingencies_) {
    for (const auto& contingency : validContingencies_->get()) {
      exportOutputsContingency(contingency);
    }
  }
}

void
Context::exportOutputsContingency(const inputs::Contingency& contingency) {
  // Prepare a DYD, PAR and JOBS for every contingency
  // The DYD and PAR contain the definition of the events of the contingency

  // Basename of event-related DYD, PAR and JOBS files
  const auto& basenameEvent = basename_ + "-" + contingency.id;

  // Specific DYD for contingency
  file::path dydEvent(config_.outputDir());
  dydEvent.append(basenameEvent + ".dyd");
  outputs::DydEvent dydEventWriter(outputs::DydEvent::DydEventDefinition(basenameEvent, dydEvent.generic_string(), contingency));
  dydEventWriter.write();

  // Specific PAR for contingency
  file::path parEvent(config_.outputDir());
  parEvent.append(basenameEvent + ".par");
  outputs::ParEvent parEventWriter(outputs::ParEvent::ParEventDefinition(basenameEvent, parEvent.generic_string(), contingency, config_.getTimeOfEvent()));
  parEventWriter.write();

#if _DEBUG_
  // A JOBS file for every contingency is produced only in DEBUG mode
  outputs::Job jobEventWriter(outputs::Job::JobDefinition(basenameEvent, def_.dynawoLogLevel, config_, contingency.id, basename_));
  boost::shared_ptr<job::JobEntry> jobEvent = jobEventWriter.write();
  jobsEvents_.emplace_back(jobEvent);
  outputs::Job::exportJob(jobEvent, absolute(def_.networkFilepath), config_.outputDir());
#endif
}

void
Context::execute() {
  auto simu_context = boost::make_shared<DYN::SimulationContext>();
  simu_context->setResourcesDirectory(def_.dynawoResDir.generic_string());
  simu_context->setLocale(def_.locale);

  file::path inputPath(config_.outputDir());
  auto path = file::canonical(inputPath);
  simu_context->setInputDirectory(path.generic_string());
  simu_context->setWorkingDirectory(config_.outputDir().generic_string());

  switch (def_.simulationKind) {
  case SimulationKind::STEADY_STATE_CALCULATION: {
    // This shall be the last log performed before building simulation,
    // because simulation constructor will re-initialize traces for Dynawo
    // Since DFL traces are persistent, they can be re-used after simulation is performed outside this function
    LOG(info) << MESS(SimulateInfo, basename_) << LOG_ENDL;

    // For a power flow calculation it is ok to directly run here a single simulation
    auto simu = boost::make_shared<DYN::Simulation>(jobEntry_, simu_context, networkManager_.dataInterface());
    simu->init();
    simu->simulate();
    simu->terminate();
    simu->clean();
    break;
  }
  case SimulationKind::SECURITY_ANALYSIS:
    LOG(info) << MESS(SecurityAnalysisSimulationInfo, basename_, def_.contingenciesFilePath) << LOG_ENDL;
    executeSecurityAnalysis();
    break;
  }
}

void
Context::executeSecurityAnalysis() {
  // For security analysis we run multiple simulations using dynawo-algorithms
  // Create one scenario for the base case and one scenario for each contingency
  auto scenarios = boost::make_shared<DYNAlgorithms::Scenarios>();
  scenarios->setJobsFile(jobEntry_->getName() + ".jobs");
  auto baseCase = boost::make_shared<DYNAlgorithms::Scenario>();
  baseCase->setId("BaseCase");
  scenarios->addScenario(baseCase);
  if (validContingencies_) {
    for (const auto& contingencyRef : validContingencies_->get()) {
      auto scenario = boost::make_shared<DYNAlgorithms::Scenario>();
      scenario->setId(contingencyRef.get().id);
      scenario->setDydFile(basename_ + "-" + contingencyRef.get().id + ".dyd");
      scenarios->addScenario(scenario);
      LOG(info) << MESS(ContingencySimulationDefined, contingencyRef.get().id) << LOG_ENDL;
    }
  }
  // Use dynawo-algorithms Systematic Analysis Launcher to simulate all the scenarios
  auto multipleJobs = multipleJobs::MultipleJobsFactory::newInstance();
  multipleJobs->setScenarios(scenarios);
  auto saLauncher = boost::make_shared<DYNAlgorithms::SystematicAnalysisLauncher>();
  saLauncher->setMultipleJobs(multipleJobs);
  saLauncher->setOutputFile("sa.zip");
  saLauncher->setDirectory(config_.outputDir().generic_string());
  saLauncher->init();
  saLauncher->launch();
  // Aggregated results could be obtained requesting writeResults method of saLauncher
}

void
Context::walkNodesMain() const {
  for (const auto& node : mainConnexNodes_) {
    for (const auto& cbk : callbacksMainConnexComponent_) {
      cbk(node);
    }
  }
}

}  // namespace dfl
