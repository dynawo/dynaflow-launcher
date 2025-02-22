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
 * @file  ParEvent.h
 *
 * @brief Dynaflow launcher PAR file writer for events in a contingency header file
 *
 */

#pragma once

#include "Configuration.h"
#include "Contingencies.h"
#include "ContingencyValidationAlgorithm.h"
#include "DynModelDefinitionAlgorithm.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "LoadDefinitionAlgorithm.h"
#include "MainConnexComponentAlgorithm.h"
#include "SVarCDefinitionAlgorithm.h"
#include "ShuntDefinitionAlgorithm.h"
#include "SlackNodeAlgorithm.h"

#include <PARParametersSet.h>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfl {
namespace outputs {
/**
 * @brief PAR event file writer
 */
class ParEvent {
 public:
  /**
   * @brief PAR event file definition
   */
  struct ParEventDefinition {
    /**
     * @brief Construct a new Par Event Definition object
     *
     * @param base basename for current simulation
     * @param filename file path for output PAR file (corresponds to basename)
     * @param contingency contingency definition for the event parameters
     * @param networkElements set of contingency elements ids using network model
     * @param timeOfEvent time of event
     */
    ParEventDefinition(const std::string &base, const std::string &filename, const inputs::Contingency &contingency,
                       const std::unordered_set<std::string> &networkElements, const double timeOfEvent)
        : basename(base), filename(filename), contingency(contingency), networkElements_(networkElements), timeOfEvent(timeOfEvent) {}

    std::string basename;                                    ///< basename
    std::string filename;                                    ///< filename of the output file to write
    const inputs::Contingency &contingency;                  ///< contingency definition for the event parameters
    const std::unordered_set<std::string> networkElements_;  ///< set of contingency elements ids using network model
    double timeOfEvent;                                      ///< time of event
  };

  /**
   * @brief Constructor
   *
   * @param def PAR event file definition
   */
  explicit ParEvent(ParEventDefinition &&def);

  /**
   * @brief Export PAR event file
   */
  void write();

 private:
  /**
   * @brief Build branch disconnection parameter set
   *
   * @param branchId the identifier of the branch
   * @param timeOfEvent time of event
   *
   * @returns the parameter set
   */
  static std::shared_ptr<parameters::ParametersSet> buildBranchDisconnection(const std::string &branchId, const double timeOfEvent);

  /**
   * @brief Build element disconnection parameter set for EventSetPointBoolean dynamic model
   *
   * @param elementId the identifier of the element
   * @param timeOfEvent time of event
   *
   * @returns the parameter set
   */
  static std::shared_ptr<parameters::ParametersSet> buildEventSetPointBooleanDisconnection(const std::string &elementId, const double timeOfEvent);

  /**
   * @brief Build element disconnection parameter set for EventConnectedStatus dynamic model
   *
   * @param elementId the identifier of the element
   * @param timeOfEvent time of event
   *
   * @returns the parameter set
   */
  static std::shared_ptr<parameters::ParametersSet> buildEventConnectedStatusDisconnection(const std::string &elementId, const double timeOfEvent);

  /**
   * @brief Determines if contingency element is using a network cpp model
   *
   * @param elementId static identifier of the equipment
   *
   * @returns true if contingency element is using a network cpp model
   */
  bool isNetwork(const std::string &elementId) const;

 private:
  ParEventDefinition def_;  ///< PAR event file definition
};

}  // namespace outputs
}  // namespace dfl
