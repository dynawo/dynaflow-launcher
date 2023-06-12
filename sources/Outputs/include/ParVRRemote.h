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
 * @file  ParVRRemote.h
 *
 * @brief Dynaflow launcher PAR file writer for VRRemote
 *
 */

#pragma once

#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"

#include <PARParametersSetCollection.h>

namespace dfl {
namespace outputs {

/**
 * @brief VRRemote PAR file writer
 */
class ParVRRemote {
 public:
  /**
   * @brief Construct a new Par VRRemote object
   *
   * @param generatorDefinitions reference to the list of generator definitions
   * @param busesRegulatedBySeveralGenerators reference to map of bus ids to a generator that regulates them
   * @param hvdcDefinitions reference to the list of hvdc definitions
   */
  explicit ParVRRemote(const std::vector<algo::GeneratorDefinition> &generatorDefinitions,
                       const algo::GeneratorDefinitionAlgorithm::BusGenMap &busesRegulatedBySeveralGenerators, const algo::HVDCLineDefinitions &hvdcDefinitions)
      : generatorDefinitions_(generatorDefinitions), busesRegulatedBySeveralGenerators_(busesRegulatedBySeveralGenerators), hvdcDefinitions_(hvdcDefinitions) {}

  /**
   * @brief enrich the parameter set collection for VRRemote
   *
   * @param paramSetCollection parameter set collection to enrich
   */
  void writeVRRemotes(boost::shared_ptr<parameters::ParametersSetCollection> &paramSetCollection);

 private:
  const std::vector<algo::GeneratorDefinition> &generatorDefinitions_;                      ///< list of generators definitions
  const algo::GeneratorDefinitionAlgorithm::BusGenMap &busesRegulatedBySeveralGenerators_;  ///< map of bus ids to a generator that regulates them
  const algo::HVDCLineDefinitions &hvdcDefinitions_;                                        ///< list of Hvdc definitions
};

}  // namespace outputs
}  // namespace dfl
