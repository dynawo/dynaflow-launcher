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
 * @file Constants.h
 *
 * @brief Dynaflow launcher common
 *
 */

#pragma once

#include <cmath>
#include <limits>
#include <string>

namespace dfl {
namespace common {
namespace constants {

const std::string diagramDirectorySuffix{"_Diagram"};              ///< Suffix for the diagram directory
const std::string svcModelName{"SecondaryVoltageControlSimp"};     ///< name of the Secondary Voltage Controller model
const std::string rpcl2PropertyName{"ReactivePowerControlLoop2"};  ///< name of the property to annote RPCL2 generators in assembling

}  // namespace constants
}  // namespace common
}  // namespace dfl
