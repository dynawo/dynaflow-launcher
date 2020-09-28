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
 * @file  Node.cpp
 *
 * @brief Node structure implementation file
 *
 */

#include "Node.h"

namespace dfl {
namespace inputs {
Node::Node(const NodeId& idNode, double nominalVoltageNode) : id(idNode), nominalVoltage{nominalVoltageNode}, neighbours{} {}

}  // namespace inputs
}  // namespace dfl
