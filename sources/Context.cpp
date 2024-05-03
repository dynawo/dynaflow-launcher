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

#include "Constants.h"
#include "Contingencies.h"
#include "Diagram.h"
#include "Dyd.h"
#include "DydEvent.h"
#include "DynModelFilterAlgorithm.h"
#include "Job.h"
#include "Log.h"
#include "Network.h"
#include "Par.h"
#include "ParEvent.h"
#include "Solver.h"

#include <DYNMultiProcessingContext.h>
#include <DYNMultipleJobsFactory.h>
#include <DYNScenario.h>
#include <DYNScenarios.h>
#include <DYNSimulation.h>
#include <DYNSimulationContext.h>
#include <DYNSystematicAnalysisLauncher.h>
#include <DYNTimer.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <tuple>

namespace file = boost::filesystem;

namespace dfl {
Context::Context(const ContextDef &def, inputs::Configuration &config)
    : def_(def), networkManager_(def.networkFilepath), dynamicDataBaseManager_(def.settingFilePath, def.assemblingFilePath),
      contingenciesManager_(def.contingenciesFilePath),
      config_(config), basename_{}, slackNode_{}, slackNodeOrigin_{SlackNodeOrigin::ALGORITHM}, generators_{}, loads_{}, staticVarCompensators_{},
      algoResults_(new algo::AlgorithmsResults()), jobEntry_{}, jobsEvents_{} {
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
      LOG(warn, NetworkSlackNodeNotFound, def.networkFilepath);
    }
    networkManager_.onNode(algo::SlackNodeAlgorithm(slackNode_));
  }

  networkManager_.onNode(algo::MainConnexComponentAlgorithm(mainConnexNodes_));
  if (config_.isShuntRegulationOn()) {
    networkManager_.onNode(algo::ShuntCounterAlgorithm(counters_));
  }
  networkManager_.onNode(algo::LinesByIdAlgorithm(linesById_));
  networkManager_.onNode(algo::TransformersByIdAlgorithm(tfosById_));

  if (dynamicDataBaseAssemblingContainsSVC()) {
    if (!config_.defaultValueModified("StopTime"))
      config_.setStopTime(config_.getStartTime() + 2000);
    if (!config_.defaultValueModified("TimeOfEvent"))
      config_.setTimeOfEvent(config_.getStartTime() + 500);
  }
}

bool Context::checkConnexity() const {
  // The slack node must be in the main connex component
  return std::find_if(mainConnexNodes_.begin(), mainConnexNodes_.end(),
                      [this](const std::shared_ptr<inputs::Node> &node) { return node->id == slackNode_->id; }) != mainConnexNodes_.end();
}

