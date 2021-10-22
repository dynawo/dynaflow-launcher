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
     * @param filename output filename
     * @param lvl dynawo log level
     */
    JobDefinition(const std::string& filename, const std::string& lvl) : filename(filename), dynawoLogLevel(lvl), contingencyId{}, baseFilename{} {}

    /**
     * @brief Constructor with a reference to a contingency
     *
     * @param filename output filename
     * @param lvl dynawo log level
     * @param contingencyId contingency identifier
     * @param baseFilename filename of base case
     */
    JobDefinition(const std::string& filename, const std::string& lvl, const std::string& contingencyId, const std::string& baseFilename) :
        filename(filename),
        dynawoLogLevel(lvl),
        contingencyId(contingencyId),
        baseFilename(baseFilename) {}
    std::string filename;        ///< filename of the job output file
    std::string dynawoLogLevel;  ///< Dynawo log level, in string representation
    std::string contingencyId;   ///< Identifier of referred contingency
    std::string baseFilename;    ///< Name for base case filename if we are defining a jobs file for a contingency
  };

 public:
  /**
  * @brief Export job file
  *
  * @param jobEntry the job entry to export
  * @param networkFileEntry path to the input network file
  * @param outputDir the output directory
  */
  static void exportJob(const boost::shared_ptr<job::JobEntry>& jobEntry, const boost::filesystem::path& networkFileEntry,
                        const boost::filesystem::path& outputDir);

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
  boost::shared_ptr<job::JobEntry> write() const;

  /**
   * Sets the start and duration time of all jobs
   */
  static void setStartAndDuration(double startTime, double durationTime) {
    timeStart_ = startTime;
    durationSimu_ = durationTime;
  }

 private:
  static double timeStart_;                  ///< The start time of the simulation of the job
  static double durationSimu_;               ///< the duration of the simulation in the job
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
  static constexpr bool exportIIDMFile_ = true;     ///< export IIDM file in job entry and file
  static constexpr bool exportDumpFile_ = false;    ///< export dump file in job entry and file

 private:
  JobDefinition def_;  ///< the job definition to use
};

}  // namespace outputs
}  // namespace dfl
