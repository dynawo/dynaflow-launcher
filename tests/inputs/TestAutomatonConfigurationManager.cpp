//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "AutomatonConfigurationManager.h"
#include "Tests.h"

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestAutomatonConfigurationManager, base) {
  using dfl::inputs::AutomatonConfigurationManager;
  AutomatonConfigurationManager manager("res/setting.xml", "res/assembling.xml");

  const auto& sets = manager.settingsDocument().sets();
  ASSERT_EQ(sets.size(), 14);

  const auto& macroConnections = manager.assemblyDocument().macroConnections();
  ASSERT_EQ(macroConnections.size(), 2);
  const auto& singleAssociations = manager.assemblyDocument().singleAssociations();
  ASSERT_EQ(singleAssociations.size(), 6);

  const auto& multiAssociations = manager.assemblyDocument().multipleAssociations();
  ASSERT_EQ(multiAssociations.size(), 2);

  const auto& dynamicAutomatons = manager.assemblyDocument().dynamicAutomatons();
  ASSERT_EQ(dynamicAutomatons.size(), 2);

  // The rest is considered covered by the unit tests of the lower classes
}
