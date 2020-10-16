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
#include <unordered_map>
#include <vector>
namespace dfl {
namespace outputs {

/**
* @brief Dyd writer
*/
class Diagram {
 public:
  /**
  * @brief Dyd definition to provide informations to build the Dyd file
  */
  struct DiagramDefinition {
    /**
     * @brief Constructor
     *
     * @param base the basename for current file (corresponds to filepath basename)
     * @param filepath the filepath of the dyd file to write
     * @param gens generators definition coming from algorithms
     */
    DiagramDefinition(const std::string& base, const std::string& filepath, const std::vector<algo::GeneratorDefinition>& gens) :
        basename{base}, filename{filepath}, generators{gens} {}

    std::string basename;                               ///< basename for file
    std::string filename;                               ///< filepath for file to write
    std::vector<algo::GeneratorDefinition> generators;  ///< generators found
  };

  /**
   * @brief Constructor
   *
   * @param def the dyd definition
   */
  explicit Diagram(DiagramDefinition&& def);

  /**
   * @brief Write the dyd file
   */
  void write();

 private:
  void write_table(const algo::GeneratorDefinition& generator, std::stringstream& buffer, bool is_table_qmin);
  DiagramDefinition def_;  ///< Dyd file information
};
}  // namespace outputs
}  // namespace dfl
