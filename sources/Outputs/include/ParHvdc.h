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
 * @file  ParHvdc.h
 *
 * @brief Dynaflow launcher PAR file writer for hvdcs header file
 *
 */

#pragma once

#include "Configuration.h"
#include "HVDCDefinitionAlgorithm.h"
#include "OutputsConstants.h"

#include <PARParametersSetCollection.h>
#include <boost/shared_ptr.hpp>

namespace dfl {
namespace outputs {

/**
 * @brief Hvdcs PAR file writer
 */
class ParHvdc {
 public:
  /**
   * @brief Construct a new Par Hvdc object
   *
   * @param hvdcDefinitions reference to the list of hvdcs definitions
   */
  explicit ParHvdc(const algo::HVDCLineDefinitions& hvdcDefinitions) : hvdcDefinitions_(hvdcDefinitions) {}

  /**
   * @brief enrich the parameter set collection for hvdcs
   *
   * @param paramSetCollection parameter set collection to enrich
   * @param basename the basename for the simulation
   * @param dirname the dirname of the output directory
   * @param startingPointMode starting point mode
   * @param tFilterHvdc value for the time phase filtering of the hvdc in AC emulation
   */
  void write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection, const std::string& basename, const boost::filesystem::path& dirname,
             dfl::inputs::Configuration::StartingPointMode startingPointMode, const double tFilterHvdc);

 private:
  /**
   * @brief Write hvdc line parameter set
   *
   * @param hvdcLine the hvdc line definition to use
   * @param basename the basename for the simulation
   * @param dirname the dirname of the output directory
   * @param startingPointMode starting point mode
   * @param tFilterHvdc value for the time phase filtering of the hvdc in AC emulation
   *
   * @returns the parameter set
   */
  boost::shared_ptr<parameters::ParametersSet> writeHdvcLine(const algo::HVDCDefinition& hvdcLine, const std::string& basename,
                                                             const boost::filesystem::path& dirname,
                                                             dfl::inputs::Configuration::StartingPointMode startingPointMode, const double tFilterHvdc);

  /**
   * @brief Computes KAC emulation parameter
   *
   * @param droop droop data, in MW/deg
   * @returns KAC, in p.u/rad (base SnRef=100MW)
   */
  inline double computeKAC(double droop) {
    return droop * 1.8 / constants::pi_;
  }

  /**
   * @brief Computes pSet emulation parameter
   *
   * @param p0 p0 data, in MW
   * @returns pSet, in pu (base SnRef=100MW)
   */
  inline double computePSET(double p0) {
    return p0 / 100.;
  }

 private:
  const algo::HVDCLineDefinitions& hvdcDefinitions_;  ///< list of hvdcs definitions
};

}  // namespace outputs
}  // namespace dfl
