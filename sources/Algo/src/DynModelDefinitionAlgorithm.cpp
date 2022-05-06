//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DynModelDefinitionAlgorithm.h"

#include "Log.h"

#include <DYNCommon.h>
#include <DYNExecUtils.h>
#include <boost/dll/import.hpp>
#include <boost/functional.hpp>
#include <tuple>

namespace dfl {
namespace algo {

DynModelAlgorithm::DynModelAlgorithm(DynamicModelDefinitions& models, const inputs::DynamicDataBaseManager& manager, bool shuntRegulationOn) :
    dynamicModels_(models),
    manager_(manager) {
  extractDynModels(shuntRegulationOn);
}

boost::optional<boost::filesystem::path>
DynModelAlgorithm::findLibraryPath(const std::string& lib) {
  static auto dflLibEnv = getenv("DYNAFLOW_LAUNCHER_LIBRARIES");

  auto libFile = lib + DYN::sharedLibraryExtension();
  // Check dynawo path
  auto libPath = DYN::getLibraryPathFromName(libFile);
  if (libPath) {
    return libPath;
  }

  if (dflLibEnv == NULL) {
    return boost::none;
  }

  auto libPathDfl = boost::filesystem::path(dflLibEnv);
  libPathDfl.append(libFile);
  if (!boost::filesystem::exists(libPathDfl)) {
    return boost::none;
  }

  return boost::make_optional(libPathDfl);
}

bool
DynModelAlgorithm::libraryExists(const std::string& lib) {
  try {
    // check DFL local path
    auto libPath = findLibraryPath(lib);
    if (!libPath) {
      return false;
    }
    boost::dll::shared_library sharedLib(libPath->generic_string());
    static_cast<void>(sharedLib);  // do nothing as we just want to check that the library can be loaded
    return true;
  } catch (const std::exception& e) {
    LOG(warn, CannotLoadLibrary, lib, e.what());
    return false;
  }
}

void
DynModelAlgorithm::extractMultiAssociationInfo(const inputs::AssemblingXmlDocument::DynamicAutomaton& automaton,
                                               const inputs::AssemblingXmlDocument::MacroConnect& macro,
                                               const inputs::AssemblingXmlDocument::MultipleAssociation& multiassoc) {
  MacroConnect connection(automaton.id, macro.macroConnection);
  const auto& vlid = multiassoc.shunt.voltageLevel;
  macroConnectByVlForShuntsId_[vlid].push_back(connection);
}

void
DynModelAlgorithm::extractSingleAssociationInfo(const inputs::AssemblingXmlDocument::DynamicAutomaton& automaton,
                                                const inputs::AssemblingXmlDocument::MacroConnect& macro,
                                                const inputs::AssemblingXmlDocument::SingleAssociation& singleassoc) {
  MacroConnect connection(automaton.id, macro.macroConnection);
  if (singleassoc.bus) {
    macroConnectByVlForBusesId_[singleassoc.bus->voltageLevel].insert(connection);
  } else if (singleassoc.line) {
    macroConnectByLineName_[singleassoc.line->name].push_back(connection);
  } else if (singleassoc.tfo) {
    macroConnectByTfoName_[singleassoc.tfo->name].push_back(connection);
  } else {
// Cannot happen if xml file is formed correctly according to its XSD
#if _DEBUG_
    assert(false);
#endif
  }
}

void
DynModelAlgorithm::extractDynModels(bool shuntRegulationOn) {
  using inputs::AssemblingXmlDocument;

  const auto& automatons = manager_.assemblingDocument().dynamicAutomatons();
  const auto& singleAssociations = manager_.assemblingDocument().singleAssociations();
  const auto& modelAssociations = manager_.assemblingDocument().modelAssociations();
  const auto& multiAssociations = manager_.assemblingDocument().multipleAssociations();

  // Use map instead of vector to find the associations
  std::unordered_map<std::string, AssemblingXmlDocument::SingleAssociation> singleAssociationsMap;
  for (const auto& asso : singleAssociations) {
    singleAssociationsMap[asso.id] = asso;
  }
  std::unordered_map<std::string, AssemblingXmlDocument::ModelAssociation> modelAssociationsMap;
  for (const auto& asso : modelAssociations) {
    modelAssociationsMap[asso.id] = asso;
  }
  std::unordered_map<std::string, AssemblingXmlDocument::MultipleAssociation> multiAssociationsMap;
  if (shuntRegulationOn) {
    for (const auto& asso : multiAssociations) {
      multiAssociationsMap[asso.id] = asso;
    }
  }

  for (const auto& automaton : automatons) {
    // Check that the automaton library is available
    if (!libraryExists(automaton.lib)) {
      LOG(warn, DynModelLibraryNotFound, automaton.lib, automaton.id);
      continue;
    }
    dynamicAutomatonsById_[automaton.id] = automaton;

    for (const auto& macro : automaton.macroConnects) {
      auto singleassoc = singleAssociationsMap.find(macro.id);
      if (singleassoc != singleAssociationsMap.end()) {
        extractSingleAssociationInfo(automaton, macro, singleassoc->second);
        continue;
      }

      auto modelassoc = modelAssociationsMap.find(macro.id);
      if (modelassoc != modelAssociationsMap.end()) {
        connectMacroConnectionForModel(automaton, macro, modelassoc->second);
        continue;
      }

      auto multiassoc = multiAssociationsMap.find(macro.id);
      if (multiassoc != multiAssociationsMap.end()) {
        extractMultiAssociationInfo(automaton, macro, multiassoc->second);
        continue;
      }
    }
  }
}

bool
DynModelAlgorithm::MacroConnect::operator==(const MacroConnect& other) const {
  return dynModelId == other.dynModelId && macroConnectionId == other.macroConnectionId;
}

bool
DynModelAlgorithm::MacroConnect::operator!=(const MacroConnect& other) const {
  return !((*this) == other);
}

bool
DynamicModelDefinition::MacroConnection::operator==(const MacroConnection& other) const {
  return id == other.id && elementType == other.elementType && connectedElementId == other.connectedElementId;
}

bool
DynamicModelDefinition::MacroConnection::operator!=(const MacroConnection& other) const {
  return !((*this) == other);
}

bool
DynamicModelDefinition::MacroConnection::operator<(const MacroConnection& other) const {
  return (id + std::to_string(static_cast<unsigned int>(elementType)) + connectedElementId) <
         (other.id + std::to_string(static_cast<unsigned int>(other.elementType)) + other.connectedElementId);
}

bool
DynamicModelDefinition::MacroConnection::operator<=(const MacroConnection& other) const {
  return (*this) < other || (*this) == other;
}

bool
DynamicModelDefinition::MacroConnection::operator>(const MacroConnection& other) const {
  return !((*this) <= other);
}

bool
DynamicModelDefinition::MacroConnection::operator>=(const MacroConnection& other) const {
  return (*this) > other || (*this) == other;
}

void
DynModelAlgorithm::connectMacroConnectionForModel(const inputs::AssemblingXmlDocument::DynamicAutomaton& automaton,
                                                  const inputs::AssemblingXmlDocument::MacroConnect& macro,
                                                  const inputs::AssemblingXmlDocument::ModelAssociation& modelassoc) {
  dynamicModels_.usedMacroConnections.insert(macro.macroConnection);
  addMacroConnectionToModelDefinitions(
      automaton,
      DynamicModelDefinition::MacroConnection(macro.macroConnection,
                                              DynamicModelDefinition::MacroConnection::ElementType::MODEL,
                                              modelassoc.model.id));
}

void
DynModelAlgorithm::connectMacroConnectionForShunt(const NodePtr& node) {
  // Connect all nodes of voltage level
  auto vl = node->voltageLevel.lock();
  const auto& macroConnections = macroConnectByVlForShuntsId_.at(vl->id);

  for (const auto& macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    // dynamic model id is present in the map as for now macroConnectByVlForShuntsId_ is filled only by dynamic automatons data
    assert(dynamicAutomatonsById_.count(macroConnection.dynModelId) > 0);
    const auto& automaton = dynamicAutomatonsById_.at(macroConnection.dynModelId);

    for (const auto& shunt : node->shunts) {
      addMacroConnectionToModelDefinitions(
          automaton,
          DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId, DynamicModelDefinition::MacroConnection::ElementType::SHUNT, shunt.id));
    }
  }
}

void
DynModelAlgorithm::connectMacroConnectionForLine(const std::shared_ptr<inputs::Line>& line) {
  const auto& macroConnections = macroConnectByLineName_.at(line->id);
  for (const auto& macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto& automaton = dynamicAutomatonsById_.at(macroConnection.dynModelId);

    addMacroConnectionToModelDefinitions(
        automaton,
        DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId, DynamicModelDefinition::MacroConnection::ElementType::LINE, line->id));
  }
}

