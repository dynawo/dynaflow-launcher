//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Message.hpp"
#include "Tests.h"

TEST(Message, wrongkey) {
  dfl::common::Dico::configure("res/test.dic");

  auto message = MESS(SimulateInfo, true, "test");

  // SimulateInfo is not defined in dictionnary test.dic
  ASSERT_EQ(message, "SimulateInfo 1 test");
}

TEST(Message, message) {
  dfl::common::Dico::configure("res/test.dic");

  auto message = MESS(NetworkSlackNodeNotFound, true, "test");

  ASSERT_EQ(message, "text with key common to real dic 1 then test");
}
