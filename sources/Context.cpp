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

#include <DYNSimulation.h>
#include <DYNSimulationContext.h>
#include <DYNDataInterface.h>
#include <DYNDanglingLineInterface.h>
#include <DYNLoadInterface.h>
#include <DYNVoltageLevelInterface.h>
#include <DYNShuntCompensatorInterface.h>
#include <DYNStaticVarCompensatorInterface.h>

#include <DYNScenario.h>
#include <DYNScenarios.h>
#include <DYNMultipleJobsFactory.h>
#include <DYNSystematicAnalysisLauncher.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <tuple>

namespace file = boost::filesystem;
using dfl::inputs::Contingencies;

namespace dfl {
Context::Context(const ContextDef& def, const inputs::Configuration& config) :
    def_(def),
    networkManager_(def.networkFilepath),
    config_(config),
    contingencies_(),
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
  for (auto &n: mainConnexNodes_) {
    mainConnexIds_.insert(n->id);
  }

  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    contingencies_ = dfl::inputs::Contingencies(def_.contingenciesFilepath);
  }
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

  onNodeOnMainConnexComponent(algo::GeneratorDefinitionAlgorithm(generators_, busesWithDynamicModel_, networkManager_.getMapBusId(),
                                                                 config_.useInfiniteReactiveLimits(), networkManager_.dataInterface()->getServiceManager()));
  onNodeOnMainConnexComponent(algo::LoadDefinitionAlgorithm(loads_, config_.getDsoVoltageLevel()));
  onNodeOnMainConnexComponent(algo::ControllerInterfaceDefinitionAlgorithm(hvdcLines_));
  walkNodesMain();

  // Check all contingencies have their elements present in the network
  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    checkContingencies();
  }

  return true;
}

bool
Context::isInMainConnectedComponent(const std::string& nodeId) const {
  auto res = mainConnexIds_.find(nodeId) != mainConnexIds_.end();
  if (res) {
    LOG(debug) << "        node in main connected component " << nodeId << LOG_ENDL;
  }
  return res;
}

