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

#include "Algo.h"
#include "Configuration.h"
#include "Constants.h"
#include "Contingencies.h"

#include <PARParametersSet.h>
#include <boost/shared_ptr.hpp>
#include <chrono>
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
     * @brief Constructor
     *
     * @param base basename for current simulation
     * @param filename file path for output PAR file (corresponds to basename)
     * @param contingency contingency dfinition for the event parameters
     * @param shuntsWithSections set of shunts with sections
     * @param timeOfEvent time of event
     */
    ParEventDefinition(const std::string& base, const std::string& filename, const inputs::Contingency& contingency,
                       const outputs::constants::ShuntsRefSet& shuntsWithSections, const std::chrono::seconds& timeOfEvent) :
        basename(base),
        filename(filename),
        contingency(contingency),
        shuntsWithSections(shuntsWithSections),
        timeOfEvent(timeOfEvent) {}

    std::string basename;                                        ///< basename
    std::string filename;                                        ///< filename of the output file to write
    const inputs::Contingency& contingency;                      ///< contingency definition for the event parameters
    const outputs::constants::ShuntsRefSet& shuntsWithSections;  ///< set of shunts with sections
    std::chrono::seconds timeOfEvent;                            ///< time of event
  };

  /**
   * @brief Constructor
   *
   * @param def PAR event file definition
   */
  explicit ParEvent(ParEventDefinition&& def);

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
  static boost::shared_ptr<parameters::ParametersSet> buildBranchDisconnection(const std::string& branchId, const std::chrono::seconds& timeOfEvent);

  /**
   * @brief Build element disconnection parameter set for EventSetPointBoolean dynamic model
   *
   * @param elementId the identifier of the element
   * @param timeOfEvent time of event
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> buildEventSetPointBooleanDisconnection(const std::string& elementId,
                                                                                             const std::chrono::seconds& timeOfEvent);

  /**
   * @brief Build element disconnection parameter set for EventSetPointReal dynamic model
   *
   * @param elementId the identifier of the element
   * @param timeOfEvent time of event
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> buildEventSetPointRealDisconnection(const std::string& elementId,
                                                                                          const std::chrono::seconds& timeOfEvent);

 private:
  ParEventDefinition def_;  ///< PAR event file definition
};

}  // namespace outputs
}  // namespace dfl
