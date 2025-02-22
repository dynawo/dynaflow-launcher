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
 * @file  ParLoads.h
 *
 * @brief Dynaflow launcher PAR file writer for loads header file
 *
 */

#pragma once

#include "LoadDefinitionAlgorithm.h"

#include <PARParametersSetCollection.h>


namespace dfl {
namespace outputs {

/**
 * @brief loads PAR file writer
 */
class ParLoads {
 public:
  /**
   * @brief Construct a new Par Loads object
   *
   * @param loadsDefinitions reference to the list of load definitions
   */
  explicit ParLoads(const std::vector<algo::LoadDefinition>& loadsDefinitions) : loadsDefinitions_(loadsDefinitions) {}

  /**
   * @brief enrich the parameter set collection for loads
   *
   * @param paramSetCollection parameter set collection to enrich
   * @param startingPointMode starting point mode
   */
  void write(const std::unique_ptr<parameters::ParametersSetCollection>& paramSetCollection,
              dfl::inputs::Configuration::StartingPointMode startingPointMode);

 private:
  /**
   * @brief create a new parameter set for loads
   *
   * @param startingPointMode starting point mode
   *
   * @return the new parameter set for loads
   */
  std::shared_ptr<parameters::ParametersSet> writeConstantLoadsSet(dfl::inputs::Configuration::StartingPointMode startingPointMode);

 private:
  std::vector<algo::LoadDefinition> loadsDefinitions_;  ///< list of loads definitions
};

}  // namespace outputs
}  // namespace dfl
