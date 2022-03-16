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

#include "DydDynModel.h"
#include "DydGenerator.h"
#include "DydHvdc.h"
#include "DydLoads.h"
#include "DydSVarC.h"
#include "DynModelDefinitionAlgorithm.h"
#include "DynamicDataBaseManager.h"
#include "GeneratorDefinitionAlgorithm.h"
#include "HVDCDefinitionAlgorithm.h"
#include "LineDefinitionAlgorithm.h"
#include "LoadDefinitionAlgorithm.h"
#include "MainConnexComponentAlgorithm.h"
#include "SVarCDefinitionAlgorithm.h"
#include "ShuntDefinitionAlgorithm.h"
#include "SlackNodeAlgorithm.h"

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
     * @param hvdcDefinitions hvdc definitions coming from algorithms
     * @param busesWithDynamicModel map of bus ids to a generator that regulates them
     * @param dynamicDataBaseManager the database manager to use
     * @param models the list of dynamic models to use
     * @param svarcsDefs the SVarC definitions to use
     */
    DydDefinition(const std::string& base, const std::string& filepath, const std::vector<algo::GeneratorDefinition>& gens,
                  const std::vector<algo::LoadDefinition>& loaddefs, const std::shared_ptr<inputs::Node>& slacknode,
                  const algo::HVDCLineDefinitions& hvdcDefinitions, const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel,
                  const inputs::DynamicDataBaseManager& dynamicDataBaseManager, const algo::DynamicModelDefinitions& models,
                  const std::vector<algo::StaticVarCompensatorDefinition> svarcsDefs) :
        basename_(base),
        filename_(filepath),
        slackNode_(slacknode),
        hvdcDefinitions_(hvdcDefinitions),
        busesWithDynamicModel_(busesWithDynamicModel),
        dynamicDataBaseManager_(dynamicDataBaseManager),
        dydLoads_(new DydLoads(loaddefs)),
        dydSVarC_(new DydSVarC(svarcsDefs)),
        dydHvdc_(new DydHvdc(hvdcDefinitions)),
        dydGenerator_(new DydGenerator(gens)),
        dydDynModel_(new DydDynModel(models)) {}

    std::string basename_;                                                        ///< basename for file
    std::string filename_;                                                        ///< filepath for file to write
    std::shared_ptr<inputs::Node> slackNode_;                                     ///< slack node to use
    const algo::HVDCLineDefinitions& hvdcDefinitions_;                            ///< list of hvdc definitions
    const algo::GeneratorDefinitionAlgorithm::BusGenMap& busesWithDynamicModel_;  ///< map of bus ids to a generator that regulates them
    const inputs::DynamicDataBaseManager& dynamicDataBaseManager_;                ///< dynamic database manager
    std::shared_ptr<DydLoads> dydLoads_;                                          ///< reference to load dyd writer
    std::shared_ptr<DydSVarC> dydSVarC_;                                          ///< reference to svarcs dyd writer
    std::shared_ptr<DydHvdc> dydHvdc_;                                            ///< reference to hvdcs dyd writer
    std::shared_ptr<DydGenerator> dydGenerator_;                                  ///< reference to generators dyd writer
    std::shared_ptr<DydDynModel> dydDynModel_;                                    ///< reference to defined dynamic model dyd writer
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
  void write() const;

 private:
  DydDefinition def_;  ///< Dyd file information
};
}  // namespace outputs
}  // namespace dfl
