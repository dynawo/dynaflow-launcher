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

#include <DYNCommon.h>
#include <gtest_dynawo.h>

using DYN::doubleEquals;

TEST(Config, Nominal) {
  dfl::inputs::Configuration config("res/config.json");

  ASSERT_TRUE(config.useInfiniteReactiveLimits());
  ASSERT_FALSE(config.isSVCRegulationOn());
  ASSERT_FALSE(config.isShuntRegulationOn());
  ASSERT_FALSE(config.isAutomaticSlackBusOn());
  ASSERT_EQ(config.settingFilePath().generic_string(), "res/setting.xml");
  ASSERT_EQ(config.assemblingFilePath().generic_string(), "res/assembling.xml");
  ASSERT_EQ("/tmp", config.outputDir());
  ASSERT_EQ(63.0, config.getDsoVoltageLevel());
  ASSERT_EQ(dfl::inputs::Configuration::ActivePowerCompensation::P, config.getActivePowerCompensation());
  ASSERT_EQ(10, config.getStartTime().count());
  ASSERT_EQ(120, config.getStopTime().count());
  ASSERT_DOUBLE_EQUALS_DYNAWO(1e-3, config.getPrecision().value());
  ASSERT_EQ(50, config.getTimeOfEvent().count());
  ASSERT_DOUBLE_EQUALS_DYNAWO(2.6, config.getTimeStep());
}

TEST(Config, Default) {
  dfl::inputs::Configuration config("res/config_default.json");

  ASSERT_FALSE(config.useInfiniteReactiveLimits());
  ASSERT_TRUE(config.isSVCRegulationOn());
  ASSERT_TRUE(config.isShuntRegulationOn());
  ASSERT_TRUE(config.isAutomaticSlackBusOn());
  ASSERT_EQ(config.settingFilePath().generic_string(), "");
  ASSERT_EQ(config.assemblingFilePath().generic_string(), "");
  ASSERT_EQ(boost::filesystem::current_path().generic_string(), config.outputDir());
  ASSERT_EQ(45.0, config.getDsoVoltageLevel());
  ASSERT_EQ(dfl::inputs::Configuration::ActivePowerCompensation::PMAX, config.getActivePowerCompensation());
  ASSERT_EQ(80, config.getTimeOfEvent().count());
  ASSERT_DOUBLE_EQUALS_DYNAWO(10., config.getTimeStep());
}
