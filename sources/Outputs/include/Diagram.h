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
 * @file  Diagram.h
 *
 * @brief Dynaflow launcher Diagram file writer header file
 *
 */

#pragma once

#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"

#include <string>
#include <vector>
namespace dfl {
namespace outputs {

/**
* @brief Diagram writer
*/
class Diagram {
 public:
  /**
  * @brief Diagram definition to provide informations to build the Diagram file
  */
  struct DiagramDefinition {
    /**
     * @brief Constructor
     *
     * @param base the basename for current file (corresponds to filepath basename)
     * @param directoryPath the directory path of the diagram files to write
     * @param gens generators definition coming from algorithms
     * @param hvdcDefinitions the HVDC definitions to used
     */
    DiagramDefinition(const std::string& base, const std::string& directoryPath, const std::vector<algo::GeneratorDefinition>& gens,
                      const algo::HVDCLineDefinitions& hvdcDefinitions) :
        basename(base),
        directoryPath(directoryPath),
        generators(gens),
        hvdcDefinitions(hvdcDefinitions) {}

    const std::string basename;       ///< basename for file
    const std::string directoryPath;  ///< directory path for files to write
    // non const copies instead of const references because we need to modify them before use
    std::vector<algo::GeneratorDefinition> generators;  ///< generators found
    algo::HVDCLineDefinitions hvdcDefinitions;          ///< HVDC definitions
  };

  /**
   * @brief Constructor
   *
   * @param def the Diagram definition
   */
  explicit Diagram(DiagramDefinition&& def);

  /**
   * @brief Write the Diagram file
   */
  void write() const;

 private:
  /// @brief Different tables in the diagram, qmin or qmax
  enum class Tables {
    TABLE_QMIN = 0,  ///< Table Qmin
    TABLE_QMAX       ///< Table Qmax
  };

  /// @brief LCC definition used to write diagrams files
  struct LCCDefinition {
    algo::HVDCDefinition::ConverterId id;                          ///< id
    std::vector<inputs::VSCConverter::ReactiveCurvePoint> points;  ///< Reactive curve points (always empty for LCC)
    double pmax;                                                   ///< maximum p
    double qmax;                                                   ///< maximum q
    double pmin;                                                   ///< minimum p
    double qmin;                                                   ///< minimum q
  };

  /**
   * @brief Write a single table in the Diagram file
   *
   * the type T requires to have:
   * - a field "id" (string)
   * - a vector of reactive curve points field "points"
   * - a double field "pmax"
   * - a double field "qmax"
   * - a double field "pmin"
   * - a double field "qmin"
   *
   * @param element The element that will be used to write the diagram values
   * @param buffer The buffer to store the string that will be written to the file
   * @param table The enum determining if we write the Qmin or Qmax table
   */
  template<class T>
  static void writeTable(const T& element, std::stringstream& buffer, Tables table);

  /// @brief Write generator diagrams
  void writeGenerators() const;
  /// @brief Write VSC converters diagrams
  void writeConverters() const;

  /**
   * @brief Write VSC converter diagram
   * @param vscDefinition the VSC definition to use
   */
  void writeVSC(const dfl::algo::VSCDefinition& vscDefinition) const;

  /**
   * @brief Write LCC converter diagram
   * @param converterId the id of the LCC converter
   * @param powerFactor the power factor of the LCC
   * @param pMax the maximum p of the HVDC line which owns the LCC converter
   */
  void writeLCC(const algo::HVDCDefinition::ConverterId& converterId, double powerFactor, double pMax) const;

 private:
  DiagramDefinition def_;  ///< Diagram file information
};
}  // namespace outputs
}  // namespace dfl
