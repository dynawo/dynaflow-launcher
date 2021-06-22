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
 * @file  AutomatonConfigurationManager.h
 *
 * @brief Manager of automaton file
 *
 */

#pragma once

#include "AssemblyXmlDocument.h"
#include "SettingsXmlDocument.h"

#include <boost/filesystem.hpp>
#include <memory>

namespace dfl {
namespace inputs {

/**
 * @brief Automaton configuration manager
 */
class AutomatonConfigurationManager {
 public:
  /**
   * @brief Retrieves the assembly document handler
   * @returns the assembly document handler
   */
  const AssemblyXmlDocument& assemblyDocument() const {
    return assemblyDoc_;
  }

  /**
   * @brief Retrieves the setting document handler
   * @returns the setting document handler
   */
  const SettingsXmlDocument& settingsDocument() const {
    return settingsDoc_;
  }

 public:
  /**
   * @brief Constructor
   * @param settingsFilePath the setting document file path
   * @param assemblyFilePath the assembly document file path
   */
  AutomatonConfigurationManager(const boost::filesystem::path& settingsFilePath, const boost::filesystem::path& assemblyFilePath);

 private:
  // Configuration
  AssemblyXmlDocument assemblyDoc_;  ///< assembly document handler
  SettingsXmlDocument settingsDoc_;  ///< setting document handler
};

}  // namespace inputs

}  // namespace dfl
