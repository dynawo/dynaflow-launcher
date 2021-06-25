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

#include <DYNScenario.h>
#include <DYNScenarios.h>
#include <DYNConverterInterface.h>
#include <DYNMultipleJobsFactory.h>
#include <DYNSystematicAnalysisLauncher.h>


#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <numeric>
#include <tuple>

namespace file = boost::filesystem;
using dfl::inputs::Contingencies;

namespace dfl {

void Context::Caches::initCaches(
  const std::vector<std::shared_ptr<inputs::Node>>& mainConnexNodes,
  const boost::shared_ptr<DYN::NetworkInterface> &network
) {
  for (auto &n: mainConnexNodes) {
    mainConnexIds.insert(n->id);
  }

  lines = makeCacheOf(network->getLines());
  twoWTransformers = makeCacheOf(network->getTwoWTransformers());
  hvdcLines = makeCacheOf(network->getHvdcLines());

  for (auto &lev: network->getVoltageLevels()) {
    const auto gens = makeCacheOf(lev->getGenerators());
    generators.insert(gens.begin(), gens.end());

    const auto shunts = makeCacheOf(lev->getShuntCompensators());
    shuntCompensators.insert(shunts.begin(), shunts.end());

    const auto lds = makeCacheOf(lev->getLoads());
    loads.insert(lds.begin(), lds.end());

    const auto dlines = makeCacheOf(lev->getDanglingLines());
    danglingLines.insert(dlines.begin(),dlines.end());

    const auto scomps = makeCacheOf(lev->getStaticVarCompensators());
    staticVarComps.insert(scomps.begin(), scomps.end());
  }
}

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

  // Fill caches
  const auto& network = networkManager_.dataInterface()->getNetwork();
  caches_.initCaches(mainConnexNodes_, network);


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

  // Check all contingencies have their elements present in the network and make
  // a new list of Contingencies with only those that are good
  if (def_.simulationKind == SimulationKind::SECURITY_ANALYSIS) {
    std::vector<std::shared_ptr<dfl::inputs::Contingencies::ContingencyDefinition>> defs;
    std::vector<dfl::inputs::Contingencies::ElementInvalidReason> errors;
    std::tie(defs, errors) = checkAndFilterContingencies();
    contingencies_ = Contingencies(defs);
  }

  return true;
}

