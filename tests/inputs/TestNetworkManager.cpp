//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "NetworkManager.h"
#include "Tests.h"

#include <regex>

static size_t count = 0;
static const std::regex reg("_BUS_+[0-9]+_TN");

static void
checkNode(const boost::shared_ptr<dfl::inputs::Node>& node) {
  std::smatch match;
  ASSERT_TRUE(std::regex_search(node->id, match, reg));
  ++count;
}

TEST(NetworkManager, walk) {
  using dfl::inputs::NetworkManager;

  NetworkManager manager("res/IEEE14.iidm");

  manager.onNode(&checkNode);

  count = 0;
  manager.walkNodes();
  ASSERT_EQ(14, count);
}
