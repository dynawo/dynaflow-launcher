//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "SlackNodeAlgorithm.h"

#include <DYNCommon.h>

namespace dfl {
namespace algo {

SlackNodeAlgorithm::SlackNodeAlgorithm(NodePtr& slackNode) : slackNode_(slackNode) {}

void
SlackNodeAlgorithm::operator()(const NodePtr& node) {
  if (node->fictitious) {
    return;
  }
  if (!slackNode_) {
    slackNode_ = node;
  } else {
    if (std::forward_as_tuple(slackNode_->nominalVoltage, slackNode_->neighbours.size()) <
        std::forward_as_tuple(node->nominalVoltage, node->neighbours.size())) {
      slackNode_ = node;
    }
  }
}

}  // namespace algo
}  // namespace dfl