bool
Context::areInMainConnectedComponent(const std::vector<boost::shared_ptr<DYN::BusInterface>>& buses) const {
  bool result;
  int count = 1;

  for(auto& bus: buses) {
    const auto& n = bus->getID();
    bool nvalid = isInMainConnectedComponent(n);
    if (buses.size() == 1) { // A simplified version for one bus
      LOG(debug) << "      bus = " << n << " valid = " << nvalid << LOG_ENDL;
    }
    else {
      LOG(debug) << "      bus" << count << " = " << n << " valid = " << nvalid << LOG_ENDL;
    }

    result = result | nvalid;
  }

  return result;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkGenerator(const std::string& generatorId) const {
  const auto& network = networkManager_.dataInterface()->getNetwork();
  for (auto& vl : network->getVoltageLevels()) {
    for (auto& g : vl->getGenerators()) {
      if (generatorId == g->getID()) {
        if (!areInMainConnectedComponent({g->getBusInterface()})) {
          return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
        }
        return boost::none; // No problem found
      }
    }
  }
  return Contingencies::ElementInvalidReason::GENERATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkLine(const std::string& branchId) const {
  const auto& network = networkManager_.dataInterface()->getNetwork();
  for (auto& l : network->getLines()) {
    if (branchId == l->getID()) {
      if (!areInMainConnectedComponent({l->getBusInterface1(), l->getBusInterface2()})) {
        return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
      }
      return boost::none; // No problem found
    }
  }
  return Contingencies::ElementInvalidReason::LINE_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkTwoWTransformer(const std::string& branchId) const {
  for (auto& t : networkManager_.dataInterface()->getNetwork()->getTwoWTransformers()) {
    if (branchId == t->getID()) {
      if (!areInMainConnectedComponent({t->getBusInterface1(),t->getBusInterface2()})) {
        return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
      }
      return boost::none; // No problem found
    }
  }
  return Contingencies::ElementInvalidReason::TWOWINDINGS_TRANFORMER_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkShuntCompensator(const std::string& shuntId) const {
  for (auto& lev : networkManager_.dataInterface()->getNetwork()->getVoltageLevels()) {
    for (auto& s: lev->getShuntCompensators()) {
      if (shuntId == s->getID()) {
        if (!areInMainConnectedComponent({s->getBusInterface()})) {
          return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
        }
        return boost::none; // No problem found
      }
    }
  }
  return Contingencies::ElementInvalidReason::SHUNT_COMPENSATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkLoad(const std::string& loadId) const {
  for (auto& t : networkManager_.dataInterface()->getNetwork()->getVoltageLevels()) {
    for (auto& l: t->getLoads()) {
      if (loadId == l->getID()) {
        if (!areInMainConnectedComponent(t->getBuses())) {
          return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
        }
        return boost::none; // No problem found
      }
    }
  }
  return Contingencies::ElementInvalidReason::SHUNT_COMPENSATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkDanglingLine(const std::string& dlineId) const {
  const auto& vlevels = networkManager_.dataInterface()->getNetwork()->getVoltageLevels();
  for (auto& lev: vlevels) {
    for (auto& l : lev->getDanglingLines()) {
      if (dlineId == l->getID()) {
        if (!areInMainConnectedComponent({l->getBusInterface()})) {
          return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
        }
        return boost::none; // No problem found
      }
    }
  }
  return Contingencies::ElementInvalidReason::DANGLING_LINE_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkStaticVarCompensator(const std::string& dlineId) const {
  const auto& vlevels = networkManager_.dataInterface()->getNetwork()->getVoltageLevels();
  for (auto& lev: vlevels) {
    for (auto& s : lev->getStaticVarCompensators()) {
      if (dlineId == s->getID()) {
        if (!areInMainConnectedComponent({s->getBusInterface()})) {
          return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
        }
        return boost::none; // No problem found
      }
    }
  }
  return Contingencies::ElementInvalidReason::STATIC_VAR_COMPENSATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkContingencyElement(const std::string& id, const std::string& type) const {
  if (type == "GENERATOR") {
    return checkGenerator(id);
  } else if (type == "LINE") {
    return checkLine(id);
  } else if (type == "TWO_WINDINGS_TRANSFORMER") {
    return checkTwoWTransformer(id);
  } else if (type == "BRANCH") {
    bool r = checkLine(id) || checkTwoWTransformer(id);
    if (!r) {
      return Contingencies::ElementInvalidReason::BRANCH_NOT_FOUND;
    }
    return boost::none; // No problem
  } else if (type == "SHUNT_COMPENSATOR") {
    return checkShuntCompensator(id);
  } else if (type == "LOAD") {
    return checkLoad(id);
  } else if (type == "DANGLING_LINE") {
    return checkDanglingLine(id);
  } else if (type == "STATIC_VAR_COMPENSATOR") {
    return checkStaticVarCompensator(id);
  } else if (type == "BUSBAR_SECTION") {
    return boost::none;
  }

  return Contingencies::ElementInvalidReason::TYPE_NOT_KNOWN;
}

std::vector<Contingencies::ElementInvalidReason>
Context::checkContingencies() const {
  std::vector<Contingencies::ElementInvalidReason> result;
  LOG(debug) << "Contingencies. Check that all elements of contingencies are valid for disconnection simulation" << LOG_ENDL;
  const auto& contingencies = contingencies_.definitions();
  for (auto c = contingencies.begin(); c != contingencies.end(); ++c) {
    LOG(debug) << c->id << LOG_ENDL;
    for (auto e = c->elements.begin(); e != c->elements.end(); ++e) {
      LOG(debug) << "  " << e->id << " (" << e->type << ")" << LOG_ENDL;
      std::string reason;
      auto invalid = checkContingencyElement(e->id, e->type);
      if (invalid) {
        LOG(warn) << "  Element " << e->id << " (" << e->type << ") not valid, reason: " << Contingencies::toString(*invalid) << LOG_ENDL;
        result.push_back(*invalid);
      } else {
        LOG(debug) << "  Element " << e->id << "(" << e->type << ") is valid" << LOG_ENDL;
      }
    }
  }

  return result;
}

void
Context::exportOutputs() {
  LOG(info) << MESS(ExportInfo, basename_) << LOG_ENDL;

  // create output directory
  file::path outputDir(config_.outputDir());

  // Job
  outputs::Job jobWriter(outputs::Job::JobDefinition(basename_, def_.dynawLogLevel));
  jobEntry_ = jobWriter.write();
  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    // For security analysis always write the main jobs file, as dynawo-algorithms will need it
    outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath), config_.outputDir());
  } else {
    // For the rest of simulations, only write jobs file when in DEBUG mode
#if _DEBUG_
    outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath), config_.outputDir());
#endif
  }

  // Dyd
  file::path dydOutput(config_.outputDir());
  dydOutput.append(basename_ + ".dyd");
  outputs::Dyd dydWriter(
      outputs::Dyd::DydDefinition(basename_, dydOutput.generic_string(), generators_, loads_, slackNode_, hvdcLines_, busesWithDynamicModel_));
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
  outputs::Par parWriter(outputs::Par::ParDefinition(basename_, config_.outputDir(), parOutput.generic_string(), generators_, hvdcLines_,
                                                     config_.getActivePowerCompensation(), busesWithDynamicModel_));
  parWriter.write();

  // Diagram
  file::path diagramDirectory(config_.outputDir());
  diagramDirectory.append(basename_ + outputs::constants::diagramDirectorySuffix);
  outputs::Diagram diagramWriter(outputs::Diagram::DiagramDefinition(basename_, diagramDirectory.generic_string(), generators_));
  diagramWriter.write();

  // Prepare a DYD, PAR and JOBS for every contingency
  // The DYD and PAR contains the definition of the events of the contingency
  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    exportOutputsContingencies();
  }
}

void
Context::exportOutputsContingencies() {
  LOG(debug) << "Preparing DYD, PAR and JOBS file for contingencies" << LOG_ENDL;
  const auto& contingencies = contingencies_.definitions();
  for (auto c = contingencies.begin(); c != contingencies.end(); ++c) {
    exportOutputsContingency(*c);
  }
}

void
Context::exportOutputsContingency(const inputs::Contingencies::ContingencyDefinition& c) {
  const std::string& contingencyId = c.id;
  LOG(debug) << contingencyId << LOG_ENDL;

  // Basename of event-related DYD, PAR and JOBS files
  const auto& basenameEvent = basename_ + "-" + contingencyId;

  // Specific DYD for contingency
  file::path dydEvent(config_.outputDir());
  dydEvent.append(basenameEvent + ".dyd");
  outputs::DydEvent dydEventWriter(outputs::DydEvent::DydEventDefinition(basenameEvent, dydEvent.generic_string(), c));
  dydEventWriter.write();

  // Specific PAR for contingency
  file::path parEvent(config_.outputDir());
  parEvent.append(basenameEvent + ".par");
  // TODO(Luma) should be a parameter
  double timeEvent = 80;
  outputs::ParEvent parEventWriter(outputs::ParEvent::ParEventDefinition(basenameEvent, parEvent.generic_string(), c, timeEvent));
  parEventWriter.write();

#if _DEBUG_
    // A JOBS file for every contingency is produced only in DEBUG mode
  outputs::Job jobEventWriter(outputs::Job::JobDefinition(basenameEvent, def_.dynawLogLevel, contingencyId, basename_));
  boost::shared_ptr<job::JobEntry> jobEvent = jobEventWriter.write();
  jobsEvents_.emplace_back(jobEvent);
  outputs::Job::exportJob(jobEvent, absolute(def_.networkFilepath), config_.outputDir());
#endif
}

void
Context::execute() {
  auto simu_context = boost::make_shared<DYN::SimulationContext>();
  simu_context->setResourcesDirectory(def_.dynawoResDir);
  simu_context->setLocale(def_.locale);

  file::path inputPath(config_.outputDir());
  auto path = file::canonical(inputPath);
  simu_context->setInputDirectory(path.generic_string() + "/");
  simu_context->setWorkingDirectory(config_.outputDir() + "/");

  // This SHALL be the last log performed before reseting traces in DynaFlowLauncher,
  // because simulation constructor will re-initialize traces for Dynawo
  LOG(info) << MESS(SimulateInfo, basename_) << LOG_ENDL;

  if (def_.simulationKind == SimulationKind::STEADY_STATE_CALCULATION) {
    // For a power flow calcualtion it is ok to directly run here a single simulation
    auto simu = boost::make_shared<DYN::Simulation>(jobEntry_, simu_context, networkManager_.dataInterface());
    simu->init();
    simu->simulate();
    simu->terminate();
    simu->clean();
  } else if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    // For security analysis we run multiple simulations using dynawo-algorithms
    // Create one scenario for the base case and one scenario for each contingency
    auto scenarios = boost::make_shared<DYNAlgorithms::Scenarios>();
    scenarios->setJobsFile(jobEntry_->getName() + ".jobs");
    const auto& contingencies = contingencies_.definitions();
    auto baseCase = boost::make_shared<DYNAlgorithms::Scenario>();
    baseCase->setId("BaseCase");
    scenarios->addScenario(baseCase);
    for (auto c = contingencies.begin(); c != contingencies.end(); ++c) {
      auto scenario = boost::make_shared<DYNAlgorithms::Scenario>();
      scenario->setId(c->id);
      scenario->setDydFile(basename_ + "-" + c->id + ".dyd");
      scenarios->addScenario(scenario);
    }
    // Use dynawo-algorithms Systematic Analysis Launcher to simulate all the scenarios
    auto multipleJobs = multipleJobs::MultipleJobsFactory::newInstance();
    multipleJobs->setScenarios(scenarios);
    auto saLauncher = boost::make_shared<DYNAlgorithms::SystematicAnalysisLauncher>();
    saLauncher->setMultipleJobs(multipleJobs);
    saLauncher->setOutputFile("sa.zip");
    saLauncher->setDirectory(config_.outputDir());
    saLauncher->setNbThreads(4);
    saLauncher->init();
    saLauncher->launch();
#if _DEBUG_
    saLauncher->writeResults();
#endif
  }
}

void
Context::walkNodesMain() {
  for (auto it = mainConnexNodes_.begin(); it != mainConnexNodes_.end(); ++it) {
    for (auto it_c = callbacksMainConnexComponent_.begin(); it_c != callbacksMainConnexComponent_.end(); ++it_c) {
      (*it_c)(*it);
    }
  }
}

}  // namespace dfl
