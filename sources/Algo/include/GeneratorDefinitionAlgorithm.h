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
 * @file  GeneratorDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for generators header file
 *
 */

#pragma once

#include "AlgorithmsResults.h"
#include "DynamicDataBaseManager.h"
#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node

namespace algo {

/**
 * @brief Generation definition for algorithm
 */
class GeneratorDefinition {
 public:
  /**
   * @brief Generator model type
   * RPCL = reactive power control loop for connection to the secondary voltage control
   */
  enum class ModelType {
    SIGNALN_INFINITE = 0,           ///< Use signalN generator model with infinite diagram
    SIGNALN_RECTANGULAR,            ///< Use signalN generator model with rectangular diagram
    DIAGRAM_PQ_SIGNALN,             ///< Use signalN generator model
    SIGNALN_RPCL_INFINITE,          ///< Use signalN generator model with infinite diagram and RPCL
    SIGNALN_RPCL_RECTANGULAR,       ///< Use signalN generator model with rectangular diagram and RPCL
    DIAGRAM_PQ_RPCL_SIGNALN,        ///< Use signalN generator model and RPCL
    SIGNALN_RPCL2_INFINITE,         ///< Use signalN generator model with infinite diagram and RPCL2
    SIGNALN_RPCL2_RECTANGULAR,      ///< Use signalN generator model with rectangular diagram and RPCL2
    DIAGRAM_PQ_RPCL2_SIGNALN,       ///< Use signalN generator model and RPCL2
    SIGNALN_TFO_INFINITE,           ///< Use signalN generator model with infinite diagram and transformer
    SIGNALN_TFO_RECTANGULAR,        ///< Use signalN generator model with rectangular diagram and transformer
    DIAGRAM_PQ_TFO_SIGNALN,         ///< Use signalN generator model and transformer
    SIGNALN_TFO_RPCL_INFINITE,      ///< Use signalN generator model with infinite diagram, transformer and RPCL
    SIGNALN_TFO_RPCL_RECTANGULAR,   ///< Use signalN generator model with rectangular diagram, transformer and RPCL
    DIAGRAM_PQ_TFO_RPCL_SIGNALN,    ///< Use signalN generator model, transformer and RPCL
    SIGNALN_TFO_RPCL2_INFINITE,     ///< Use signalN generator model with infinite diagram, transformer and RPCL2
    SIGNALN_TFO_RPCL2_RECTANGULAR,  ///< Use signalN generator model with rectangular diagram, transformer and RPCL2
    DIAGRAM_PQ_TFO_RPCL2_SIGNALN,   ///< Use signalN generator model with transformer and RPCL2
    REMOTE_SIGNALN_INFINITE,        ///< Use signalN generator regulating remote nodes with infinite diagram
    REMOTE_SIGNALN_RECTANGULAR,     ///< Use signalN generator regulating remote nodes with rectangular diagram
    REMOTE_DIAGRAM_PQ_SIGNALN,      ///< Use signalN generator regulating remote nodes
    PROP_SIGNALN_INFINITE,          ///< Use signalN generator sharing node regulation with infinite diagram
    PROP_SIGNALN_RECTANGULAR,       ///< Use signalN generator sharing node regulation with rectangular diagram
    PROP_DIAGRAM_PQ_SIGNALN,        ///< Use Use signalN generator sharing node regulation
    NETWORK                         ///< Use network cpp model
  };
  using ReactiveCurvePoint = inputs::Generator::ReactiveCurvePoint;  ///< Alias for reactive curve point
  using BusId = std::string;                                         ///< alias of BusId

