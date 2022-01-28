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

#include "NetworkManager.h"
#include "Node.h"

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node

namespace algo {

/**
 * @brief Generation definition for algorithm
 */
struct GeneratorDefinition {
  /**
   * @brief Generator model type
   */
  enum class ModelType {
    SIGNALN = 0,                ///< Use GeneratorPVSignalN model
    DIAGRAM_PQ_SIGNALN,         ///< Use GeneratorPVDiagramPQSignalN model
    REMOTE_SIGNALN,             ///< Use GeneratorPVRemoteSignalN
    REMOTE_DIAGRAM_PQ_SIGNALN,  ///< Use GeneratorPVRemoteDiagramPQSignalN
    PROP_SIGNALN,               ///< Use GeneratorPQPropSignalN
    PROP_DIAGRAM_PQ_SIGNALN     ///< Use GeneratorPQPropDiagramPQSignalN
  };
  using ReactiveCurvePoint = DYN::GeneratorInterface::ReactiveCurvePoint;  ///< Alias for reactive curve point
  using BusId = std::string;                                               ///< alias of BusId

  /**
   * @brief test if the model used is a diagram
   *
   * @return boolean indicating if the model uses a diagram
   */
  bool isUsingDiagram() const {
    return model == ModelType::DIAGRAM_PQ_SIGNALN || model == ModelType::REMOTE_DIAGRAM_PQ_SIGNALN || model == ModelType::PROP_DIAGRAM_PQ_SIGNALN;
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
   */
  GeneratorDefinition(const inputs::Generator::GeneratorId& genId, ModelType type, const inputs::Node::NodeId& nodeId,
                      const std::vector<ReactiveCurvePoint>& curvePoints, double qmin, double qmax, double pmin, double pmax, double targetP,
                      const BusId& regulatedBusId) :
      id{genId},
      model{type},
      nodeId{nodeId},
      points(curvePoints),
      qmin{qmin},
      qmax{qmax},
      pmin{pmin},
      pmax{pmax},
      targetP{targetP},
      regulatedBusId{regulatedBusId} {}

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

  /**
   * @brief Constructor
   *
   * @param gens generators list to update
   * @param busesWithDynamicModel map of bus ids to a generator that regulates them
   * @param busMap mapping of busId and the number of generators that regulates them
   * @param infinitereactivelimits parameter to determine if infinite reactive limits are used
   * @param serviceManager dynawo service manager in order to use Dynawo extra algorithms
   */
  GeneratorDefinitionAlgorithm(Generators& gens, BusGenMap& busesWithDynamicModel, const inputs::NetworkManager::BusMapRegulating& busMap,
                               bool infinitereactivelimits, const boost::shared_ptr<DYN::ServiceManagerInterface>& serviceManager);

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
   */
  void operator()(const NodePtr& node);

 private:
  /**
   * @brief Checks for diagram validity according to the list of points associated with the generator
   *
   * @param generator The generator with it list of points
   * @return Boolean indicating if the diagram is valid
   */
  static bool isDiagramValid(const inputs::Generator& generator);

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

  Generators& generators_;                                          ///< the generators list to update
  BusGenMap& busesWithDynamicModel_;                                ///< map of bus ids to a generator that regulates them
  const inputs::NetworkManager::BusMapRegulating& busMap_;          ///< mapping of busId and the number of generators that regulates them
  bool useInfiniteReactivelimits_;                                  ///< determine if infinite reactive limits are used
  boost::shared_ptr<DYN::ServiceManagerInterface> serviceManager_;  ///< dynawo service manager
};
}  // namespace algo
}  // namespace dfl