bool Context::process() {
#if defined(_DEBUG_) || defined(PRINT_TIMERS)
  DYN::Timer timer("DFL::Context::process()");
#endif
  // Process all algorithms on nodes
  networkManager_.walkNodes();

  if (!slackNode_) {
    throw Error(SlackNodeNotFound, basename_);
  }
  LOG(info, SlackNode, slackNode_->id, static_cast<unsigned int>(slackNodeOrigin_));

  if (!checkConnexity()) {
    if (slackNodeOrigin_ == SlackNodeOrigin::FILE) {
      throw Error(ConnexityError, slackNode_->id);
    } else {
      LOG(warn, ConnexityErrorReCompute, slackNode_->id);
      // Compute slack node only on main connex component
      slackNode_.reset();
      std::for_each(mainConnexNodes_.begin(), mainConnexNodes_.end(), algo::SlackNodeAlgorithm(slackNode_));

      // By construction, the new slack node is in the main connex component
      LOG(info, SlackNode, slackNode_->id, static_cast<unsigned int>(slackNodeOrigin_));
    }
  }

  onNodeOnMainConnexComponent(algo::GeneratorDefinitionAlgorithm(generators_, networkManager_.getBusRegulationMap(), dynamicDataBaseManager_,
                                                                 config_.useInfiniteReactiveLimits(), config_.getTfoVoltageLevel()));
  onNodeOnMainConnexComponent(algo::LoadDefinitionAlgorithm(loads_, config_.getDsoVoltageLevel()));
  onNodeOnMainConnexComponent(algo::HVDCDefinitionAlgorithm(hvdcLineDefinitions_, networkManager_.getBusRegulationMap(), config_.useInfiniteReactiveLimits(),
                                                            networkManager_.computeVSCConverters(), dynamicDataBaseManager_));
  onNodeOnMainConnexComponent(algo::DynModelAlgorithm(dynamicModels_, dynamicDataBaseManager_, config_.isShuntRegulationOn()));

  if (config_.isSVarCRegulationOn()) {
    onNodeOnMainConnexComponent(algo::StaticVarCompensatorAlgorithm(staticVarCompensators_));
  }

  if (def_.simulationKind == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS) {
    const auto &contingencies = contingenciesManager_.get();
    if (!contingencies.empty()) {
      validContingencies_ = boost::make_optional(algo::ValidContingencies(contingencies));
      onNodeOnMainConnexComponent(algo::ContingencyValidationAlgorithmOnNodes(*validContingencies_));
    }
  }
  walkNodesMain();

  algo::DynModelFilterAlgorithm dynModelFilterAlgorithm(dynamicDataBaseManager_.assembling(), generators_, dynamicModels_.models);
  dynModelFilterAlgorithm.filter();

  // the validation of contingencies on algorithm definitions must be done after walking all nodes
  // on main topological island, because this is where we fill the algorithms definitions
  if (def_.simulationKind == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS && validContingencies_) {
    auto contValOnDefs = algo::ContingencyValidationAlgorithmOnDefs(*validContingencies_);
    contValOnDefs.fillValidContingenciesOnDefs(loads_, generators_, staticVarCompensators_);
  }

  if (!algoResults_->isAtLeastOneGeneratorRegulating) {
    // no generator is regulating the voltage in the main connex component : do not simulate
    throw Error(NetworkHasNoRegulatingGenerator, def_.networkFilepath);
  }
  if (validContingencies_) {
    validContingencies_->keepContingenciesWithAllElementsValid();
  }

  return true;
}

void Context::exportOutputs() {
#if defined(_DEBUG_) || defined(PRINT_TIMERS)
  DYN::Timer timer("DFL::Context::exportOutputs()");
#endif
  LOG(info, ExportInfo, basename_);

  // create output directory
  file::path outputDir(config_.outputDir());

  // Job
  exportOutputJob();

  // Only the root process is allowed to export files
  auto &mpiContext = DYNAlgorithms::multiprocessing::context();
  if (!mpiContext.isRootProc())
    return;

  // Dyd
  file::path dydOutput(config_.outputDir());
  dydOutput.append(basename_ + ".dyd");
  outputs::Dyd dydWriter(outputs::Dyd::DydDefinition(basename_, dydOutput.generic_string(), generators_, loads_, slackNode_, hvdcLineDefinitions_,
                                                     networkManager_.getBusRegulationMap(), dynamicDataBaseManager_, dynamicModels_, staticVarCompensators_));
  dydWriter.write();

  // create Network.par
  file::path networkOutput(config_.outputDir());
  networkOutput.append("Network.par");
  outputs::Network networkWriter(outputs::Network::NetworkDefinition(networkOutput, config_.getStartingPointMode()));
  networkWriter.write();

  // create specific par
  file::path parOutput(config_.outputDir());
  parOutput.append(basename_ + ".par");
  outputs::Par parWriter(outputs::Par::ParDefinition(basename_, config_, parOutput, generators_, hvdcLineDefinitions_, networkManager_.getBusRegulationMap(),
                                                     dynamicDataBaseManager_, counters_, dynamicModels_, linesById_, tfosById_, staticVarCompensators_,
                                                     loads_));
  parWriter.write();

  // Diagram
  file::path diagramDirectory(config_.outputDir());
  diagramDirectory.append(basename_ + common::constants::diagramDirectorySuffix);
  outputs::Diagram diagramWriter(outputs::Diagram::DiagramDefinition(basename_, diagramDirectory.generic_string(), generators_, hvdcLineDefinitions_));
  diagramWriter.write();

  outputs::Solver solverWriter{dfl::outputs::Solver::SolverDefinition(config_)};
  solverWriter.write();

  if (def_.simulationKind == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS) {
    exportOutputsContingencies();
  }
}