  /**
   * @brief test if the generator has a finite diagram
   *
   * @return @b true if the generator uses a finite diagram, @b false otheriwise
   */
  bool isUsingDiagram() const {
    return model != ModelType::SIGNALN_INFINITE && model != ModelType::REMOTE_SIGNALN_INFINITE && model != ModelType::PROP_SIGNALN_INFINITE &&
           model != ModelType::SIGNALN_TFO_INFINITE && model != ModelType::SIGNALN_RPCL_INFINITE && model != ModelType::SIGNALN_TFO_RPCL_INFINITE &&
           model != ModelType::SIGNALN_RPCL2_INFINITE && model != ModelType::SIGNALN_TFO_RPCL2_INFINITE && model != ModelType::NETWORK;
  }
  /**
   * @brief test is the generator has a rectangular diagram
   *
   * @return @b true if the diagram is rectangular, @b false otherwise
   */
  bool isUsingRectangularDiagram() const {
    return model == ModelType::SIGNALN_RECTANGULAR || model == ModelType::SIGNALN_TFO_RECTANGULAR || model == ModelType::REMOTE_SIGNALN_RECTANGULAR ||
           model == ModelType::PROP_SIGNALN_RECTANGULAR || model == ModelType::SIGNALN_TFO_RPCL_RECTANGULAR || model == ModelType::SIGNALN_RPCL_RECTANGULAR ||
           model == ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR || model == ModelType::SIGNALN_RPCL2_RECTANGULAR;
  }
  /**
   * @brief test is the generator has a tranformer
   *
   * @return @b true if the generator has a tranformer, @b false otherwise
   */
  bool hasTransformer() const {
    return model == ModelType::SIGNALN_TFO_INFINITE || model == ModelType::SIGNALN_TFO_RECTANGULAR || model == ModelType::DIAGRAM_PQ_TFO_SIGNALN ||
           model == ModelType::SIGNALN_TFO_RPCL_INFINITE || model == ModelType::SIGNALN_TFO_RPCL_RECTANGULAR ||
           model == ModelType::DIAGRAM_PQ_TFO_RPCL_SIGNALN || model == ModelType::SIGNALN_TFO_RPCL2_INFINITE ||
           model == ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR || model == ModelType::DIAGRAM_PQ_TFO_RPCL2_SIGNALN;
  }
  /**
   * @brief test is the generator has a reactive power control loop for connection to the secondary voltage control
   *
   * @return @b true if the generator has a reactive power control loop for connection to the secondary voltage control, @b false otherwise
   */
  bool hasRpcl() const {
    return model == ModelType::SIGNALN_RPCL_INFINITE || model == ModelType::SIGNALN_RPCL_RECTANGULAR || model == ModelType::DIAGRAM_PQ_RPCL_SIGNALN ||
           model == ModelType::SIGNALN_TFO_RPCL_INFINITE || model == ModelType::SIGNALN_TFO_RPCL_RECTANGULAR ||
           model == ModelType::DIAGRAM_PQ_TFO_RPCL_SIGNALN || model == ModelType::SIGNALN_RPCL2_INFINITE || model == ModelType::SIGNALN_RPCL2_RECTANGULAR ||
           model == ModelType::DIAGRAM_PQ_RPCL2_SIGNALN || model == ModelType::SIGNALN_TFO_RPCL2_INFINITE ||
           model == ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR || model == ModelType::DIAGRAM_PQ_TFO_RPCL2_SIGNALN;
  }
  /**
   * @brief test is the generator has a reactive power control loop 2 for connection to the secondary voltage control
   *
   * @return @b true if the generator has a reactive power control loop 2 for connection to the secondary voltage control, @b false otherwise
   */
  bool hasRpcl2() const {
    return model == ModelType::SIGNALN_RPCL2_INFINITE || model == ModelType::SIGNALN_RPCL2_RECTANGULAR || model == ModelType::DIAGRAM_PQ_RPCL2_SIGNALN ||
           model == ModelType::SIGNALN_TFO_RPCL2_INFINITE || model == ModelType::SIGNALN_TFO_RPCL2_RECTANGULAR ||
           model == ModelType::DIAGRAM_PQ_TFO_RPCL2_SIGNALN;
  }
  /**
   * @brief determines if the generator model type is network
   *
   * @return true when model type is network
   */
  bool isNetwork() const {
    return model == ModelType::NETWORK;
  }

  /**
   * @brief determines if the generator is regulating remotely
   *
   * @return true when generator is regulating remotely
   */
  bool isRegulatingRemotely() const {
    return model == ModelType::REMOTE_DIAGRAM_PQ_SIGNALN || model == ModelType::REMOTE_SIGNALN_INFINITE || model == ModelType::REMOTE_SIGNALN_RECTANGULAR;
  }

  /**
   * @brief Constructor
   *
   * @param genId generator id
   * @param type the model to use
   * @param nodeId the node id connected to the generator
   * @param curvePoints the list of reactive capabilities curve points
   * @param qmin minimum reactive power for the generator
   * @param qmax maximum reactive power for the generator
   * @param pmin minimum active power for the generator
   * @param pmax maximum active power for the generator
   * @param targetP target active power of the generator
   * @param regulatedBusId the Bus Id this generator is regulating
   * @param isNuclear true if the energy source of this generator is nuclear
   */
  GeneratorDefinition(const inputs::Generator::GeneratorId& genId, ModelType type, const inputs::Node::NodeId& nodeId,
                      const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax, double targetP,
                      const BusId& regulatedBusId, bool isNuclear = false) :
      id{genId},
      model{type},
      nodeId{nodeId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax},
      targetP{targetP},
      regulatedBusId{regulatedBusId},
      isNuclear{isNuclear} {}

