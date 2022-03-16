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
 * @file  DydSVarC.h
 *
 * @brief Dynaflow launcher DYD file writer for static var compensators header file
 *
 */

#pragma once

#include "SVarCDefinitionAlgorithm.h"

#include <DYDDynamicModelsCollection.h>
#include <boost/shared_ptr.hpp>
#include <unordered_map>

namespace std {
/**
 * @brief specialization hash for StaticVarCompensator model types
 *
 * For old compilers, hashs are not implemented for unordered_map when key is an enum class.
 * see https://en.cppreference.com/w/cpp/utility/hash for template definition
 */
template<>
struct hash<dfl::algo::StaticVarCompensatorDefinition::ModelType> {
  /// @brief Constructor
  hash() {}
  /**
   * @brief Action operator
   *
   * Performs hash by relying on integer cast for enum class
   *
   * @param key the key to hash
   * @returns the hash value
   */
  size_t operator()(const dfl::algo::StaticVarCompensatorDefinition::ModelType& key) const {
    return hash<unsigned int>{}(static_cast<unsigned int>(key));
  }
};
}  // namespace std

namespace dfl {
namespace outputs {

class DydSVarC {
 public:
  /**
   * @brief Construct a new Par SVarC object
   *
   * @param svarcsDefinitions reference to the list of SVarCs definitions
   */
  explicit DydSVarC(const std::vector<algo::StaticVarCompensatorDefinition>& svarcsDefinitions) : svarcsDefinitions_(svarcsDefinitions) {}

  /**
   * @brief enrich the dynamic black models set collection for SVarCs
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   */
  void write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename);

 private:
  /**
   * @brief add the macro connector for SVarCs
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief add the macro static reference for SVarCs
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

 private:
  std::vector<algo::StaticVarCompensatorDefinition> svarcsDefinitions_;  ///< list of SVarCs definitions
  static const std::unordered_map<algo::StaticVarCompensatorDefinition::ModelType, std::string>
      svarcModelsNames_;  ///< Correspondance between svarcs model type and library name in dyd file
  const std::string macroConnectorSVarCName_{"StaticVarCompensatorMacroConnector"};
  const std::string macroStaticRefSVarCName_{"StaticVarCompensatorStaticRef"};
};

}  // namespace outputs
}  // namespace dfl
