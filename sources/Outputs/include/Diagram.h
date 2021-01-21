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

#include "Algo.h"

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
     */
    DiagramDefinition(const std::string& base, const std::string& directoryPath, const std::vector<algo::GeneratorDefinition>& gens) :
        basename(base),
        directoryPath(directoryPath),
        generators(gens) {}

    std::string basename;                               ///< basename for file
    std::string directoryPath;                          ///< directory path for files to write
    std::vector<algo::GeneratorDefinition> generators;  ///< generators found
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
  void write();

 private:
  /// @brief Different tables in the diagram, qmin or qmax
  enum class Tables {
    TABLE_QMIN = 0,  ///< Table Qmin
    TABLE_QMAX       ///< Table Qmax
  };

  /**
   * @brief Write a single table in the Diagram file
   *
   * @param generator The generator that will be used to write the diagram values
   * @param buffer The buffer to store the string that will be written to the file
   * @param table The enum determining if we write the Qmin or Qmax table
   */
  static void writeTable(const algo::GeneratorDefinition& generator, std::stringstream& buffer, Tables table);

  DiagramDefinition def_;  ///< Diagram file information
};
}  // namespace outputs
}  // namespace dfl
