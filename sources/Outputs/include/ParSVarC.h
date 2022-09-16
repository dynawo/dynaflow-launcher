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
 * @file  ParSVarC.h
 *
 * @brief Dynaflow launcher PAR file writer for static var compensators header file
 *
 */

#pragma once

#include "SVarCDefinitionAlgorithm.h"
#include "Configuration.h"

#include <PARParametersSetCollection.h>
#include <boost/shared_ptr.hpp>

namespace dfl {
namespace outputs {

/**
 * @brief static var compensators PAR file writer
 */
class ParSVarC {
 public:
  /**
   * @brief Construct a new Par SVarCs object
   *
   * @param svarcsDefinitions reference to the list of SVarCs definitions
   */
  explicit ParSVarC(const std::vector<algo::StaticVarCompensatorDefinition>& svarcsDefinitions) : svarcsDefinitions_(svarcsDefinitions) {}

  /**
   * @brief enrich the parameter set collection for SvarCs
   *
   * @param paramSetCollection parameter set collection to enrich
   * @param startingPointMode starting point mode
   */
  void write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection,
              dfl::inputs::Configuration::StartingPointMode startingPointMode);

 private:
  /**
   * @brief Write the macro parameter set used for static var compensators
   * @param startingPointMode starting point mode
   * @returns the macro parameter set for SVarC
   */
  boost::shared_ptr<parameters::MacroParameterSet> writeMacroParameterSetStaticVarCompensators(dfl::inputs::Configuration::StartingPointMode startingPointMode);

  /**
   * @brief Write parameter set for static var compensator
   * @param svarc the static var compensator to use
   * @returns the parameter set to add to the exported file
   */
  boost::shared_ptr<parameters::ParametersSet> writeStaticVarCompensator(const algo::StaticVarCompensatorDefinition& svarc);

  /**
   * @brief Computes the susceptance value in PU unit
   *
   * bMaxPU = B * Vnom² / Sb
   *
   * @param b the susceptance value to convert
   * @param VNom the nominal voltage of the corresponding SVarC
   * @returns the converted susceptance value
   */
  inline double computeBPU(double b, double VNom) {
    return b * VNom * VNom / Sb_;
  }

 private:
  std::vector<algo::StaticVarCompensatorDefinition> svarcsDefinitions_;  ///< list of SVarCs definitions
  static const std::string macroParameterSetStaticCompensator_;          ///< Name of the macro parameter set for static var compensator
  static constexpr double svarcThresholdDown_ = 0.;                      ///< time threshold down for SVarC
  static constexpr double svarcThresholdUp_ = 60.;                       ///< time threshold up for SVarC
  static constexpr double Sb_ = 100;                                     ///< Sb value
};

}  // namespace outputs
}  // namespace dfl
