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
#include "Diagram.h"
#include "Dyd.h"
#include "Job.h"
#include "Log.h"
#include "Message.hpp"
#include "Par.h"

#include <DYNSimulation.h>
#include <DYNSimulationContext.h>
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
    config_(config),
    basename_{},
    slackNode_{},
    slackNodeOrigin_{SlackNodeOrigin::ALGORITHM},
    generators_{},
    loads_{},
    jobEntry_{} {
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
  networkManager_.onNode(algo::DynModelAlgorithm(models_, dynamicDataBaseManager_));
  networkManager_.onNode(algo::ShuntCounterAlgorithm(counters_));
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
  filterDynModels();

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

  return true;
}

void
Context::filterDynModels() {
  const auto& automatonsConfig = dynamicDataBaseManager_.assemblingDocument().dynamicAutomatons();
  for (const auto& automaton : automatonsConfig) {
    if (models_.models.count(automaton.id) == 0) {
      continue;
    }

    const auto& modelDef = models_.models.at(automaton.id);
    for (const auto& macroConnect : automaton.macroConnects) {
      auto found = std::find_if(
          modelDef.nodeConnections.begin(), modelDef.nodeConnections.end(),
          [&macroConnect](const algo::DynModelDefinition::MacroConnection& macroConnection) { return macroConnection.id == macroConnect.macroConnection; });
      if (found == modelDef.nodeConnections.end()) {
        LOG(debug) << "Dynamic model " << automaton.id << " is only partially connected to network so it is removed from exported models" << LOG_ENDL;
        models_.models.erase(automaton.id);
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
  outputs::Job jobWriter(outputs::Job::JobDefinition(basename_, def_.dynawoLogLevel));
  jobEntry_ = jobWriter.write();
#if _DEBUG_
  outputs::Job::exportJob(jobEntry_, absolute(def_.networkFilepath.generic_string()), config_.outputDir().generic_string());
#endif

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
  outputs::Par parWriter(outputs::Par::ParDefinition(basename_, config_.outputDir(), parOutput, generators_, hvdcLines_, config_.getActivePowerCompensation(),
                                                     busesWithDynamicModel_));
  parWriter.write();

  // Diagram
  file::path diagramDirectory(config_.outputDir());
  diagramDirectory.append(basename_ + outputs::constants::diagramDirectorySuffix);
  outputs::Diagram diagramWriter(outputs::Diagram::DiagramDefinition(basename_, diagramDirectory.generic_string(), generators_));
  diagramWriter.write();
}

void
Context::execute() {
  auto simu_context = boost::make_shared<DYN::SimulationContext>();
  simu_context->setResourcesDirectory(def_.dynawoResDir.generic_string());
  simu_context->setLocale(def_.locale);

  file::path inputPath(config_.outputDir());
  auto path = file::canonical(inputPath);
  simu_context->setInputDirectory(path.generic_string() + "/");
  simu_context->setWorkingDirectory(config_.outputDir().generic_string() + "/");

  // This shall be the last log performed before building simulation,
  // because simulation constructor will re-initialize traces for Dynawo
  // Since DFL traces are persistent, they can be re-used after simulation is performed outside this function
  LOG(info) << MESS(SimulateInfo, basename_) << LOG_ENDL;

  auto simu = boost::make_shared<DYN::Simulation>(jobEntry_, simu_context, networkManager_.dataInterface());
  simu->init();
  simu->simulate();
  simu->terminate();
  simu->clean();
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
