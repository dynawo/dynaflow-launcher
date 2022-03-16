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
 * @file  DydHvdc.h
 *
 * @brief Dynaflow launcher DYD file writer for hvdcs header file
 *
 */

#pragma once

#include "HVDCDefinitionAlgorithm.h"

#include <DYDDynamicModelsCollection.h>
#include <boost/shared_ptr.hpp>
#include <unordered_map>

namespace std {
/**
 * @brief specialization hash for HVDC models types
 *
 * For old compilers, hashs are not implemented for unordered_map when key is an enum class.
 * see https://en.cppreference.com/w/cpp/utility/hash for template definition
 */
template<>
struct hash<dfl::algo::HVDCDefinition::HVDCModel> {
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
  size_t operator()(const dfl::algo::HVDCDefinition::HVDCModel& key) const {
    return hash<unsigned int>{}(static_cast<unsigned int>(key));
  }
};
}  // namespace std

namespace dfl {
namespace outputs {

class DydHvdc {
 public:
  /**
   * @brief Construct a new Par Hvdc object
   *
   * @param hvdcDefinitions reference to the list of load definitions
   */
  explicit DydHvdc(const algo::HVDCLineDefinitions& hvdcDefinitions) : hvdcDefinitions_(hvdcDefinitions) {}

  /**
   * @brief enrich the dynamic black models set collection for Hvdc
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   */
  void write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename);

 private:
  /**
   * @brief write specific hvdc connections
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param hvdcLine the hvdc line definition to process
   */
  void writeConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::HVDCDefinition& hvdcLine);

 private:
  const algo::HVDCLineDefinitions& hvdcDefinitions_;  ///< list of Hvdc definitions
  static const std::unordered_map<algo::HVDCDefinition::HVDCModel, std::string>
      hvdcModelsNames_;  ///< Correspondence between HVDC model and their library name in dyd file
};

}  // namespace outputs
}  // namespace dfl
