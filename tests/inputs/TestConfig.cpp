//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Configuration.h"
#include "Tests.h"
#include "Dico.h"

TEST(Config, Nominal) {
  dfl::common::Dico::configure("../../etc/DFLMessages_en_GB.dic");

  dfl::inputs::Configuration config("res/config.json");

  ASSERT_TRUE(config.useInfiniteReactiveLimits());
  ASSERT_FALSE(config.isPSTRegulationOn());
  ASSERT_FALSE(config.isSVCRegulationOn());
  ASSERT_FALSE(config.isShuntRegulationOn());
  ASSERT_FALSE(config.isAutomaticSlackBusOn());
  ASSERT_TRUE(config.useVSCAsGenerators());
  ASSERT_TRUE(config.useLCCAsLoads());
  ASSERT_EQ("/tmp", config.outputDir());
  ASSERT_EQ(63.0, config.getDsoVoltageLevel());
  ASSERT_EQ(dfl::inputs::Configuration::ActivePowerCompensation::P, config.getActivePowerCompensation());
}

TEST(Config, Default) {
  dfl::inputs::Configuration config("res/config_default.json");

  ASSERT_FALSE(config.useInfiniteReactiveLimits());
  ASSERT_TRUE(config.isPSTRegulationOn());
  ASSERT_TRUE(config.isSVCRegulationOn());
  ASSERT_TRUE(config.isShuntRegulationOn());
  ASSERT_TRUE(config.isAutomaticSlackBusOn());
  ASSERT_FALSE(config.useVSCAsGenerators());
  ASSERT_FALSE(config.useLCCAsLoads());
  ASSERT_EQ(boost::filesystem::current_path().generic_string(), config.outputDir());
  ASSERT_EQ(45.0, config.getDsoVoltageLevel());
  ASSERT_EQ(dfl::inputs::Configuration::ActivePowerCompensation::PMAX, config.getActivePowerCompensation());
}
