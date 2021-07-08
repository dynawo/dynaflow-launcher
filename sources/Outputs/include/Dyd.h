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
 * @file  Dyd.h
 *
 * @brief Dynaflow launcher DYD file writer header file
 *
 */

#pragma once

#include "Algo.h"
#include "DynamicDataBaseManager.h"

#include <DYDBlackBoxModel.h>
#include <DYDDynamicModelsCollection.h>
#include <DYDMacroConnect.h>
#include <DYDMacroConnection.h>
#include <DYDMacroConnector.h>
#include <DYDMacroStaticReference.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <unordered_map>
#include <vector>

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
 * @brief DYD file writer
 */
class Dyd {
 public:
  /**
   * @brief Dyd definition to provide informations to build the Dyd file
   */
  struct DydDefinition {
    /**
     * @brief Constructor
     *
     * @param base the basename for current file (corresponds to filepath basename)
     * @param filepath the filepath of the dyd file to write
     * @param gens generators definition coming from algorithms
     * @param loaddefs load definitions coming from algorithms
     * @param slacknode the slack node to use
     * @param hvdcLines hvdc definition coming from algorithms
     * @param busesWithDynamicModel map of bus ids to a generator that regulates them
     * @param dynamicDataBaseManager the database manager to use
     * @param models the list of models to use
     */
    DydDefinition(const std::string& base, const std::string& filepath, const std::vector<algo::GeneratorDefinition>& gens,
                  const std::vector<algo::LoadDefinition>& loaddefs, const std::shared_ptr<inputs::Node>& slacknode,
                  const algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap& hvdcLines,
                  const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel, const inputs::DynamicDataBaseManager& dynamicDataBaseManager,
                  const algo::DynModelDefinitions& models) :
        basename(base),
        filename(filepath),
        generators(gens),
        loads(loaddefs),
        slackNode(slacknode),
        hvdcLines(hvdcLines),
        busesWithDynamicModel(busesWithDynamicModel),
        dynamicDataBaseManager(dynamicDataBaseManager),
        models(models) {}

    std::string basename;                                                        ///< basename for file
    std::string filename;                                                        ///< filepath for file to write
    std::vector<algo::GeneratorDefinition> generators;                           ///< generators found
    std::vector<algo::LoadDefinition> loads;                                     ///< list of loads
    std::shared_ptr<inputs::Node> slackNode;                                     ///< slack node to use
    algo::ControllerInterfaceDefinitionAlgorithm::HvdcLineMap hvdcLines;         ///< list of hvdc lines
    const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel;  ///< map of bus ids to a generator that regulates them
    const inputs::DynamicDataBaseManager& dynamicDataBaseManager;                ///< dynamic database manager
    const algo::DynModelDefinitions& models;                                     ///< the list of models to export
  };

  /**
   * @brief Constructor
   *
   * @param def the dyd definition
   */
  explicit Dyd(DydDefinition&& def);

  /**
   * @brief Write the dyd file
   */
  void write();

 private:
  /**
   * @brief Create black box model for load
   *
   * @param loaddef load definition to use
   * @param basename basename for file
   *
   * @returns black box model for load
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> writeLoad(const algo::LoadDefinition& loaddef, const std::string& basename);

  /**
   * @brief Create black box model for generator
   *
   * @param def generator definition to use
   * @param basename basename for file
   *
   * @returns black box model for generator
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename);

  /**
   * @brief Create black box model for remote voltage regulators
   *
   * @param busId bus id to use
   * @param basename basename for file
   *
   * @returns black box model for a remote voltage regulator
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> writeVRRemote(const std::string& busId, const std::string& basename);

  /**
   * @brief Create black box model for hvdc line
   *
   * @param hvdcLine generator definition to use
   * @param basename basename for file
   *
   * @returns black box model for hvdc line
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> writeHvdcLine(const algo::HvdcLineDefinition& hvdcLine, const std::string& basename);

  /**
   * @brief Create constant models
   *
   * Create Model signal N model
   *
   * @returns black box models
   */
  static std::vector<boost::shared_ptr<dynamicdata::BlackBoxModel>> writeConstantsModel();

