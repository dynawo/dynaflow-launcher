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
 * @file  ParNetwork.h
 *
 * @brief Dynaflow launcher PAR file writer for Network header file
 *
 */

#pragma once

#include <PARParametersSetCollection.h>
#include <boost/shared_ptr.hpp>
#include "Configuration.h"

namespace dfl {
namespace outputs {

/**
 * @brief network PAR file writer
 */
class ParNetwork {
 public:
    /**
    * @brief enrich the parameter set collection for network
    *
    * @param paramSetCollection parameter set collection to enrich
    * @param startingPointMode starting point mode
    */
    void write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection,
                                 dfl::inputs::Configuration::StartingPointMode startingPointMode);

 private:
  /**
   * @brief create a new parameter set for network
   *
   * @param startingPointMode starting point mode
   *
   * @return the new parameter set for network
   */
  boost::shared_ptr<parameters::ParametersSet> writeNetworkSet(dfl::inputs::Configuration::StartingPointMode startingPointMode);
};

}  // namespace outputs
}  // namespace dfl
