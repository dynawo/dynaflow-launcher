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

#include <JOBJobEntry.h>
#include <chrono>
#include <string>

namespace dfl {
/// @brief Namespace for outputs management
namespace outputs {

/**
 * @brief Jopb manager
 */
class Job {
 public:
  /**
   * @brief Job definition on DFL point of view
   */
  struct JobDefinition {
    /**
     * @brief Constructor
     *
     * @param filepath output filename
     * @param lvl dynawo log level
     */
    JobDefinition(const std::string& filepath, const std::string& lvl) : filename(filepath), dynawoLogLevel(lvl) {}

    std::string filename;        ///< filename of the job output file
    std::string dynawoLogLevel;  ///< Dynawo log level, in string representation
  };

 public:
  /**
  * @brief Export job file
  *
  * @param jobEntry the job entry to export
  * @param outputDir the output directory
  */
  static void exportJob(const boost::shared_ptr<job::JobEntry>& jobEntry, const std::string& networkFileEntry, const std::string& outputDir);

  /**
   * @brief Constructor
   *
   * @param def the job definition to use
   */
  explicit Job(JobDefinition&& def);

  /**
   * @brief Exports the job
   *
   * @returns a job entry
   */
  boost::shared_ptr<job::JobEntry> write();

 private:
  static const std::chrono::seconds timeStart_;     ///< The constant start time of the simulation of the job
  static const std::chrono::seconds durationSimu_;  ///< the constant duration of the simulation in the job
  static const std::string solverName_;             ///< The solver name used during the simulation
  static const std::string solverFilename_;         ///< The solver filename
  static const std::string solverParId_;            ///< The parameter id in the .par file corresponding to the solver parameters

 private:
  /**
   * @brief Write the solver element of the job file in formatter
   *
   * @returns solver entry to add to the job entry
   */
  boost::shared_ptr<job::SolverEntry> writeSolver();

  /**
   * @brief Write the modeler element of the job file in formatter
   *
   * @returns modeler entry to add to the job
   */
  boost::shared_ptr<job::ModelerEntry> writeModeler();

  /**
   * @brief Write the simulation element of the job file in formatter
   *
   * @returns simulation entry to add to the job
   */
  boost::shared_ptr<job::SimulationEntry> writeSimulation();

  /**
   * @brief Write the outputs element of the job file in formatter
   *
   * @returns outputs entry to add to the job
   */
  boost::shared_ptr<job::OutputsEntry> writeOutputs();

 private:
  JobDefinition def_;  ///< the job definition to use
};

}  // namespace outputs
}  // namespace dfl
