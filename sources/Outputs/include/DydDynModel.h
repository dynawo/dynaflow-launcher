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
 * @file  DydDynModel.h
 *
 * @brief Dynaflow launcher DYD file writer for for defined dynamic models header file
 *
 */

#pragma once

#include "DynModelDefinitionAlgorithm.h"
#include "GeneratorDefinitionAlgorithm.h"

#include <DYDDynamicModelsCollection.h>
#include <boost/shared_ptr.hpp>

namespace dfl {
namespace outputs {

/**
 * @brief dynamic models DYD file writer
 */
class DydDynModel {
 public:
  /**
   * @brief Construct a new Par defined dynamic models object
   *
   * @param dynamicModelsDefinitions reference to the list of load definitions
   * @param gens generators definition coming from algorithms
   */
  explicit DydDynModel(const algo::DynamicModelDefinitions& dynamicModelsDefinitions, const std::vector<algo::GeneratorDefinition>& gens);

  /**
   * @brief enrich the dynamic black models set collection for defined dynamic models
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   * @param dynamicDataBaseManager the database manager to use
   */
  void write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename,
             const inputs::DynamicDataBaseManager& dynamicDataBaseManager);

 private:
  /**
   * @brief add the macro connector for defined dynamic models
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param dynModel defined dynamic model
   */
  void writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::DynamicModelDefinition& dynModel);
  /**
   * @brief add all the macro connectors for defined dynamic models
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param dynamicDataBaseManager the database manager to use
   */
  void writeMacroConnectors(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
                            const inputs::DynamicDataBaseManager& dynamicDataBaseManager);

 private:
  const algo::DynamicModelDefinitions& dynamicModelsDefinitions_;  ///< list of defined dynamic models
  std::unordered_set<std::string> generatorsWithDynamicModels_;    ///< ids of generators with a dynamic model different from the network one
};

}  // namespace outputs
}  // namespace dfl
