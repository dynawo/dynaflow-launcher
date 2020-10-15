//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
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
 * @brief Dynaflow launcher constants common to all writers
 *
 */

#pragma once

#include <limits>
#include <string>

namespace dfl {
namespace outputs {
/// @brief Namespace for constant variables common to all writers
namespace constants {
const std::string loadParId{"GenericRestorativeLoad"};              ///< PAR id common to all loads
const std::string signalNParId{"SignalN"};                          ///< PAR id for signalN
const std::string diagramFileSuffixExt{"_Diagram.txt"};             ///< Basename for QTables diagrams
const std::string diagramMaxTableSuffix{"_tableqmax"};              ///< Suffix for the table name for qmax in diagram file
const std::string diagramMinTableSuffix{"_tableqmin"};              ///< Suffix for the table name for qmin in diagram file
const std::string signalNGeneratorParId{"signalNGenerator"};        ///< PAR id for generators using signal N
const std::string impSignalNGeneratorParId{"impSignalNGenerator"};  ///< PAR id for generators using signal N with impendance

constexpr double powerValueMax = std::numeric_limits<double>::max();  ///< Maximum value for powers, meaning infinite

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
