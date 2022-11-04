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
 * @file  Network.h
 *
 * @brief Dynaflow launcher Network model parameters file writer header file
 *
 */

#pragma once

#include "Configuration.h"
#include <PARParametersSet.h>

namespace dfl {
namespace outputs {
/**
 * @brief Network model parameters file writer
 */
class Network {
 public:
  /**
   * @brief Network model parameters file definition
   */
  struct NetworkDefinition {
    /**
     * @brief Constructor
     *
     * @param filename file path for output Network model parameters file (corresponds to basename)
     * @param startingPointMode starting point mode
     */
    NetworkDefinition(const boost::filesystem::path& filename,
                      dfl::inputs::Configuration::StartingPointMode startingPointMode) :
        filepath_(filename),
        startingPointMode_(startingPointMode) {}

    boost::filesystem::path filepath_;                                 ///< file path of the output file to write
    dfl::inputs::Configuration::StartingPointMode startingPointMode_;  ///< starting point mode
  };

  /**
   * @brief Constructor
   *
   * @param def Network model parameters file description
   */
  explicit Network(NetworkDefinition&& def);

  /**
   * @brief Export Network model parameters file
   */
  void write() const;

 private:
  /**
   * @brief create a new parameter set for network
   *
   * @return the new parameter set for network
   */
  boost::shared_ptr<parameters::ParametersSet> writeNetworkSet() const;

  NetworkDefinition def_;  ///< Network model parameters file description
};

}  // namespace outputs
}  // namespace dfl
