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
 * @file OutputsConstants.h
 *
 * @brief Dynaflow launcher common to all writers
 *
 */

#pragma once

#include <cmath>
#include <limits>
#include <string>

namespace dfl {
namespace outputs {
/// @brief Namespace for constant variables common to all writers
namespace constants {

/**
 * @brief Return an uuid from a string as input
 * @param str The string that will serve as input for uuid generation
 * @return The uuid as a string
 */
std::string uuid(const std::string& str);

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
  return pMax * std::sqrt(1. / (powerFactor * powerFactor) - 1);
}

/**
 * @brief Return the filename of a diagram file
 *
 * @param id the element id with the diagram name
 * @return The string filename of the diagram file
 */
std::string diagramFilename(const std::string& id);

const std::string networkModelName{"NETWORK"};                               ///< Name of the model corresponding to network
const std::string loadParId{"GenericRestorativeLoad"};                       ///< PAR id common to all loads
const std::string diagramMaxTableSuffix{"_tableqmax"};                       ///< Suffix for the table name for qmax in diagram file
const std::string diagramMinTableSuffix{"_tableqmin"};                       ///< Suffix for the table name for qmin in diagram file
const std::string signalNGeneratorParId{"signalNGenerator"};                 ///< PAR id for generators using signal N
const std::string signalNTfoGeneratorParId{"signalNTfoGenerator"};           ///< PAR id for generators using signal N and transformer
const std::string signalNGeneratorParIdRect{"signalNGeneratorRectangular"};  ///< PAR id for generators using signal N with rectangular diagram
const std::string signalNGeneratorFixedPParId{"signalNGeneratorFixedP"};     ///< PAR id for generators using signal N with fixed P
const std::string networkParId{"Network"};                                   ///< PAR id for Network
const std::string signalNGeneratorFixedPParIdRect{
    "signalNGeneratorFixedPRectangular"};                             ///< PAR id for generators using signal N with fixed P and rectangular diagram
const std::string propSignalNGeneratorParId{"propSignalNGenerator"};  ///< PAR id for generators using prop signal N
const std::string propSignalNGeneratorParIdRect{"propSignalNGeneratorRectangular"};  ///< PAR id for generators using prop signal N with rectangular diagram
const std::string propSignalNGeneratorFixedPParId{"propSignalNGeneratorFixedP"};     ///< PAR id for generators using prop signal N with fixed P
const std::string propSignalNGeneratorFixedPParIdRect{
    "propSignalNGeneratorFixedPRectangular"};                            ///< PAR id for generators using prop signal N with fixed P and rectangular diagram
const std::string remoteVControlParId{"remoteVControl"};                 ///< PAR id for using remote voltage control
const std::string remoteVControlParIdRect{"remoteVControlRectangular"};  ///< PAR id for using remote voltage control with rectangular diagram
const std::string remoteSignalNGeneratorFixedP{"remoteSignalNFixedP"};   ///< PAR id for using remote signal N with fixed P
const std::string remoteSignalNGeneratorFixedPRect{
    "remoteSignalNFixedPRectangular"};                                ///< PAR id for using remote signal N with fixed P and rectangular diagram
const std::string xmlEncoding{"UTF-8"};                               ///< Default encoding for XML outputs files
const std::string modelSignalNQprefix_{"Model_Signal_NQ_"};           ///< Prefix for SignalN models
constexpr double powerValueMax = std::numeric_limits<double>::max();  ///< Maximum value for powers, meaning infinite
static constexpr double pi_ = M_PI;                                   ///< PI value
static constexpr double kGoverNullValue_ = 0.;                        ///< KGover null value
static constexpr double kGoverDefaultValue_ = 1.;                     ///< KGover default value
const std::string solverParFileName{"solver.par"};                    ///< name of the solver par file
const std::string componentTransformerIdTag{"@TFO@"};                 ///< TFO special tag for component id
const std::string seasonTag{"@SAISON@"};                              ///< Season special tag
const double generatorXPuValue{0.1228};                               ///< value of XPu of generators
const double generatorNucXPuValue{0.1426};                            ///< value of XPu of nuclear generators

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
