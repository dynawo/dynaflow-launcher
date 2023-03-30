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
 * @file  DynModelDefinitionAlgorithm.h
 *
 * @brief Dynaflow launcher algorithms for dynamic model management from database: header file
 *
 */

#pragma once

#include "AlgorithmsResults.h"
#include "DynamicDataBaseManager.h"
#include "NetworkManager.h"
#include "Node.h"

#include <array>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dfl {

using NodePtr = std::shared_ptr<inputs::Node>;  ///< Alias for pointer to node
namespace algo {

/**
 * @brief DynModel definition
 *
 * Structure containing minimum information to define a dynamic model with its connections
 */
struct DynamicModelDefinition {
  using DynModelId = std::string;  ///< alias for dynamic model id

  /**
   * @brief Macro connection definition
   */
  struct MacroConnection {
    using MacroId = std::string;    ///< alias for macro connector id
    using ElementId = std::string;  ///< alias for connected element id

    /// @brief Connected element type
    enum class ElementType {
      NODE = 0,  ///< Node type
      LINE,      ///< Line type
      TFO,       ///< Transformer type
      SHUNT,     ///< Shunt type
      GENERATOR  ///< Generator type
    };

    /**
     * @brief Constructor
     *
     * @param macroid the macro connector id to use
     * @param type type of the element to connect
     * @param id the element id to connect
     */
    MacroConnection(const MacroId& macroid, const ElementType& type, const ElementId& id) : id(macroid), elementType(type), connectedElementId(id) {}

    /**
     * @brief Equality operator
     * @param other the macro connection to compare to
     * @returns true if they are equal, false if not
     */
    bool operator==(const MacroConnection& other) const;

    /**
     * @brief Non-equality operator
     * @param other the macro connection to compare to
     * @returns true if they are not equal, false if not
     */
    bool operator!=(const MacroConnection& other) const;

    /**
     * @brief Less than operator
     * @param other the macro connection to compare to
     * @returns true if inferior than other, false if not
     */
    bool operator<(const MacroConnection& other) const;

    /**
     * @brief Less or equal than operator
     * @param other the macro connection to compare to
     * @returns true if inferior or equal than other, false if not
     */
    bool operator<=(const MacroConnection& other) const;

    /**
     * @brief Greater than operator
     * @param other the macro connection to compare to
     * @returns true if superior than other, false if not
     */
    bool operator>(const MacroConnection& other) const;

    /**
     * @brief Greater or equal than operator
     * @param other the macro connection to compare to
     * @returns true if superior or equal than other, false if not
     */
    bool operator>=(const MacroConnection& other) const;

    MacroId id;                    ///< Macro connector id
    ElementType elementType;       ///< Connected element type
    ElementId connectedElementId;  ///< Element id connected throught the macro connection
  };

  /**
   * @brief Constructor
   *
   * @param dynModelId the dynamic model id
   * @param dynModelLib the library name
   */
  DynamicModelDefinition(const DynModelId& dynModelId, const std::string& dynModelLib) : id(dynModelId), lib(dynModelLib) {}

  DynModelId id;                              ///< dynamic model id
  std::string lib;                            ///< library name
  std::set<MacroConnection> nodeConnections;  ///< set of macro connections for the dynamic model
};

/**
 * @brief Definition of models
 */
struct DynamicModelDefinitions {
  std::map<DynamicModelDefinition::DynModelId, DynamicModelDefinition> models;                ///< models by dynamic model id
  std::unordered_set<DynamicModelDefinition::MacroConnection::MacroId> usedMacroConnections;  ///< list of macro connectors used for current set of models
};

/**
 * @brief Algorithm to find dynamic models
 */
class DynModelAlgorithm {
 public:
  /**
   * @brief Constructor
   *
   * The constructor will pre-process some data so that relevant information are retrieved with efficiency during main algorithm
   *
   * @param models the models to update
   * @param manager the dynamic data base manager to use
   * @param shuntRegulationOn whether the shunt regulation is activated or not
   */
  DynModelAlgorithm(DynamicModelDefinitions& models, const inputs::DynamicDataBaseManager& manager, bool shuntRegulationOn);

  /**
   * @brief Perform the algorithm
   *
   * Depending on whether the node is concerned in a macro connection, dispatch the node to update the macro connections information in the models definition.
   * Macro connections not connected to a network node will be discarded and not put in the dynamic models definitions.
   *
   * @param node the node to process
   * @param algoRes pointer to algorithms results class
   */
  void operator()(const NodePtr& node, std::shared_ptr<AlgorithmsResults>& algoRes);

 private:
  /**
   * @brief DynModel macro connect definition
   */
  struct MacroConnect {
    /// @brief Default Constructor
    MacroConnect() = default;

    /**
     * @brief Constructor
     *
     * @param modelId dynamic model id
     * @param macroConnection macro connector id
     */
    MacroConnect(const DynamicModelDefinition::DynModelId& modelId, const DynamicModelDefinition::MacroConnection::MacroId& macroConnection) :
        dynModelId(modelId),
        macroConnectionId(macroConnection) {}

