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
#include "Dyd.h"
#include "Log.h"
#include "Message.hpp"

#include <DYNSimulationContext.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <tuple>

namespace file = boost::filesystem;

namespace dfl {
Context::Context(const ContextDef& def) :
    def_{def},
    networkManager_(def.networkFilepath),
    config_(def.configFilepath),
    basename_{},
    slackNode_{},
    slackNodeOrigin_{SlackNodeOrigin::ALGORITHM},
    generators_{},
    loads_{},
    simu_{} {
  file::path path(def.networkFilepath);
  basename_ = path.filename().replace_extension().generic_string();

  auto found_slack_node = networkManager_.getSlackNode();
  if (found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
    slackNode_ = *found_slack_node;
    slackNodeOrigin_ = SlackNodeOrigin::FILE;
  } else {
    slackNodeOrigin_ = SlackNodeOrigin::ALGORITHM;
    // slack node not given in iidm or not requested: it is computed internally
    if (!found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
      LOG(warn) << MESS(NetworkSlackNodeNotFound, def.networkFilepath) << LOG_ENDL;
    }
    networkManager_.onNode(algo::SlackNodeAlgorithm(slackNode_));
  }

  networkManager_.onNode(algo::MainConnexComponentAlgorithm(mainConnexNodes_));
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

  onNodeOnMainConnexComponent(algo::GeneratorDefinitionAlgorithm(generators_, config_.useInfiniteReactiveLimits()));
  onNodeOnMainConnexComponent(algo::LoadDefinitionAlgorithm(loads_));

  walkNodesMain();

  return true;
}

void
Context::createSimulation(boost::shared_ptr<job::JobEntry>& job) {
  auto simu_context = boost::make_shared<DYN::SimulationContext>();
  simu_context->setResourcesDirectory(def_.dynawoResDir);
  simu_context->setLocale(def_.locale);

  file::path inputPath(config_.outputDir());
  auto path = file::canonical(inputPath);
  simu_context->setInputDirectory(path.generic_string() + "/");
  simu_context->setWorkingDirectory(config_.outputDir() + "/");

  // TODO(lecourtoisflo) For now, since we don't generate the other outputs files, an exception is trigerred: uncomment this once all files are generated
  // simu_ = boost::make_shared<DYN::Simulation>(job, simu_context);
  // simu_->init();
  (void)job;  // TODO(lecourtoisflo) remove this line after uncommenting previous TODO: this line is here to avoid the unused variable error
}

void
Context::exportOutputs() {
  LOG(info) << MESS(ExportInfo, basename_) << LOG_ENDL;

  // create output directory
  file::path outputDir(config_.outputDir());
  if (!file::exists(outputDir)) {
    file::create_directory(outputDir);
  }

  // Copy IIDM in output directory in order to be used correctly by the simulation
  // TODO(lecourtoisflo) Remove this when IIDM will not be re-read by Dynawo library
  file::path dest(outputDir);
  dest.append(basename_ + ".iidm");
  file::copy_file(def_.networkFilepath, dest, file::copy_option::overwrite_if_exists);

  // Job
  outputs::Job jobWriter(outputs::Job::JobDefinition(basename_, def_.dynawLogLevel));
  auto job = jobWriter.write();

  // Dyd
  file::path dydOutput(config_.outputDir());
  dydOutput.append(basename_ + ".dyd");
  outputs::Dyd dydWriter(outputs::Dyd::DydDefinition(basename_, dydOutput.generic_string(), generators_, loads_, slackNode_));
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

  createSimulation(job);
}

void
Context::execute() {
  if (simu_) {
    simu_->simulate();
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