void Context::exportOutputJob() {
  outputs::Job jobWriter(outputs::Job::JobDefinition(basename_, def_.dynawoLogLevel, config_));
  jobEntry_ = jobWriter.write();

  auto &mpiContext = DYNAlgorithms::multiprocessing::context();
  // Only the root process is allowed to export files
  if (!mpiContext.isRootProc())
    return;

  switch (def_.simulationKind) {
  case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS: {
    // For security analysis always export the main jobs file, as dynawo-algorithms will need it
    outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath), config_);
    break;
  }
  default:
    // For the rest of calculations, only export the jobs file when in DEBUG mode
#if _DEBUG_
    outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath), config_);
#endif
    break;
  }
}

void Context::exportOutputsContingencies() {
  if (validContingencies_) {
    for (const auto &contingency : validContingencies_->get()) {
      exportOutputsContingency(contingency, validContingencies_->getNetworkElements());
    }
  }
}

void Context::exportOutputsContingency(const inputs::Contingency &contingency, const std::unordered_set<std::string> &networkElements) {
  // Prepare a DYD, PAR and JOBS for every contingency
  // The DYD and PAR contain the definition of the events of the contingency

  // Basename of event-related DYD, PAR and JOBS files
  const auto &basenameEvent = basename_ + "-" + contingency.id;

  // Specific DYD for contingency
  file::path dydEvent(config_.outputDir());
  dydEvent.append(basenameEvent + ".dyd");
  outputs::DydEvent dydEventWriter(outputs::DydEvent::DydEventDefinition(basenameEvent, dydEvent.generic_string(), contingency, networkElements));
  dydEventWriter.write();

  // Specific PAR for contingency
  file::path parEvent(config_.outputDir());
  parEvent.append(basenameEvent + ".par");
  outputs::ParEvent parEventWriter(
      outputs::ParEvent::ParEventDefinition(basenameEvent, parEvent.generic_string(), contingency, networkElements, config_.getTimeOfEvent()));
  parEventWriter.write();

#if _DEBUG_
  // A JOBS file for every contingency is produced only in DEBUG mode
  outputs::Job jobEventWriter(outputs::Job::JobDefinition(basenameEvent, def_.dynawoLogLevel, config_, contingency.id, basename_));
  boost::shared_ptr<job::JobEntry> jobEvent = jobEventWriter.write();
  jobsEvents_.emplace_back(jobEvent);
  outputs::Job::exportJob(jobEvent, absolute(def_.networkFilepath), config_);
#endif
}

void Context::execute() {
#if defined(_DEBUG_) || defined(PRINT_TIMERS)
  DYN::Timer timer("DFL::Context::execute()");
#endif
  auto simu_context = boost::make_shared<DYN::SimulationContext>();
  simu_context->setResourcesDirectory(def_.dynawoResDir.generic_string());
  simu_context->setLocale(def_.locale);

  file::path inputPath(config_.outputDir());
  auto path = file::canonical(inputPath);
  simu_context->setInputDirectory(path.generic_string());
  simu_context->setWorkingDirectory(config_.outputDir().generic_string());

  switch (def_.simulationKind) {
  case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION: {
    // This shall be the last log performed before building simulation,
    // because simulation constructor will re-initialize traces for Dynawo
    // Since DFL traces are persistent, they can be re-used after simulation is performed outside this function
    LOG(info, SimulateInfo, basename_);

    // For a power flow calculation it is ok to directly run here a single simulation
    auto simu = boost::make_shared<DYN::Simulation>(jobEntry_, simu_context, networkManager_.dataInterface());
    simu->init();
    try {
      simu->simulate();
    } catch (const DYN::Error &err) {
      // Needed as otherwise terminate might crash due to missing staticRef variables
      if (err.key() == DYN::KeyError_t::StateVariableNoReference) {
        simu->disableExportIIDM();
        simu->setLostEquipmentsExportMode(DYN::Simulation::EXPORT_LOSTEQUIPMENTS_NONE);
      }
      simu->terminate();
      throw;
    } catch (const DYN::Terminate &) {
      simu->terminate();
      throw;
    } catch (const DYN::MessageError &) {
      simu->terminate();
      throw;
    } catch (const char *) {
      simu->terminate();
      throw;
    } catch (const std::string &) {
      simu->terminate();
      throw;
    } catch (const std::exception &) {
      simu->terminate();
      throw;
    }
    simu->terminate();
    simu->clean();
    break;
  }
  case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS: {
    LOG(info, SecurityAnalysisSimulationInfo, basename_, def_.contingenciesFilePath);
    executeSecurityAnalysis();
    break;
  }
  }
}