bool
Context::isInMainConnectedComponent(const std::string& nodeId) const {
  auto res = caches_.mainConnexIds.find(nodeId) != caches_.mainConnexIds.end();
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
  const auto item = caches_.generators.find(generatorId);
  if (item != caches_.generators.end()) {
    if (!areInMainConnectedComponent({item->second->getBusInterface()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found

  }

  return Contingencies::ElementInvalidReason::GENERATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkLine(const std::string& branchId) const {
  const auto item = caches_.lines.find(branchId);
  if (item != caches_.lines.end()) {
    if (!areInMainConnectedComponent({item->second->getBusInterface1(), item->second->getBusInterface2()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found
  }

  return Contingencies::ElementInvalidReason::LINE_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkTwoWTransformer(const std::string& twoWTransId) const {
  const auto item = caches_.twoWTransformers.find(twoWTransId);
  if (item != caches_.twoWTransformers.end()) {
    if (!areInMainConnectedComponent({item->second->getBusInterface1(), item->second->getBusInterface2()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found
  }

  return Contingencies::ElementInvalidReason::TWOWINDINGS_TRANFORMER_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkShuntCompensator(const std::string& shuntId) const {
  const auto item = caches_.shuntCompensators.find(shuntId);
  if (item != caches_.shuntCompensators.end()) {
    if (!areInMainConnectedComponent({item->second->getBusInterface()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found
  }

  return Contingencies::ElementInvalidReason::SHUNT_COMPENSATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkLoad(const std::string& loadId) const {
  const auto item = caches_.loads.find(loadId);
  if (item != caches_.loads.find(loadId)) {
    if (!areInMainConnectedComponent({item->second->getBusInterface()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found
  }
  return Contingencies::ElementInvalidReason::SHUNT_COMPENSATOR_NOT_FOUND;
}


boost::optional<Contingencies::ElementInvalidReason>
Context::checkDanglingLine(const std::string& dlineId) const {
  const auto item = caches_.danglingLines.find(dlineId);
  if (item != caches_.danglingLines.end()) {
    if (!areInMainConnectedComponent({item->second->getBusInterface()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found
  }

  return Contingencies::ElementInvalidReason::DANGLING_LINE_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkHvdcLine(const std::string& hlineId) const {
  const auto item = caches_.hvdcLines.find(hlineId);
  if (item != caches_.hvdcLines.end()) {
      if (!areInMainConnectedComponent({
        item->second->getConverter1()->getBusInterface(),
        item->second->getConverter2()->getBusInterface()})) {
        return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
      }
      return boost::none; // No problem found
  }

  return Contingencies::ElementInvalidReason::HVDC_LINE_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkStaticVarCompensator(const std::string& dlineId) const {
  const auto item = caches_.staticVarComps.find(dlineId);
  if (item != caches_.staticVarComps.end()) {
    if (!areInMainConnectedComponent({item->second->getBusInterface()})) {
      return Contingencies::ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT;
    }
    return boost::none; // No problem found
  }

  return Contingencies::ElementInvalidReason::STATIC_VAR_COMPENSATOR_NOT_FOUND;
}

boost::optional<Contingencies::ElementInvalidReason>
Context::checkContingencyElement(const std::string& id, Contingencies::Type type) const {
  switch(type){
    case Contingencies::Type::GENERATOR:
      return checkGenerator(id);
    case Contingencies::Type::LINE:
      return checkLine(id);
    case Contingencies::Type::TWO_WINDINGS_TRANSFORMER:
      return checkTwoWTransformer(id);
    case Contingencies::Type::BRANCH: {
      bool r = checkLine(id) || checkTwoWTransformer(id);
      if (!r) {
        return Contingencies::ElementInvalidReason::BRANCH_NOT_FOUND;
      }
      return boost::none; // No problem
    }
    case Contingencies::Type::SHUNT_COMPENSATOR:
      return checkShuntCompensator(id);
    case Contingencies::Type::LOAD:
      return checkLoad(id);
    case Contingencies::Type::DANGLING_LINE:
      return checkDanglingLine(id);
    case Contingencies::Type::HVDC_LINE:
      return checkHvdcLine(id);
    case Contingencies::Type::STATIC_VAR_COMPENSATOR:
      return checkStaticVarCompensator(id);
    case Contingencies::Type::BUSBAR_SECTION:
      return boost::none;

    default:
      throw std::logic_error("Gotten an unexpected type (or a corrupted value)");
  }
}

std::vector<Contingencies::ElementInvalidReason>
Context::checkContingency(std::shared_ptr<dfl::inputs::Contingencies::ContingencyDefinition> c) const {
  std::vector<Contingencies::ElementInvalidReason> result;
  LOG(debug) << c->id << LOG_ENDL;

  // Check each element and see whether they are good for inclusion
  for (auto e: c->elements) {
    LOG(debug) << "  " << e.id << " (" << Contingencies::toString(e.type) << ")" << LOG_ENDL;
    auto invalid = checkContingencyElement(e.id, e.type);
    if (invalid) {
      LOG(warn) << MESS(ElementInvalid, e.id, Contingencies::toString(e.type), Contingencies::toString(*invalid)) << LOG_ENDL;
      result.push_back(*invalid);
    } else {
      LOG(debug) << "  Element " << e.id << "(" << Contingencies::toString(e.type) << ") is valid" << LOG_ENDL;
    }
  }

  return result;
}

std::tuple<
  std::vector<std::shared_ptr<dfl::inputs::Contingencies::ContingencyDefinition>>,
  std::vector<dfl::inputs::Contingencies::ElementInvalidReason>
>
Context::checkAndFilterContingencies() const {
  std::vector<Contingencies::ElementInvalidReason> errors;
  LOG(debug) << "Contingencies. Check that all elements of contingencies are valid for disconnection simulation" << LOG_ENDL;

  // We'll store here the iterators we don't want
  std::vector<std::vector<std::shared_ptr<dfl::inputs::Contingencies::ContingencyDefinition>>::iterator> iteratorsToRemove;
  auto defs = contingencies_.definitions();
  for (auto c = defs.begin(); c != defs.end(); ++c) {
    const auto errs = checkContingency(*c);
    if (!errs.empty()) {
      iteratorsToRemove.push_back(c);
      errors.reserve(errors.size() + errs.size());
      errors.insert(errors.end(), errs.begin(), errs.end());
    }
  }

  // Better start from the end just in case the iterators reffer to elements
  // by position
  for (auto i = iteratorsToRemove.rbegin(); i != iteratorsToRemove.rend(); ++i) {
    defs.erase(*i);
  }

  return {defs, errors};
}

void
Context::exportOutputs() {
  LOG(info) << MESS(ExportInfo, basename_) << LOG_ENDL;

  // create output directory
  file::path outputDir(config_.outputDir());
  outputs::Job::setStartAndDuration(config_.getStartTimestamp(),
    config_.getEndTimestamp() - config_.getStartTimestamp());

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
Context::exportOutputsContingency(const std::shared_ptr<inputs::Contingencies::ContingencyDefinition>& c) {
  const std::string& contingencyId = c->id;
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
  outputs::ParEvent parEventWriter(outputs::ParEvent::ParEventDefinition(basenameEvent, parEvent.generic_string(), c, config_.getContingenciesTimestamp()));
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
      scenario->setId((*c)->id);
      scenario->setDydFile(basename_ + "-" + (*c)->id + ".dyd");
      scenarios->addScenario(scenario);
    }
    // Use dynawo-algorithms Systematic Analysis Launcher to simulate all the scenarios
    auto multipleJobs = multipleJobs::MultipleJobsFactory::newInstance();
    multipleJobs->setScenarios(scenarios);
    auto saLauncher = boost::make_shared<DYNAlgorithms::SystematicAnalysisLauncher>();
    saLauncher->setMultipleJobs(multipleJobs);
    saLauncher->setOutputFile("sa.zip");
    saLauncher->setDirectory(config_.outputDir());
    saLauncher->setNbThreads(config_.getNumberOfThreads());
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
