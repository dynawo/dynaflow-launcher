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
 * @brief Dynaflow launcher common to all writers
 *
 */

#pragma once

#include "Algo.h"
#include "Node.h"

#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfl {
namespace outputs {
/// @brief Namespace for constant variables common to all writers
namespace constants {

/// @brief Alias for map of references to shunts by connected bus id
using RefShuntsByIdMap = std::unordered_map<inputs::Shunt::BusId, std::vector<std::reference_wrapper<const inputs::Shunt>>>;

/// @brief Alias for reference on const shunt
using ConstShuntRef = std::reference_wrapper<const inputs::Shunt>;
/// @brief Hash for shunts references
struct ShuntRefHash {
  /**
     * @brief Retrieve the hash value
     *
     * @param shunt the shunt to hash
     * @return the hash value
     */
  size_t operator()(const ConstShuntRef& shunt) const noexcept;
};
/// @brief Operator equal definition for shunt references
struct ShuntRefEqual {
  /**
     * @brief Determine if two references to a shunt are the same
     *
     * They are equal if the embedded shunts are equal
     *
     * @param lhs left comparee
     * @param rhs right comparee
     * @return true if both referenced shunts are equal, false if not
     */
  bool operator()(const ConstShuntRef& lhs, const ConstShuntRef& rhs) const;
};
/// @brief Alias for set of references to shunt
using ShuntsRefSet = std::unordered_set<ConstShuntRef, ShuntRefHash, ShuntRefEqual>;  ///< Alias for set of reference to shunts

/**
 * @brief Return a hash number from a string as input
 * @param str The string that will serve as input for the hash function
 * @return The hash as a number
 */
static inline std::size_t
hash(const std::string& str) {
  return std::hash<std::string>{}(str);
}

/**
 * @brief Computes Qmax
 *
 * powerFactor = PMax / sqrt(PMax^2 + QMax^2) => QMax = PMax * sqrt(1/powerFactor^2 - 1)
 *
 * @param powerFactor the power factor to use
 * @param pMax the PMax to use
 * @returns the corresponding QMax value
 */
static inline double
computeQmax(double powerFactor, double pMax) {
  return pMax * std::sqrt((1. / (powerFactor * powerFactor) - 1));
}

/**
 * @brief Return the filename of a diagram file
 *
 * @param id the element id with the diagram name
 * @return The string filename of the diagram file
 */
std::string diagramFilename(const std::string& id);

/**
 * @brief Retrieve the shunt regulation id
 *
 * @param busId the connected bus id of the shunt
 * @return the computed shunt regulation id
 */
static inline std::string
computeShuntRegulationId(const std::string& busId) {
  return "ShuntRegulation_" + busId;
}

/**
 * @brief Compute the list of shunts, sorting by connected bus id
 *
 * @param shuntDefinitions the shund definitions to use
 * @return the sorted list of shunts
 */
RefShuntsByIdMap computeFilteredShuntsByIds(const algo::ShuntDefinitions& shuntDefinitions);

const std::string networkModelName{"NETWORK"};                                    ///< Name of the model corresponding to network
const std::string loadParId{"GenericRestorativeLoad"};                            ///< PAR id common to all loads
const std::string diagramDirectorySuffix{"_Diagram"};                             ///< Suffix for the diagram directory
const std::string diagramMaxTableSuffix{"_tableqmax"};                            ///< Suffix for the table name for qmax in diagram file
const std::string diagramMinTableSuffix{"_tableqmin"};                            ///< Suffix for the table name for qmin in diagram file
const std::string signalNGeneratorParId{"signalNGenerator"};                      ///< PAR id for generators using signal N
const std::string signalNGeneratorFixedPParId{"signalNGeneratorFixedP"};          ///< PAR id for generators using signal N with fixed P
const std::string propSignalNGeneratorParId{"propSignalNGenerator"};              ///< PAR id for generators using prop signal N
const std::string propSignalNGeneratorFixedPParId{"propSignalNGeneratorFixedP"};  ///< PAR id for generators using prop signal N with fixed P
const std::string remoteVControlParId{"remoteVControl"};                          ///< PAR id for using remote voltage control
const std::string remoteSignalNGeneratorFixedP{"remoteSignalNFixedP"};            ///< PAR id for using remote signal N with fixed P
const std::string xmlEncoding{"UTF-8"};                                           ///< Default encoding for XML outputs files
const std::string diagramTableBPu{"tableBPu"};                                    ///< Name of the table in susceptance sections diagram files

constexpr double powerValueMax = std::numeric_limits<double>::max();  ///< Maximum value for powers, meaning infinite

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
