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

#include "Log.h"
#include "Message.hpp"

#include <tuple>

namespace dfl {
Context::Context(const std::string& networkFilepath, const std::string& configFilepath) :
    networkManager_(networkFilepath),
    config_(configFilepath),
    slackNode_{},
    slackNodeOrigin_{SlackNodeOrigin::ALGORITHM} {
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
    networkManager_.onNode(std::bind(&Context::processSlackNode, this, std::placeholders::_1));
  }
}

void
Context::processSlackNode(const boost::shared_ptr<inputs::Node>& node) {
  if (!slackNode_) {
    slackNode_ = node;
  } else {
    if (std::forward_as_tuple(slackNode_->nominalVoltage, slackNode_->neighbours.size()) >
        std::forward_as_tuple(node->nominalVoltage, node->neighbours.size())) {
      slackNode_ = node;
    }
  }
}

void
Context::process() {
  // Process all algorithms on nodes
  networkManager_.walkNodes();

  LOG(info) << MESS(SlackNode, slackNode_->id, static_cast<unsigned int>(slackNodeOrigin_)) << LOG_ENDL;
}

}  // namespace dfl
