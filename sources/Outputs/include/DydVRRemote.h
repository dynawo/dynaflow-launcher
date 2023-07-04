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
 * @file  DydVRRemote.h
 *
 * @brief Dynaflow launcher DYD file writer for VRRemote
 *
 */

#pragma once

#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"

#include <DYDDynamicModelsCollection.h>

namespace dfl {
namespace outputs {

/**
 * @brief VRRemote DYD file writer
 */
class DydVRRemote {
 public:
  /**
   * @brief Construct a new DydVRRemote object
   *
   * @param generatorDefinitions reference to the list of generator definitions
   * @param busesRegulatedBySeveralGenerators map of bus ids to a generator that regulates them
   * @param hvdcDefinitions reference to the list of Hvdc definitions
   */
  explicit DydVRRemote(const std::vector<algo::GeneratorDefinition>& generatorDefinitions,
                        const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesRegulatedBySeveralGenerators,
                        const algo::HVDCLineDefinitions& hvdcDefinitions) :
    generatorDefinitions_(generatorDefinitions),
    busesRegulatedBySeveralGenerators_(busesRegulatedBySeveralGenerators),
    hvdcDefinitions_(hvdcDefinitions) {}

  /**
   * @brief enrich the dynamic black models set collection for VRRemote
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   */
  void writeVRRemotes(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
                      const std::string& basename);

 private:
  /**
   * @brief add the macroconnectors for VRRemotes
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief write the macroconnects and connects for VRRemotes
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeConnections(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  const std::string macroConnectorGenVRRemoteName_{"GEN_VRREMOTE_CONNECTOR"};               ///< name for the macro connector for generators
  const std::vector<algo::GeneratorDefinition>& generatorDefinitions_;                      ///< list of generators definitions
  const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesRegulatedBySeveralGenerators_;  ///< map of bus ids to a generator that regulates them
  const algo::HVDCLineDefinitions& hvdcDefinitions_;                                        ///< list of Hvdc definitions
};

}  // namespace outputs
}  // namespace dfl
