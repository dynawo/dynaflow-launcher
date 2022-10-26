//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  DynamicDataBaseManager.h
 *
 * @brief Manager of dynamic database files
 *
 */

#pragma once

#include "AssemblingDataBase.h"
#include "SettingDataBase.h"

#include <boost/filesystem.hpp>

namespace dfl {
namespace inputs {

/**
 * @brief Automaton configuration manager
 */
class DynamicDataBaseManager {
 public:
  /**
   * @brief Constructor
   * @param settingFilePath the setting document file path
   * @param assemblingFilePath the assembling document file path
   */
  DynamicDataBaseManager(const boost::filesystem::path& settingFilePath, const boost::filesystem::path& assemblingFilePath);

  /**
   * @brief get current assembling database
   * @return current assembling database
   */
  const AssemblingDataBase& assembling() const {
    return assembling_;
  }

  /**
   * @brief get current setting database
   * @return current setting database
   */
  const SettingDataBase& setting() const {
    return setting_;
  }

 private:
  AssemblingDataBase assembling_;  ///< assembling database
  SettingDataBase setting_;        ///< setting database
};

}  // namespace inputs

}  // namespace dfl
