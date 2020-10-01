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
 * @brief Dynaflow launcher job file writer header file
 *
 */

#pragma once

#include <chrono>
#include <string>
#include <xml/sax/formatter/Formatter.h>

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
     * @param dir the output directory for job
     * @param filepath output filename
     * @param lvl dynawo log level
     */
    JobDefinition(const std::string& dir, const std::string& filepath, const std::string& lvl) : dirname(dir), filename(filepath), dynawoLogLevel(lvl) {}

    std::string dirname;         ///< output directory
    std::string filename;        ///< filename of the job output file
    std::string dynawoLogLevel;  ///< Dynawo log level, in string representation
  };

  /**
   * @brief Constructor
   *
   * @param def the job definition to use
   */
  explicit Job(const JobDefinition& def);

  /**
   * @brief Write the output job file
   */
  void write();

 private:
  static const std::chrono::seconds timeStart_;     ///< The constant start time of the simulation of the job
  static const std::chrono::seconds durationSimu_;  ///< the constant duration of the simulation in the job
  static const std::string solverName_;             ///< The solver name used during the simulation
  static const std::string solverFilename;          ///< The solver filename
  static const std::string solverParId_;            ///< The parameter id in the .par file corresponding to the solver parameters

 private:
  /**
   * @brief Write the solver element of the job file in formatter
   *
   * @param formatter the formatter to update
   */
  void writeSolver(xml::sax::formatter::Formatter& formatter);

  /**
   * @brief Write the modeler element of the job file in formatter
   *
   * @param formatter the formatter to update
   */
  void writeModeler(xml::sax::formatter::Formatter& formatter);

  /**
   * @brief Write the simulation element of the job file in formatter
   *
   * @param formatter the formatter to update
   */
  void writeSimulation(xml::sax::formatter::Formatter& formatter);

  /**
   * @brief Write the outputs element of the job file in formatter
   *
   * @param formatter the formatter to update
   */
  void writeOutputs(xml::sax::formatter::Formatter& formatter);

 private:
  JobDefinition def_;  ///< the job definition to use
};

}  // namespace outputs
}  // namespace dfl
