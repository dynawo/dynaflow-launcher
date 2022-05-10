//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DynamicDataBaseManager.h"
#include "Log.h"
#include "Tests.h"

#include <gtest_dynawo.h>

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestDynamicDataBaseManager, base) {
  using dfl::inputs::DynamicDataBaseManager;
  DynamicDataBaseManager manager("res/setting.xml", "res/assembling.xml");

  const auto& sets = manager.settingDocument().sets();
  ASSERT_EQ(sets.size(), 14);

  const auto& macroConnections = manager.assemblingDocument().macroConnections();
  ASSERT_EQ(macroConnections.size(), 2);
  const auto& singleAssociations = manager.assemblingDocument().singleAssociations();
  ASSERT_EQ(singleAssociations.size(), 6);

  const auto& multiAssociations = manager.assemblingDocument().multipleAssociations();
  ASSERT_EQ(multiAssociations.size(), 2);

  const auto& modelAssociations = manager.assemblingDocument().modelAssociations();
  ASSERT_EQ(modelAssociations.size(), 2);

  const auto& dynamicAutomatons = manager.assemblingDocument().dynamicAutomatons();
  ASSERT_EQ(dynamicAutomatons.size(), 2);

  // The rest is considered covered by the unit tests of the members classes
}

size_t dummySize = 0;

static void
createManager() {
  dfl::inputs::DynamicDataBaseManager manager("res/setting.xml", "");

  // pointless operation to ensure that construction is not removed when compiling
  dummySize = manager.settingDocument().sets().size();
}

// MUST BE THE LAST TEST OF THE FILE, SINCE WE MODIFY THE ENVIRONMENTS VARIABLES
TEST(TestDynamicDataBaseManager, wrong_xsd) {
  using dfl::inputs::DynamicDataBaseManager;

  setenv("DYNAFLOW_LAUNCHER_XSD", "res", 1);

  ASSERT_THROW_DYNAWO(createManager(), DYN::Error::GENERAL, dfl::KeyError_t::DynModelFileReadError);
}
