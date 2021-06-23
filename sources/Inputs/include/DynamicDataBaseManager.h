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

#include "AssemblingXmlDocument.h"
#include "SettingXmlDocument.h"

#include <boost/filesystem.hpp>
#include <memory>

namespace dfl {
namespace inputs {

/**
 * @brief Automaton configuration manager
 */
class DynamicDataBaseManager {
 public:
  /**
   * @brief Retrieves the assembling document handler
   * @returns the assembling document handler
   */
  const AssemblingXmlDocument& assemblingDocument() const {
    return assemblingDoc_;
  }

  /**
   * @brief Retrieves the setting document handler
   * @returns the setting document handler
   */
  const SettingXmlDocument& settingDocument() const {
    return settingsDoc_;
  }

 public:
  /**
   * @brief Constructor
   * @param settingsFilePath the setting document file path
   * @param assemblingFilePath the assembling document file path
   */
  DynamicDataBaseManager(const boost::filesystem::path& settingsFilePath, const boost::filesystem::path& assemblingFilePath);

 private:
  // Configuration
  AssemblingXmlDocument assemblingDoc_;  ///< assembling document handler
  SettingXmlDocument settingsDoc_;       ///< setting document handler
};

}  // namespace inputs

}  // namespace dfl
