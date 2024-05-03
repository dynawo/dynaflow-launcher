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
   * @param busesToNumberOfRegulationMap mapping of busId and the number of generators/VSCs that regulates them
   * @param hvdcDefinitions reference to the list of Hvdc definitions
   */
  explicit DydVRRemote(const std::vector<algo::GeneratorDefinition> &generatorDefinitions,
                       const inputs::NetworkManager::BusMapRegulating &busesToNumberOfRegulationMap, const algo::HVDCLineDefinitions &hvdcDefinitions)
      : generatorDefinitions_(generatorDefinitions), busesToNumberOfRegulationMap_(busesToNumberOfRegulationMap), hvdcDefinitions_(hvdcDefinitions) {}

  /**
   * @brief enrich the dynamic black models set collection for VRRemote
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   */
  void writeVRRemotes(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const std::string &basename);

 private:
  /**
   * @brief add the macroconnectors for VRRemotes
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect);

  /**
   * @brief write the macroconnects and connects for VRRemotes
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   */
  void writeConnections(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const std::string &basename);

  const std::string macroConnectorGenVRRemoteName_{"GEN_VRREMOTE_CONNECTOR"};               ///< name for the macro connector for generators
  const std::string macroConnectorHvdcVRRemoteSide1Name_{"HVDC_VRREMOTE_CONNECTOR_SIDE1"};  ///< name for the macro connector for side 1 of hvdcs
  const std::string macroConnectorHvdcVRRemoteSide2Name_{"HVDC_VRREMOTE_CONNECTOR_SIDE2"};  ///< name for the macro connector for side 2 of hvdcs
  const std::vector<algo::GeneratorDefinition> &generatorDefinitions_;                      ///< list of generators definitions
  const inputs::NetworkManager::BusMapRegulating &busesToNumberOfRegulationMap_;  ///< mapping of busId and the number of generators that regulates them
  const algo::HVDCLineDefinitions &hvdcDefinitions_;                              ///< list of Hvdc definitions
};

}  // namespace outputs
}  // namespace dfl
