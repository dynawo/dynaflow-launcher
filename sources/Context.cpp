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

#include <tuple>

namespace dfl {
Context::Context(const std::string& networkFilepath, const std::string& configFilepath) :
    networkManager_(networkFilepath),
    config_(configFilepath),
    slack_node_{} {
  auto found_slack_node = networkManager_.getSlackNode();
  if (found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
    slack_node_ = *found_slack_node;
  } else {
    // slack node not given in iidm or not requested: it is computed internally
    if (!found_slack_node.is_initialized() && !config_.isAutomaticSlackBusOn()) {
      // TODO(lecourtoisflo) use warning without restriction for language
      LOG(warn) << "Network slack node requested but not present in network input file" << LOG_ENDL;
    }
    networkManager_.onNode(std::bind(&Context::processSlackNode, this, std::placeholders::_1));
  }
}

void
Context::processSlackNode(const boost::shared_ptr<inputs::Node>& node) {
  if (!slack_node_) {
    slack_node_ = node;
  } else {
    if (std::forward_as_tuple(slack_node_->nominalVoltage, slack_node_->neighbours.size()) >
        std::forward_as_tuple(node->nominalVoltage, node->neighbours.size())) {
      slack_node_ = node;
    }
  }
}

void
Context::process() {
  // Process all algorithms on nodes
  networkManager_.walkNodes();
}

}  // namespace dfl
