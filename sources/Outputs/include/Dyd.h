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

#include <DYDBlackBoxModel.h>
#include <DYDMacroConnect.h>
#include <DYDMacroConnection.h>
#include <DYDMacroConnector.h>
#include <DYDMacroStaticReference.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace dfl {
namespace outputs {

/**
 * @brief Dyd writer
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
     * @param loaddefs laod definitions coming from algorithms
     * @param slacknode the slack node to use
     */
    DydDefinition(const std::string& base, const std::string& filepath, const std::vector<algo::GeneratorDefinition>& gens,
                  const std::vector<algo::LoadDefinition>& loaddefs, const std::shared_ptr<inputs::Node>& slacknode) :
        basename{base},
        filename{filepath},
        generators{gens},
        loads{loaddefs},
        slackNode{slacknode} {}

    std::string basename;                               ///< basename for file
    std::string filename;                               ///< filepath for file to write
    std::vector<algo::GeneratorDefinition> generators;  ///< generators found
    std::vector<algo::LoadDefinition> loads;            ///< list of loads
    std::shared_ptr<inputs::Node> slackNode;            ///< list of generators
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
   * @brief Create constant models
   *
   * Create Model signal N model
   *
   * @param basename basename for file
   *
   * @returns black box models
   */
  static std::vector<boost::shared_ptr<dynamicdata::BlackBoxModel>> writeConstantsModel(const std::string& basename);

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
   * Create maro static reference elements for generators and loads
   *
   * @returns list of macro connectors
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroStaticReference>> writeMacroStaticRef();

  /**
   * @brief Write connections for loads
   *
   * Use macro connection
   *
   * @param loaded the load definition to process
   *
   * @returns the macro connection element
   */
  static boost::shared_ptr<dynamicdata::MacroConnect> writeLoadConnect(const algo::LoadDefinition& loaddef);

  /**
   * @brief Write connections for generators
   *
   * Use macro connection
   *
   * @param def the generator definition to process
   *
   * @returns the macro connection element
   */
  static std::vector<boost::shared_ptr<dynamicdata::MacroConnect>> writeGenConnect(const algo::GeneratorDefinition& def, unsigned int index);

 private:
  static const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string>
      correspondence_lib_;  ///< Correspondence between generator model type and libary name in dyd file
  static const std::unordered_map<algo::GeneratorDefinition::ModelType, std::string>
      correspondence_macro_connector_;                           ///< Correspondence between generator model type and macro connector name in dyd file
  static const std::string macroConnectorLoadName_;              ///< name of the macro connector for loads
  static const std::string macroConnectorGenName_;               ///< name for the macro connector for generators
  static const std::string macroConnectorGenImpedenceName_;      ///< name for the macro connector for generators with impedence
  static const std::string macroConnectorGenSignalNName_;        ///< Name for the macro connector for SignalN
  static const std::string macroStaticRefSignalNGeneratorName_;  ///< Name for the static ref macro for generators using signalN model
  static const std::string macroStaticRefImpGeneratorName_;      ///< Name for the static ref macro for generators using impedance model
  static const std::string macroStaticRefLoadName_;              ///< Name for the static ref macro for loads
  static const std::string networkModelName_;                    ///< name of the model corresponding to network
  static const std::string signalNModelName_;                    ///< Name of the SignaN model

 private:
  DydDefinition def_;  ///< Dyd file information
};
}  // namespace outputs
}  // namespace dfl
