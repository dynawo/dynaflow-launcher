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
#include "Log.h"
#include "Message.hpp"

#include <boost/filesystem.hpp>
#include <tuple>

namespace file = boost::filesystem;

namespace dfl {
Context::Context(const std::string& networkFilepath, const std::string& configFilepath, const std::string& dynawLogLevel, const std::string& parFileDir) :
    networkManager_(networkFilepath),
    config_(configFilepath),
    basename_{},
    dynawLogLevel_{dynawLogLevel},
    parFileDir_{parFileDir},
    slackNode_{},
    slackNodeOrigin_{SlackNodeOrigin::ALGORITHM},
    jobWriter_{} {
  file::path path(networkFilepath);
  basename_ = path.filename().replace_extension().generic_string();

  auto found_slack_node = networkManager_.getSlackNode();
  if (found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
    slackNode_ = *found_slack_node;
    slackNodeOrigin_ = SlackNodeOrigin::FILE;
  } else {
    slackNodeOrigin_ = SlackNodeOrigin::ALGORITHM;
    // slack node not given in iidm or not requested: it is computed internally
    if (!found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
      LOG(warn) << MESS(NetworkSlackNodeNotFound, networkFilepath) << LOG_ENDL;
    }
    networkManager_.onNode(algo::SlackNodeAlgorithm(slackNode_));
  }

  networkManager_.onNode(algo::ConnexityAlgorithm(mainConnexNodes_));
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
    LOG(error) << MESS(ConnexityError, slackNode_->id) << LOG_ENDL;
    return false;
  }

  return true;
}

void
Context::exportOutputs() {
  LOG(info) << MESS(ExportInfo, basename_) << LOG_ENDL;

  // Job
  jobWriter_ = std::unique_ptr<outputs::Job>(new outputs::Job(outputs::Job::JobDefinition(config_.outputDir(), basename_, dynawLogLevel_)));
  jobWriter_->write();

  // Par
  // copy constants files
  for (auto& entry : boost::make_iterator_range(file::directory_iterator(parFileDir_))) {
    if (entry.path().extension() == ".par") {
      file::path dest(config_.outputDir());
      dest.append(entry.path().filename().generic_string());
      file::copy_file(entry.path(), dest, file::copy_option::overwrite_if_exists);
    }
  }
}

}  // namespace dfl
