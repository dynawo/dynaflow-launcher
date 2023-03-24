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
 * @file  Job.h
 *
 * @brief Dynaflow launcher job exporter header file
 *
 */

#pragma once

#include "Configuration.h"

#include <JOBJobEntry.h>
#include <boost/optional.hpp>
#include <string>

namespace dfl {
/// @brief Namespace for outputs management
namespace outputs {

/**
 * @brief Job manager
 */
class Job {
 public:
  /**
   * @brief Job definition on DFL point of view
   */
  class JobDefinition {
   public:
    /**
     * @brief Constructor
     *
     * @param filename output filename
     * @param lvl dynawo log level
     * @param config configuration
     */
    JobDefinition(const std::string &filename, const std::string &lvl, const dfl::inputs::Configuration &config) :
        filename(filename),
        dynawoLogLevel(lvl),
        configuration(config) {}

    /**
     * @brief Constructor with a reference to a contingency
     *
     * @param filename output filename
     * @param lvl dynawo log level
     * @param config configuration
     * @param contingencyId contingency identifier
     * @param baseFilename filename of base case
     */
    JobDefinition(const std::string &filename, const std::string &lvl, const dfl::inputs::Configuration &config, const std::string &contingencyId,
                  const std::string &baseFilename) :
        filename(filename),
        dynawoLogLevel(lvl),
        configuration(config),
        contingencyId(contingencyId),
        baseFilename(baseFilename) {}

    std::string filename;                             ///< filename of the job output file
    std::string dynawoLogLevel;                       ///< Dynawo log level, in string representation
    const dfl::inputs::Configuration &configuration;  ///< Simulation configuration
    boost::optional<std::string> contingencyId;       ///< Identifier of referred contingency, only for security analysis jobs
    boost::optional<std::string> baseFilename;        ///< Name for base case filename if we are defining a jobs file for a contingency
  };

 public:
  /**
   * @brief Export job file
   *
   * @param jobEntry the job entry to export
   * @param networkFileEntry path to the input network file
   * @param config configuration
   */
  static void exportJob(const boost::shared_ptr<job::JobEntry> &jobEntry, const boost::filesystem::path &networkFileEntry,
                        const dfl::inputs::Configuration &config);
  /**
   * @brief Constructor
   *
   * @param def the job definition to use
   */
  explicit Job(JobDefinition &&def);

  /**
   * @brief Exports the job
   *
   * @returns a job entry
   */
  boost::shared_ptr<job::JobEntry> write() const;

 private:
  static const std::string solverName_;      ///< The solver name used during the simulation
  static const std::string solverFilename_;  ///< The solver filename
  static const std::string solverParId_;     ///< The parameter id in the .par file corresponding to the solver parameters

 private:
  /**
   * @brief Write the solver element of the job file in formatter
   *
   * @returns solver entry to add to the job entry
   */
  boost::shared_ptr<job::SolverEntry> writeSolver() const;

  /**
   * @brief Write the modeler element of the job file in formatter
   *
   * @returns modeler entry to add to the job
   */
  boost::shared_ptr<job::ModelerEntry> writeModeler() const;

  /**
   * @brief Write the simulation element of the job file in formatter
   *
   * @returns simulation entry to add to the job
   */
  boost::shared_ptr<job::SimulationEntry> writeSimulation() const;

  /**
   * @brief Write the outputs element of the job file in formatter
   *
   * @returns outputs entry to add to the job
   */
  boost::shared_ptr<job::OutputsEntry> writeOutputs() const;

 private:
  static constexpr bool useStandardModels_ = true;  ///< use standard models in job entry and file

 private:
  JobDefinition def_;  ///< the job definition to use
};

}  // namespace outputs
}  // namespace dfl
