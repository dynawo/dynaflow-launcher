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
 * @file  SVarCDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for StaticVarCompensators header file
 *
 */

#pragma once

#include "AlgorithmsResults.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief StaticVarCompensator definition for algorithm
 */
class StaticVarCompensatorDefinition {
 public:
  /**
   * @brief StaticVarCompensator model type
   */
  enum class ModelType {
    SVARCPV = 0,
    SVARCPVMODEHANDLING,
    SVARCPVREMOTE,
    SVARCPVREMOTEMODEHANDLING,
    SVARCPVPROP,
    SVARCPVPROPMODEHANDLING,
    SVARCPVPROPREMOTE,
    SVARCPVPROPREMOTEMODEHANDLING,
    NETWORK
  };
  /**
   * @brief Constructor
   *
   * @param sVarCId the id of the SVarC
   * @param modelType SVarC model type
   * @param bMin the minimum susceptance value for the variable susceptance of the SVarC
   * @param bMax the maximum susceptance value for the variable susceptance of the SVarC
   * @param voltageSetPoint the voltage set point of the SVarC
   * @param UNom the nominal voltage of the connection bus of the SVarC
   * @param UMinActivation the low voltage activation threshold of the SVarC
   * @param UMaxActivation the high voltage activation threshold of the SVarC
   * @param USetPointMin the low voltage set point of the SVarC
   * @param USetPointMax the high voltage set point of the SVarC
   * @param b0 the initial susceptance value  of the SVarC
   * @param slope the slope (kV/MVar) of the SVarC voltage regulation
   * @param UNomRemote the nominal voltage of the remotely regulated bus
   * @param regulatedBusId id of the regulated bus
   */
  StaticVarCompensatorDefinition(const inputs::StaticVarCompensator::SVarCid &sVarCId, ModelType modelType, const double bMin, const double bMax,
                                 const double voltageSetPoint, const double UNom, const double UMinActivation, const double UMaxActivation,
                                 const double USetPointMin, const double USetPointMax, const double b0, const double slope, const double UNomRemote,
                                 const std::string &regulatedBusId = "")
      : id{sVarCId}, model{modelType}, bMin{bMin}, bMax{bMax}, voltageSetPoint{voltageSetPoint}, UNom{UNom}, UMinActivation{UMinActivation},
        UMaxActivation{UMaxActivation}, USetPointMin{USetPointMin}, USetPointMax{USetPointMax}, b0{b0}, slope{slope}, UNomRemote{UNomRemote},
        regulatedBusId{regulatedBusId} {}

  /**
   * @brief determines if the SVarC model type is network
   *
   * @return true when model type is network
   */
  bool isNetwork() const { return model == ModelType::NETWORK; }

  /**
   * @brief determines if the SVarC has a remote regulation
   *
   * @return true when model type has a remote regulation
   */
  bool isRemoteRegulation() const {
    return model == ModelType::SVARCPVREMOTE || model == ModelType::SVARCPVREMOTEMODEHANDLING || model == ModelType::SVARCPVPROPREMOTE ||
           model == ModelType::SVARCPVPROPREMOTEMODEHANDLING;
  }

  inputs::StaticVarCompensator::SVarCid id;  ///< the id of the SVarC
  ModelType model;                           ///< SVarC model type
  const double bMin;                         ///< the minimum susceptance value for the variable susceptance of the SVarC
  const double bMax;                         ///< the maximum susceptance value for the variable susceptance of the SVarC
  const double voltageSetPoint;              ///< the voltage set point of the SVarC
  const double UNom;                         ///< the nominal voltage of the connection bus of the SVarC
  const double UMinActivation;               ///< the low voltage activation threshold of the SVarC
  const double UMaxActivation;               ///< the high voltage activation threshold of the SVarC
  const double USetPointMin;                 ///< the low voltage set point of the SVarC
  const double USetPointMax;                 ///< the high voltage set point of the SVarC
  const double b0;                           ///< the initial susceptance value of the SVarC
  const double slope;                        ///< the slope (kV/MVar) of the SVarC voltage regulation
  const double UNomRemote;                   ///< the nominal voltage of the remotely regulated bus
  const std::string regulatedBusId;          ///< The bus id regulated by this sVarC
};

/// @brief Static var compensator algorithm
class StaticVarCompensatorAlgorithm {
 public:
  using SVarCDefinitions = std::vector<StaticVarCompensatorDefinition>;  ///< Alias for vector of SVarC definitions
  using ModelType = StaticVarCompensatorDefinition::ModelType;           ///< Alias for SVarC model type

  /**
   * @brief Constructor
   * @param svarcs the list of SVarCs to update
   */
  explicit StaticVarCompensatorAlgorithm(SVarCDefinitions &svarcs);

  /**
   * @brief Perform algorithm
   * Add the SVarCs of the nodes and deducing the model to use.
   * @param node the node to process
   * @param algoRes pointer to algorithms results class
   */
  void operator()(const NodePtr &node, std::shared_ptr<AlgorithmsResults> &algoRes);

 private:
  SVarCDefinitions &svarcs_;  ///< the list of SVarCs to update
};
}  // namespace algo
}  // namespace dfl
