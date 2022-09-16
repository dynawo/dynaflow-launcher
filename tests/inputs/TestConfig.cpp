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
#include "Log.h"
#include "Tests.h"

#include <DYNCommon.h>
#include <gtest_dynawo.h>

using DYN::doubleEquals;

TEST(Config, Incorrect) {
  ASSERT_THROW_DYNAWO(dfl::inputs::Configuration config("res/incorrect_config.json"),
                                                        DYN::Error::GENERAL,
                                                        dfl::KeyError_t::ErrorConfigFileRead);
  ASSERT_THROW_DYNAWO(dfl::inputs::Configuration config("res/config_flat_activepowercompensation_p.json"),
                                                        DYN::Error::GENERAL,
                                                        dfl::KeyError_t::InvalidActivePowerCompensation);
}

TEST(Config, Nominal) {
  dfl::inputs::Configuration config("res/config.json");

  ASSERT_TRUE(config.useInfiniteReactiveLimits());
  ASSERT_FALSE(config.isSVCRegulationOn());
  ASSERT_FALSE(config.isShuntRegulationOn());
  ASSERT_FALSE(config.isAutomaticSlackBusOn());
  ASSERT_EQ(config.settingFilePath().generic_string(), "res/setting.xml");
  ASSERT_EQ(config.assemblingFilePath().generic_string(), "res/assembling.xml");
  ASSERT_EQ("/tmp", config.outputDir());
  ASSERT_DOUBLE_EQUALS_DYNAWO(63.0, config.getDsoVoltageLevel());
  ASSERT_DOUBLE_EQUALS_DYNAWO(150., config.getTfoVoltageLevel());
  ASSERT_EQ(dfl::inputs::Configuration::ActivePowerCompensation::P, config.getActivePowerCompensation());
  ASSERT_DOUBLE_EQUALS_DYNAWO(10, config.getStartTime());
  ASSERT_DOUBLE_EQUALS_DYNAWO(120, config.getStopTime());
  ASSERT_DOUBLE_EQUALS_DYNAWO(1e-3, config.getPrecision().value());
  ASSERT_DOUBLE_EQUALS_DYNAWO(50, config.getTimeOfEvent());
  ASSERT_DOUBLE_EQUALS_DYNAWO(2.6, config.getTimeStep());
#if _DEBUG_
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ));
#else
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
  ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE));
  ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ));
#endif
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
  ASSERT_DOUBLE_EQUALS_DYNAWO(45.0, config.getDsoVoltageLevel());
  ASSERT_DOUBLE_EQUALS_DYNAWO(100., config.getTfoVoltageLevel());
  ASSERT_EQ(dfl::inputs::Configuration::ActivePowerCompensation::PMAX, config.getActivePowerCompensation());
  ASSERT_DOUBLE_EQUALS_DYNAWO(10., config.getTimeOfEvent());
  ASSERT_DOUBLE_EQUALS_DYNAWO(10., config.getTimeStep());
#if _DEBUG_
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE));
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ));
#else
  ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
  ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS));
  ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE));
  ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ));
#endif
}

TEST(Config, ChosenOutputTest) {
  /*
  To check which field should be displayed we use four masks :
  STEADYSTATE corresponds to 1000
  CONSTRAINTS corresponds to 0100
  TIMELINE corresponds to 0010
  LOSTEQ corresponds to 0001
  */

  dfl::inputs::Configuration::SimulationKind simulationKindsArray[2] = {dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION,
                                                                        dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS};
  for (dfl::inputs::Configuration::SimulationKind simulationKind : simulationKindsArray) {
    std::map<const std::string, const int> configFileToChosenOutputMap;
#if _DEBUG_
    configFileToChosenOutputMap = {{"../outputs/res/config_empty.json", 0b1111},
                                   {"../outputs/res/config_only_lost_equipments_chosen.json", 0b1111},
                                   {"../outputs/res/config_all_chosen_outputs.json", 0b1111}};
#else
    switch (simulationKind) {
    case dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION:
      configFileToChosenOutputMap = {{"../outputs/res/config_empty.json", 0b1000},
                                     {"../outputs/res/config_only_lost_equipments_chosen.json", 0b1001},
                                     {"../outputs/res/config_all_chosen_outputs.json", 0b1111}};
      break;
    case dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS:
      configFileToChosenOutputMap = {{"../outputs/res/config_empty.json", 0b0101},
                                     {"../outputs/res/config_only_timeline_chosen.json", 0b0111},
                                     {"../outputs/res/config_all_chosen_outputs.json", 0b1111}};
      break;
    }
#endif

    ASSERT_FALSE(configFileToChosenOutputMap.empty());

    for (std::pair<const std::string, const int> chosenOutput : configFileToChosenOutputMap) {
      const boost::filesystem::path chosenOutputsPath = static_cast<boost::filesystem::path>(chosenOutput.first);
      dfl::inputs::Configuration config(chosenOutputsPath, simulationKind);

      if (chosenOutput.second & 0b1000) {
        ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
      } else {
        ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::STEADYSTATE));
      }

      if (chosenOutput.second & 0b0100) {
        ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS));
      } else {
        ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::CONSTRAINTS));
      }

      if (chosenOutput.second & 0b0010) {
        ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE));
      } else {
        ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::TIMELINE));
      }

      if (chosenOutput.second & 0b0001) {
        ASSERT_TRUE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ));
      } else {
        ASSERT_FALSE(config.isChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::LOSTEQ));
      }
    }
  }
}
