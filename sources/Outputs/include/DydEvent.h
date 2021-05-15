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
 * @file  DydEvent.h
 *
 * @brief Dynaflow launcher DYD file writer for events header file
 *
 */

#pragma once

#include "Algo.h"
#include "Contingencies.h"

#include <DYDBlackBoxModel.h>
#include <DYDDynamicModelsCollection.h>
#include <DYDMacroConnect.h>
#include <DYDMacroConnection.h>
#include <DYDMacroConnector.h>
#include <DYDMacroStaticReference.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfl {
namespace outputs {

/**
 * @brief DYD file writer for events
 */
class DydEvent {
 public:
  /**
   * @brief Dyd event definition to provide informations to build the Dyd file
   */
  struct DydEventDefinition {
    /**
     * @brief Constructor
     *
     * @param base the basename for current file (corresponds to filepath basename)
     * @param filepath the filepath of the dyd file to write
     * @param contingency definition of the contingency for which we have to create a DYD file
     */
    DydEventDefinition(const std::string& base, const std::string& filepath, const inputs::Contingencies::ContingencyDefinition& contingency) :
        basename(base),
        filename(filepath),
        contingency(contingency) {}

    std::string basename;     ///< basename for file
    std::string filename;     ///< filepath for file to write
    // TODO(Luma) use a shared_ptr<ContingencyDefinition> ...
    inputs::Contingencies::ContingencyDefinition contingency;  ///< definition of the contingency
  };

  /**
   * @brief Constructor
   *
   * @param def the dyd definition
   */
  explicit DydEvent(DydEventDefinition&& def);

  /**
   * @brief Write the dyd file
   */
  void write();

 private:
  /**
   * @brief Write macro connectors
   *
   * Create macro connectors elements
   *
   * @returns list of macro connectors
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroConnector>> buildMacroConnectors();

  /**
   * @brief Create black box model for disconnecting a branch
   *
   * @param branchId static identifier of the branch
   * @param basename basename for file
   *
   * @returns model for the branch disconnection event
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> buildBranchDisconnection(const std::string& branchId, const std::string& basename);

  /**
   * @brief Create connections for branch disconnection events
   *
   * Use macro connection
   *
   * @param branchId the static id of the branch to process
   *
   * @returns the macro connection elements
   */
  static boost::shared_ptr<dynamicdata::MacroConnect> buildBranchDisconnectionConnect(const std::string& branchId);

  /**
   * @brief Create black box model for disconnecting an equipment through a switchOffSignal
   *
   * @param elementId static identifier of the equipment
   * @param basename basename for file
   *
   * @returns model for the equipment disconnection event
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> buildSwitchOffSignalDisconnection(const std::string& elementId, const std::string& basename);

  /**
   * @brief Adds connections for disconnection events that use a switch off signal
   *
   * @param dynamicModels the list of dynamic models where the connections will be added
   * @param elementId the static id of the equipment to process
   * @param var2Prefix the prefix of the var2 name in the connect depends on the type of equipment ("generator", ...)
   *
   */
  static void addSwitchOffSignalDisconnectionConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModels,
    const std::string& elementId,
    const std::string& var2Prefix);

  /**
   * @brief Create black box model for disconnecting an equipment through a change in Network state
   *
   * @param elementId static identifier of the equipment
   * @param basename basename for file
   *
   * @returns model for the equipment disconnection event
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> buildNetworkStateDisconnection(const std::string& elementId, const std::string& basename);

  /**
   * @brief Adds connections for disconnection events that use a change in Network state
   *
   * @param dynamicModels the list of dynamic models where the connections will be added
   * @param elementId the static id of the equipment to process
   *
   */
  static void addNetworkStateDisconnectionConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModels, const std::string& elementId);

 private:
  static const std::string networkModelName_;                       ///< name of the model corresponding to network

 private:
  DydEventDefinition def_;  ///< Dyd file information
};
}  // namespace outputs
}  // namespace dfl