  /**
   * @brief Write macro connectors
   *
   * Create maro connectors elements for generators and loads
   *
   * @returns list of macro connectors
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroConnector>> writeMacroConnectors();

  /**
   * @brief Write macro static references
   *
   * Create macro static reference elements for generators and loads
   *
   * @returns list of macro connectors
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroStaticReference>> writeMacroStaticRef();

  /**
   * @brief Write connections for loads
   *
   * Use macro connection
   *
   * @param loaddef the load definition to process
   *
   * @returns the macro connection element
   */
  static boost::shared_ptr<dynamicdata::MacroConnect> writeLoadConnect(const algo::LoadDefinition& loaddef);

  /**
   * @brief Write macro connections for generators
   *
   * Use macro connection
   *
   * @param def the generator definition to process
   * @param index the index of the generator in the global list of generators
   *
   * @returns the macro connection element
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroConnect>> writeGenMacroConnect(const algo::GeneratorDefinition& def, unsigned int index);

  /**
   * @brief Write connection for generators
   *
   * @param dynamicModelsToConnect the collection where the connections will be added
   * @param def the generator definition to process
   *
   */
  static void writeGenConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::GeneratorDefinition& def);

  /**
   * @brief Write connections for remote voltage regulators
   *
   * @param dynamicModelsToConnect the collection where the connections will be added
   * @param busId the bus id to use
   */
  static void writeVRRemoteConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& busId);

  /**
   * @brief Write connections for hvdc lines
   *   *
   * @param dynamicModelsToConnect the collection where the connections will be added
   * @param hvdcLine the hvdc line definition to process
   */
  static void writeHvdcLineConnect(const boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
                                   const algo::HvdcLineDefinition& hvdcLine);

  /**
   * @brief Write list of macro connectors for models
   *
   * @param usedMacros macro connectors used in current simulation
   * @param macros total list of macro connections defined for models
   * @returns list of macro connectors to write
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroConnector>>
  writeDynModelMacroConnectors(const std::unordered_set<std::string>& usedMacros,
                               const std::unordered_map<std::string, inputs::AssemblingXmlDocument::MacroConnection>& macros);

  /**
   * @brief Write model for dynamic model
   * @param dynModel dynamic model to export
   * @param basename basename for file
   * @returns model corresponding to dynamic model
   */
  static boost::shared_ptr<dynamicdata::BlackBoxModel> writeDynModel(const algo::DynModelDefinition& dynModel, const std::string& basename);

  /**
   * @brief Write macro connec for dynamic model
   * @param dynModel dynamic model to use
   * @returns list of macro connect to write
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroConnect>> writeDynModelMacroConnect(const algo::DynModelDefinition& dynModel);

 private:
  static const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string>
      correspondence_lib_;  ///< Correspondence between generator model type and library name in dyd file
  static const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string>
      correspondence_macro_connector_;                           ///< Correspondence between generator model type and macro connector name in dyd file
  static const std::string macroConnectorLoadName_;              ///< name of the macro connector for loads
  static const std::string macroConnectorGenName_;               ///< name for the macro connector for generators
  static const std::string macroConnectorGenSignalNName_;        ///< Name for the macro connector for SignalN
  static const std::string macroStaticRefSignalNGeneratorName_;  ///< Name for the static ref macro for generators using signalN model
  static const std::string macroStaticRefLoadName_;              ///< Name for the static ref macro for loads
  static const std::string networkModelName_;                    ///< name of the model corresponding to network
  static const std::string signalNModelName_;                    ///< Name of the SignaN model

 private:
  DydDefinition def_;  ///< Dyd file information
};
}  // namespace outputs
}  // namespace dfl
