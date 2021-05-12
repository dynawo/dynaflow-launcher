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
#include "Contingencies.h"

#include <PARParametersSet.h>
#include <boost/shared_ptr.hpp>
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
     * @param contingency contingency definition for the event parameters
     * @param timeEvent time of event
     */
    ParEventDefinition(const std::string& base, const std::string& filename,
            const inputs::Contingencies::ContingencyDefinition& contingency,
            const double& timeEvent) :
        basename(base),
        filename(filename),
        contingency(contingency),
        timeEvent(timeEvent) {}

    std::string basename;                                      ///< basename
    std::string filename;                                      ///< filename of the output file to write
    // TODO(Luma) use shared_ptr<Contingency...>?
    inputs::Contingencies::ContingencyDefinition contingency;  ///< contingency definition for the event parameters
    double timeEvent;                                          ///< Time of event
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
   * @brief Write branch disconnection parameter set
   *
   * @param branchId the identifier of the branch
   * @param timeEvent time of event
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeBranchDisconnection(const std::string& branchId, double timeEvent);

  /**
   * @brief Write element disconnection parameter set
   *
   * @param elementId the identifier of the element (can be a generator, load, shunt, ...)
   * @param timeEvent time of event
   *
   * @returns the parameter set
   */
  static boost::shared_ptr<parameters::ParametersSet> writeElementDisconnection(const std::string& elementId, double timeEvent);

 private:
  ParEventDefinition def_;  ///< PAR event file definition
};

}  // namespace outputs
}  // namespace dfl