void
DynModelAlgorithm::connectMacroConnectionForBus(const NodePtr& node) {
  auto vl = node->voltageLevel.lock();
  const auto& macroConnections = macroConnectByVlForBusesId_.at(vl->id);

  for (const auto& macroConnection : macroConnections) {
    // We use the first node available in the voltage level
    const auto& nodeId = vl->nodes.front()->id;

    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);  // Tag the used macro connection

    const auto& automaton = dynamicAutomatonsById_.at(macroConnection.dynModelId);
    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(
                                                        macroConnection.macroConnectionId, DynamicModelDefinition::MacroConnection::ElementType::NODE, nodeId));
  }
}

void
DynModelAlgorithm::connectMacroConnectionForTfo(const std::shared_ptr<inputs::Tfo>& tfo) {
  const auto& macroConnections = macroConnectByTfoName_.at(tfo->id);
  for (const auto& macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto& automaton = dynamicAutomatonsById_.at(macroConnection.dynModelId);

    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(
                                                        macroConnection.macroConnectionId, DynamicModelDefinition::MacroConnection::ElementType::TFO, tfo->id));
  }
}

void
DynModelAlgorithm::addMacroConnectionToModelDefinitions(const dfl::inputs::AssemblingXmlDocument::DynamicAutomaton& automaton,
                                                        const DynamicModelDefinition::MacroConnection& macroConnection) {
  if (dynamicModels_.models.count(automaton.id) == 0) {
    DynamicModelDefinition modelDef(automaton.id, automaton.lib);
    modelDef.nodeConnections.insert(macroConnection);

    dynamicModels_.models.insert({automaton.id, modelDef});
  } else {
    auto& modelDef = dynamicModels_.models.at(automaton.id);
    modelDef.nodeConnections.insert(macroConnection);
  }
}

void
DynModelAlgorithm::operator()(const NodePtr& node) {
  auto vl = node->voltageLevel.lock();
  if (macroConnectByVlForBusesId_.count(vl->id) > 0) {
    connectMacroConnectionForBus(node);
  }
  if (macroConnectByVlForShuntsId_.count(vl->id) > 0) {
    connectMacroConnectionForShunt(node);
  }
  for (const auto& line_ptr : node->lines) {
    auto line = line_ptr.lock();
    if (macroConnectByLineName_.count(line->id) > 0) {
      connectMacroConnectionForLine(line);
    }
  }
  for (const auto& tfo_ptr : node->tfos) {
    auto tfo = tfo_ptr.lock();
    if (macroConnectByTfoName_.count(tfo->id) > 0) {
      connectMacroConnectionForTfo(tfo);
    }
  }
}

std::size_t
DynModelAlgorithm::MacroConnectHash::operator()(const MacroConnect& connect) const noexcept {
  std::size_t seed = 0;
  boost::hash_combine(seed, connect.dynModelId);
  boost::hash_combine(seed, connect.macroConnectionId);
  return seed;
}
}  // namespace algo
}  // namespace dfl
