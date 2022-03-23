//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  DydLoads.h
 *
 * @brief Dynaflow launcher DYD file writer for loads header file
 *
 */

#pragma once

#include "LoadDefinitionAlgorithm.h"

#include <DYDDynamicModelsCollection.h>
#include <boost/shared_ptr.hpp>

namespace dfl {
namespace outputs {

/**
 * @brief loads DYD file writer
 */
class DydLoads {
 public:
  /**
   * @brief Construct a new Par Loads object
   *
   * @param loadsDefinitions reference to the list of load definitions
   */
  explicit DydLoads(const std::vector<algo::LoadDefinition>& loadsDefinitions) : loadsDefinitions_(loadsDefinitions) {}

  /**
   * @brief enrich the dynamic black models set collection for loads
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   */
  void write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename);

 private:
  /**
   * @brief add the macro connector for loads
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief add the macro static reference for loads
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

 private:
  std::vector<algo::LoadDefinition> loadsDefinitions_;                  ///< list of loads definitions
  const std::string macroStaticRefLoadName_{"LoadRef"};                 ///< Name for the static ref macro for loads
  const std::string macroConnectorLoadName_{"LOAD_NETWORK_CONNECTOR"};  ///< name of the macro connector for loads
};

}  // namespace outputs
}  // namespace dfl
