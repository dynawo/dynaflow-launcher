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
 * @file  Solver.h
 *
 * @brief Dynaflow launcher solver file writer header file
 *
 */

#pragma once

#include "Configuration.h"

#include <PARParametersSet.h>
#include <boost/shared_ptr.hpp>
#include <string>

namespace dfl {
namespace outputs {
/**
 * @brief solver par file writer
 */
class Solver {
 public:
  /**
   * @brief solver par definition
   */
  struct SolverDefinition {
    /**
     * @brief Construct a new Solver Definition object
     *
     * @param config input configuration
     */
    explicit SolverDefinition(const dfl::inputs::Configuration& config) : outputDir_(config.outputDir()), timeStep_(config.getTimeStep()) {}

    boost::filesystem::path outputDir_;  ///< directory for output files
    double timeStep_;                    ///< maximum value of the solver timestep
  };

  /**
   * @brief Constructor
   *
   * @param def reference to SolverDefinition object
   */
  explicit Solver(SolverDefinition&& def);

  /**
   * @brief Export solver par file
   */
  void write() const;

 private:
  /**
  * @brief creates the parameter set for solver
  *
  * @return reference to the new created parameter set
  */
  boost::shared_ptr<parameters::ParametersSet> writeSolverSet() const;

  SolverDefinition def_;  ///< solver par definition
};

}  // namespace outputs
}  // namespace dfl