void Context::executeSecurityAnalysis() {
  // For security analysis we run multiple simulations using dynawo-algorithms
  // Create one scenario for the base case and one scenario for each contingency
  auto scenarios = boost::make_shared<DYNAlgorithms::Scenarios>();
  scenarios->setJobsFile(jobEntry_->getName() + ".jobs");
  if (validContingencies_) {
    for (const auto &contingencyRef : validContingencies_->get()) {
      auto scenario = boost::make_shared<DYNAlgorithms::Scenario>();
      scenario->setId(contingencyRef.get().id);
      scenario->setDydFile(basename_ + "-" + contingencyRef.get().id + ".dyd");
      scenarios->addScenario(scenario);
      LOG(info, ContingencySimulationDefined, contingencyRef.get().id);
    }
  }
  // Use dynawo-algorithms Systematic Analysis Launcher to simulate all the scenarios
  auto multipleJobs = multipleJobs::MultipleJobsFactory::newInstance();
  multipleJobs->setScenarios(scenarios);
  auto saLauncher = boost::make_shared<DYNAlgorithms::SystematicAnalysisLauncher>();
  saLauncher->setMultipleJobs(multipleJobs);
  saLauncher->setOutputFile("aggregatedResults.xml");
  saLauncher->setDirectory(config_.outputDir().generic_string());
  saLauncher->init();
  saLauncher->launch();
  saLauncher->writeResults();
}

void Context::exportResults(bool simulationOk) {
  switch (def_.simulationKind) {
  case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION: {
    boost::property_tree::ptree resultsTree;
    boost::property_tree::ptree componentResultsTree;
    boost::property_tree::ptree componentResultsChild;
    resultsTree.put("version", "1.2");
    resultsTree.put("isOK", simulationOk);
    resultsTree.put("metrics.useInfiniteReactiveLimits", config_.useInfiniteReactiveLimits());
    resultsTree.put("metrics.isSVCRegulationOn", config_.isSVarCRegulationOn());
    resultsTree.put("metrics.isShuntRegulationOn", config_.isShuntRegulationOn());
    resultsTree.put("metrics.isAutomaticSlackBusOn", config_.isAutomaticSlackBusOn());
    componentResultsChild.put("connectedComponentNum", 0);
    componentResultsChild.put("synchronousComponentNum", 0);
    componentResultsChild.put("status", simulationOk ? "CONVERGED" : "SOLVER_FAILED");
    componentResultsChild.put("iterationCount", 0);
    if (slackNode_)
      componentResultsChild.put("slackBusId", slackNode_->id);
    else
      componentResultsChild.put("slackBusId", "NOT FOUND");
    componentResultsChild.put("slackBusActivePowerMismatch", 0);
    componentResultsTree.push_back(std::make_pair("", componentResultsChild));
    resultsTree.add_child("componentResults", componentResultsTree);

    file::path resultsOutput(config_.outputDir());
    resultsOutput.append("results.json");
    std::ofstream ofs(resultsOutput.c_str(), std::ios::binary);
    boost::property_tree::json_parser::write_json(ofs, resultsTree);
    break;
  }
  case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
    break;
  }
}

void Context::walkNodesMain() {
#if defined(_DEBUG_) || defined(PRINT_TIMERS)
  DYN::Timer timer("DFL::Context::walkNodesMain()");
#endif
  for (const auto &node : mainConnexNodes_) {
    for (const auto &cbk : callbacksMainConnexComponent_) {
      cbk(node, algoResults_);
    }
  }
}
}  // namespace dfl
