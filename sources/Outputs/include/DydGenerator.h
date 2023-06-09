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
 * @file  DydGenerator.h
 *
 * @brief Dynaflow launcher DYD file writer for generators header file
 *
 */

#pragma once

#include "GeneratorDefinitionAlgorithm.h"

#include <DYDDynamicModelsCollection.h>
#include <boost/shared_ptr.hpp>
#include <unordered_map>

namespace std {
/**
 * @brief specialization hash for generators model types
 *
 * For old compilers, hashs are not implemented for unordered_map when key is an enum class.
 * see https://en.cppreference.com/w/cpp/utility/hash for template definition
 */
template<>
struct hash<dfl::algo::GeneratorDefinition::ModelType> {
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
  size_t operator()(const dfl::algo::GeneratorDefinition::ModelType& key) const {
    return hash<unsigned int>{}(static_cast<unsigned int>(key));
  }
};
}  // namespace std

namespace dfl {
namespace outputs {

/**
 * @brief generators DYD file writer
 */
class DydGenerator {
 public:
  /**
   * @brief Construct a new Dyd Generator object
   *
   * @param generatorDefinitions reference to the list of generator definitions
   */
  explicit DydGenerator(const std::vector<algo::GeneratorDefinition>& generatorDefinitions) : generatorDefinitions_(generatorDefinitions) {}

  /**
   * @brief enrich the dynamic black models set collection for generators
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param basename the basename for current file
   * @param slackNodeId id of slack node
   */
  void write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
              const std::string& basename,
              const std::string& slackNodeId);

 private:
  /**
   * @brief add the macro connector for generators
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief add the macro static reference for generators
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief add the signal N balck box model
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeSignalNBlackBox(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief write all the connection between a generator and signal N
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   */
  void writeMacroConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect);

  /**
   * @brief add the connection of signal N model to the slack node
   *
   * @param dynamicModelsToConnect dynamic black models set collection to enrich
   * @param slackNodeId slack node id
   */
  void writeThetaRefConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& slackNodeId);

 private:
  static const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string>
      correspondence_lib_;                                                      ///< Correspondance between generator model type and library name in dyd file
  const std::string macroStaticRefSignalNGeneratorName_{"GeneratorStaticRef"};  ///< Name for the static ref macro for generators using signalN model
  const std::string macroConnectorGenName_{"GEN_NETWORK_CONNECTOR"};            ///< name for the macro connector for generators
  const std::string macroConnectorGenSignalNName_{"GEN_SIGNALN_CONNECTOR"};     ///< Name for the macro connector for SignalN
  const std::string signalNModelName_{"Model_Signal_N"};                        ///< Name of the SignalN model
  const std::vector<algo::GeneratorDefinition>& generatorDefinitions_;          ///< list of generators definitions
};

}  // namespace outputs
}  // namespace dfl
