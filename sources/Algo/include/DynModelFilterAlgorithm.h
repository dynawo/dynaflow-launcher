//
// Copyright (c) 2023, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  DynModelFilterAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms to filter model definitions with partial connectivity : header file
 *
 */

#pragma once

#include "AssemblingDataBase.h"
#include "DynModelDefinitionAlgorithm.h"
#include "GeneratorDefinitionAlgorithm.h"

namespace dfl {
namespace algo {

/**
 * @brief DynModelFilterAlgorithm definition to remove model definitions with partial connectivity for algorithm
 */
class DynModelFilterAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * @param assembling assembling database
   * @param generators the generators list to update
   * @param dynamicModelsToFilter dynamic models by model id
   */
  DynModelFilterAlgorithm(const inputs::AssemblingDataBase& assembling,
                          GeneratorDefinitionAlgorithm::Generators& generators,
                          std::map<DynamicModelDefinition::DynModelId, DynamicModelDefinition>& dynamicModelsToFilter) :
    assembling_(assembling),
    generators_(generators),
    dynamicModelsToFilter_(dynamicModelsToFilter) {}

  /**
   * @brief Remove partially connected SVC, RPCL in generators not connected to SVC and filter remaining partially
   *        connected dynamic models
   */
  void filter();

 private:
  /**
   * @brief remove RPCL in generators if SVC is not connected to a bus and then remove the related SVC
   */
  void removeRpclInGeneratorsAndSvcIfMissingConnexionToSvc();

  /**
   * @brief Check if a SVC is connected to a bus
   *
   * @param svcModel SVC model to check
   *
   * @returns true if the SVC is connected to a bus, false otherwise
   */
  bool checkIfSVCconnectedToUMeasurement(const algo::DynamicModelDefinition& svcModel) const;

  /**
   * @brief Filter partially connected dynamic models
   *
   * Remove model definitions with partial connectivity
   */
  void filterPartiallyConnectedDynamicModels();

  const inputs::AssemblingDataBase& assembling_;                                         ///< assembling database
  GeneratorDefinitionAlgorithm::Generators& generators_;                                 ///< the generators list to update
  std::map<DynamicModelDefinition::DynModelId, DynamicModelDefinition>& dynamicModelsToFilter_;  ///< models by dynamic model id
};

}  // namespace algo
}  // namespace dfl