    /**
     * @brief Equality operator
     * @param other the other macro connect to compare to
     * @returns true if they are equal, false if not
     */
    bool operator==(const MacroConnect& other) const;

    /**
     * @brief Non-equality operator
     * @param other the other macro connect to compare to
     * @returns true if they are not equal, false if not
     */
    bool operator!=(const MacroConnect& other) const;

    DynamicModelDefinition::DynModelId dynModelId;                       ///< dynamic model id
    DynamicModelDefinition::MacroConnection::MacroId macroConnectionId;  ///< macro connection id
  };

  /**
   * @brief Hash structure for macro connect
   */
  struct MacroConnectHash {
    /**
     * @brief Operator to hash a macro connect
     *
     * @param connect target macro connect
     * @returns unique hash
     */
    std::size_t operator()(const MacroConnect& connect) const noexcept;
  };

 private:
  /**
   * @brief Determines if a library is loadable in current environement
   * @param lib library name
   * @returns true if library could be loaded, false if not
   */
  static bool libraryExists(const std::string& lib);

  /**
   * @brief Computes library path for library name
   *
   * Search for the library in dynawo environment and then in DFL environment
   *
   * @param lib the library name
   * @returns the filepath of the library, or nullopt if not found
   */
  static boost::optional<boost::filesystem::path> findLibraryPath(const std::string& lib);

 private:
  /// @brief Extract models from configuration before processing the nodes
  /// @param shuntRegulationOn whether the shunt regulation is activated or not
  void extractDynModels(bool shuntRegulationOn);

  /**
   * @brief Process single association from configuration
   *
   * @param automaton the dynamic automaton
   * @param macro the macro connection connected to the singleassociation
   */
  void extractSingleAssociationInfo(const inputs::AssemblingDataBase::DynamicAutomaton& automaton, const inputs::AssemblingDataBase::MacroConnect& macro);

  /**
   * @brief Process multi association from configuration
   *
   * @param automaton the dynamic automaton
   * @param macro the macro connection connected to the multiassociation
   * @param shuntRegulationOn whether the shunt regulation is activated or not
   */
  void extractMultiAssociationInfo(const inputs::AssemblingDataBase::DynamicAutomaton& automaton, const inputs::AssemblingDataBase::MacroConnect& macro,
                                   bool shuntRegulationOn);

  /**
   * @brief Process node in case of dynamic automaton bus connection
   * @param node node to process
   */
  void connectMacroConnectionForBus(const NodePtr& node);

  /**
   * @brief Process node in case of dynamic automaton shunt connection
   * @param node node to process
   */
  void connectMacroConnectionForShunt(const NodePtr& node);

  /**
   * @brief Process node in case of dynamic automaton line connection
   * @param line line to process
   */
  void connectMacroConnectionForLine(const std::shared_ptr<inputs::Line>& line);

  /**
   * @brief Process node in case of dynamic automaton transformer connection
   * @param tfo transformer to process
   */
  void connectMacroConnectionForTfo(const std::shared_ptr<inputs::Tfo>& tfo);

  /**
   * @brief Process node in case of dynamic automaton generator connection
   * @param generator to process
   */
  void connectMacroConnectionForGenerator(const inputs::Generator& generator);

  /**
   * @brief Add macro connection to the dynamic model definition
   *
   * Creates the dynamic model definition if not already existing
   * Update the dynamic model definitions member
   *
   * @param automaton the dynamic automaton
   * @param macroConnection the macro connection to add
   */
  void addMacroConnectionToModelDefinitions(const dfl::inputs::AssemblingDataBase::DynamicAutomaton& automaton,
                                            const DynamicModelDefinition::MacroConnection& macroConnection);

 private:
  DynamicModelDefinitions& dynamicModels_;  ///< Dynamic model definitions to update

  std::unordered_map<inputs::VoltageLevel::VoltageLevelId, std::unordered_set<MacroConnect, MacroConnectHash>>
      macroConnectByVlForBusesId_;  ///< macro connections for buses, by voltage level
  std::unordered_map<inputs::VoltageLevel::VoltageLevelId, std::vector<MacroConnect>>
      macroConnectByVlForShuntsId_;                                                             ///< macro connections for shunts, by voltage level
  std::unordered_map<inputs::Line::LineId, std::vector<MacroConnect>> macroConnectByLineName_;  ///< macro connections for lines, by line id
  std::unordered_map<inputs::Tfo::TfoId, std::vector<MacroConnect>> macroConnectByTfoName_;     ///< macro connections for transformer, by transformer id
  std::unordered_map<inputs::Generator::GeneratorId, std::vector<MacroConnect>>
      macroConnectByGeneratorName_;  ///< macro connections for generators, by generator id

  const inputs::DynamicDataBaseManager& manager_;  ///< dynamic database config manager
};
}  // namespace algo
}  // namespace dfl