  inputs::Generator::GeneratorId id;       ///< generator id
  ModelType model;                         ///< model
  inputs::Node::NodeId nodeId;             ///< connected node id
  std::vector<ReactiveCurvePoint> points;  ///< curve points
  double qmin;                             ///< minimum reactive power
  double qmax;                             ///< maximum reactive power
  double pmin;                             ///< minimum active power
  double pmax;                             ///< maximum active power
  double targetP;                          ///< target active power of the generator
  const BusId regulatedBusId;              ///< regulated Bus Id
  const bool isNuclear;                    ///< true if the energy source of this generator is nuclear
};

/**
 * @brief Algorithm to find generators
 */
class GeneratorDefinitionAlgorithm {
 public:
  using Generators = std::vector<GeneratorDefinition>;  ///< alias for list of generators
  using BusId = std::string;                            ///< alias for bus id
  using GenId = std::string;                            ///< alias for generator id
  using BusGenMap = std::unordered_map<BusId, GenId>;   ///< alias for map of bus id to generator id
  using ModelType = GeneratorDefinition::ModelType;     ///< alias for generator model type

  /**
   * @brief Constructor
   *
   * @param gens generators list to update
   * @param busesWithDynamicModel map of bus ids to a generator that regulates them
   * @param busMap mapping of busId and the number of generators that regulates them
   * @param manager the dynamic data base manager to use
   * @param infinitereactivelimits parameter to determine if infinite reactive limits are used
   * @param tfoVoltageLevel maximum voltage level for which we assume that generator's transformers are already described in the static description
   */
  GeneratorDefinitionAlgorithm(Generators& gens, BusGenMap& busesWithDynamicModel, const inputs::NetworkManager::BusMapRegulating& busMap,
                               const inputs::DynamicDataBaseManager& manager, bool infinitereactivelimits, double tfoVoltageLevel);

  /**
   * @brief Perform algorithm
   *
   * Add the generators of the nodes and deducing the model to use.
   * Validity of the generator is checked whenever it makes sense.
   * If the diagram is not valid, the generator is ignored by the algorithm and the default dynawo behavior is used.
   * For each generator, we look in the busMap_ the number of generators that are regulating the same bus as this generator,
   * If this generator is the only one regulating the bus, then we decide which model to use based on whether this generator
   * regulates the bus in local or not. Otherwise we use the prop model.
   * In order to create later on in the dyd and the par specific models based on the buses that are regulated by multiples
   * generators, we fill the busesWithDynamicModel_ map. Each time we found a bus regulated by multiples generators we add in the
   * busesWithDynamicModel_ map an element mapping the regulated bus to a generator id that regulates that bus.
   * @param node the node to process
   * @param algoRes pointer to algorithms results class
   */
  void operator()(const NodePtr& node, std::shared_ptr<AlgorithmsResults>& algoRes);

 private:
  /**
   * @brief Checks for diagram validity according to the list of points associated with the generator
   *
   * @param generator The generator with it list of points
   * @return Boolean indicating if the diagram is valid
   */
  bool isDiagramValid(const inputs::Generator& generator);

  /**
   * @brief Determines if a node is connected to another generator node through a switch network path
   *
   * Uses the dynawo service manager
   *
   * @param node the generator node to check
   *
   * @returns @b true if another generator is connected, @b false if not
   */
  bool IsOtherGeneratorConnectedBySwitches(const NodePtr& node) const;

  /**
   * @brief determine if targetP of generator is initially in diagram
   *
   * @param generator reference to inputs generator
   * @return true if targetP is initially in diagram
   */
  bool isTargetPValid(const inputs::Generator& generator) const;

  /**
   * @brief checks if the generator has a rectangular diagram
   *
   * @param generator reference to input generator
   * @return @b true if diagram is rectangular, @b false otherwise
   */
  bool isDiagramRectangular(const inputs::Generator& generator) const;

  Generators& generators_;                                  ///< the generators list to update
  BusGenMap& busesWithDynamicModel_;                        ///< map of bus ids to a generator that regulates them
  const inputs::NetworkManager::BusMapRegulating& busMap_;  ///< mapping of busId and the number of generators that regulates them
  std::unordered_map<std::string, bool> generatorsInSVC;    ///< If a generator id is in this map then it belongs to a secondary voltage control area,
                                                            // if the associated bool is true then it uses a reactive power control loop 2
  bool useInfiniteReactivelimits_;                          ///< determine if infinite reactive limits are used,
  double tfoVoltageLevel_;  ///< Maximum voltage level for which we assume that generator's transformers are already described in the static description
};
}  // namespace algo
}  // namespace dfl
