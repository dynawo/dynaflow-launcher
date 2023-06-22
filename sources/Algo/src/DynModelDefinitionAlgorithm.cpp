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

DynModelAlgorithm::DynModelAlgorithm(DynamicModelDefinitions &models, const inputs::DynamicDataBaseManager &manager, bool shuntRegulationOn)
    : dynamicModels_(models), manager_(manager) {
  extractDynModels(shuntRegulationOn);
}

boost::optional<boost::filesystem::path> DynModelAlgorithm::findLibraryPath(const std::string &lib) {
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

bool DynModelAlgorithm::libraryExists(const std::string &lib) {
  try {
    // check DFL local path
    auto libPath = findLibraryPath(lib);
    if (!libPath) {
      return false;
    }
    boost::dll::shared_library sharedLib(libPath->generic_string());
    static_cast<void>(sharedLib);  // do nothing as we just want to check that the library can be loaded
    return true;
  } catch (const std::exception &e) {
    LOG(warn, CannotLoadLibrary, lib, e.what());
    return false;
  }
}

void DynModelAlgorithm::extractMultiAssociationInfo(const inputs::AssemblingDataBase::DynamicAutomaton &automaton,
                                                    const inputs::AssemblingDataBase::MacroConnect &macro, bool shuntRegulationOn) {
  MacroConnect connection(automaton.id, macro.macroConnection);
  auto &multiAssoc = manager_.assembling().getMultipleAssociation(macro.id);
  if (shuntRegulationOn && multiAssoc.shunt) {
    const auto &vlid = multiAssoc.shunt->voltageLevel;
    macroConnectByVlForShuntsId_[vlid].push_back(connection);
  }
}

void DynModelAlgorithm::extractSingleAssociationInfo(const inputs::AssemblingDataBase::DynamicAutomaton &automaton,
                                                     const inputs::AssemblingDataBase::MacroConnect &macro) {
  MacroConnect connection(automaton.id, macro.macroConnection);
  auto &singleAssoc = manager_.assembling().getSingleAssociation(macro.id);
  for (const auto &generator : singleAssoc.generators) {
    macroConnectByGeneratorName_[generator.name].push_back(connection);
  }
  for (const auto &load : singleAssoc.loads) {
    macroConnectByLoadName_[load.name].push_back(connection);
  }
  if (singleAssoc.bus) {
    macroConnectByVlForBusesId_[singleAssoc.bus->voltageLevel].insert(connection);
  } else if (singleAssoc.line) {
    macroConnectByLineName_[singleAssoc.line->name].push_back(connection);
  } else if (singleAssoc.hvdcLine) {
    macroConnectByHvdcName_[singleAssoc.hvdcLine->name].push_back(connection);
  } else if (singleAssoc.tfo) {
    macroConnectByTfoName_[singleAssoc.tfo->name].push_back(connection);
  } else if (singleAssoc.shunt) {
    macroConnectByShuntName_[singleAssoc.shunt->name].push_back(connection);
  }
}

void DynModelAlgorithm::extractDynModels(bool shuntRegulationOn) {
  for (const auto &automaton : manager_.assembling().dynamicAutomatons()) {
    // Check that the automaton library is available
    if (!libraryExists(automaton.second.lib)) {
      LOG(warn, DynModelLibraryNotFound, automaton.second.lib, automaton.first);
      continue;
    }

    for (const auto &macro : automaton.second.macroConnects) {
      if (manager_.assembling().isSingleAssociation(macro.id)) {
        extractSingleAssociationInfo(automaton.second, macro);
      } else if (manager_.assembling().isMultipleAssociation(macro.id)) {
        extractMultiAssociationInfo(automaton.second, macro, shuntRegulationOn);
        continue;
      } else if (manager_.assembling().dynamicAutomatons().count(macro.id) > 0) {
        connectMacroConnectionForDynAutomaton(automaton.second, macro);
        continue;
      }
    }
  }
}

bool DynModelAlgorithm::MacroConnect::operator==(const MacroConnect &other) const {
  return dynModelId == other.dynModelId && macroConnectionId == other.macroConnectionId;
}

bool DynModelAlgorithm::MacroConnect::operator!=(const MacroConnect &other) const { return !((*this) == other); }

bool DynamicModelDefinition::MacroConnection::operator==(const MacroConnection &other) const {
  return id == other.id && elementType == other.elementType && connectedElementId == other.connectedElementId;
}

bool DynamicModelDefinition::MacroConnection::operator!=(const MacroConnection &other) const { return !((*this) == other); }

bool DynamicModelDefinition::MacroConnection::operator<(const MacroConnection &other) const {
  return (id + std::to_string(static_cast<unsigned int>(elementType)) + connectedElementId) <
         (other.id + std::to_string(static_cast<unsigned int>(other.elementType)) + other.connectedElementId);
}

bool DynamicModelDefinition::MacroConnection::operator<=(const MacroConnection &other) const { return (*this) < other || (*this) == other; }

bool DynamicModelDefinition::MacroConnection::operator>(const MacroConnection &other) const { return !((*this) <= other); }

bool DynamicModelDefinition::MacroConnection::operator>=(const MacroConnection &other) const { return (*this) > other || (*this) == other; }

void DynModelAlgorithm::connectMacroConnectionForDynAutomaton(const inputs::AssemblingDataBase::DynamicAutomaton &automaton,
                                                              const inputs::AssemblingDataBase::MacroConnect &macro) {
  dynamicModels_.usedMacroConnections.insert(macro.macroConnection);
  const auto &macroConn = manager_.assembling().getMacroConnection(macro.macroConnection);
  addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macro.macroConnection,
                                                                                          DynamicModelDefinition::MacroConnection::ElementType::AUTOMATON,
                                                                                          macro.id, macroConn.indexId));
}

void DynModelAlgorithm::connectMacroConnectionForMultipleShunts(const NodePtr &node) {
  // Connect all nodes of voltage level
  auto vl = node->voltageLevel.lock();
  const auto &macroConnections = macroConnectByVlForShuntsId_.at(vl->id);

  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    // dynamic model id is present in the map as for now macroConnectByVlForShuntsId_ is filled only by dynamic automatons data
    assert(manager_.assembling().dynamicAutomatons().count(macroConnection.dynModelId) > 0);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);

    for (const auto &shunt : node->shunts) {
      addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                              DynamicModelDefinition::MacroConnection::ElementType::SHUNT,
                                                                                              shunt.id, macroConn.indexId));
    }
  }
}

void DynModelAlgorithm::connectMacroConnectionForLine(const std::shared_ptr<inputs::Line> &line) {
  const auto &macroConnections = macroConnectByLineName_.at(line->id);
  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);

    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::LINE,
                                                                                            line->id, macroConn.indexId));
  }
}

void DynModelAlgorithm::connectMacroConnectionForSingleShunt(const inputs::Shunt &shunt) {
  const auto &macroConnections = macroConnectByShuntName_.at(shunt.id);
  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);

    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::SHUNT,
                                                                                            shunt.id, macroConn.indexId));
  }
}

void DynModelAlgorithm::connectMacroConnectionForBus(const NodePtr &node) {
  auto vl = node->voltageLevel.lock();
  const auto &macroConnections = macroConnectByVlForBusesId_.at(vl->id);

  for (const auto &macroConnection : macroConnections) {
    dfl::inputs::Node::NodeId nodeId;
    for (const auto &nodeVL : vl->nodes) {
      if (nodeVL->isBusConnected()) {
        nodeId = nodeVL->id;
        break;
      }
    }
    if (nodeId.empty())
      return;  // ignore this connection

    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);  // Tag the used macro connection
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);

    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::NODE, nodeId,
                                                                                            macroConn.indexId));
  }
}

void DynModelAlgorithm::connectMacroConnectionForTfo(const std::shared_ptr<inputs::Tfo> &tfo) {
  const auto &macroConnections = macroConnectByTfoName_.at(tfo->id);
  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);

    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::TFO, tfo->id,
                                                                                            macroConn.indexId));
  }
}

void DynModelAlgorithm::connectMacroConnectionForGenerator(const inputs::Generator &generator) {
  const auto &macroConnections = macroConnectByGeneratorName_.at(generator.id);
  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);
    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::GENERATOR,
                                                                                            generator.id, macroConn.indexId));
  }
}
void DynModelAlgorithm::connectMacroConnectionForLoad(const inputs::Load &load) {
  const auto &macroConnections = macroConnectByLoadName_.at(load.id);
  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);
    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::LOAD, load.id,
                                                                                            macroConn.indexId));
  }
}

void DynModelAlgorithm::connectMacroConnectionForHvdc(const inputs::HvdcLine &hvdcLine) {
  const auto &macroConnections = macroConnectByHvdcName_.at(hvdcLine.id);
  for (const auto &macroConnection : macroConnections) {
    dynamicModels_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto &automaton = manager_.assembling().dynamicAutomatons().at(macroConnection.dynModelId);
    const auto &macroConn = manager_.assembling().getMacroConnection(macroConnection.macroConnectionId);
    addMacroConnectionToModelDefinitions(automaton, DynamicModelDefinition::MacroConnection(macroConnection.macroConnectionId,
                                                                                            DynamicModelDefinition::MacroConnection::ElementType::HVDC,
                                                                                            hvdcLine.id, macroConn.indexId));
  }
}

void DynModelAlgorithm::addMacroConnectionToModelDefinitions(const dfl::inputs::AssemblingDataBase::DynamicAutomaton &automaton,
                                                             const DynamicModelDefinition::MacroConnection &macroConnection) {
  if (dynamicModels_.models.count(automaton.id) == 0) {
    DynamicModelDefinition modelDef(automaton.id, automaton.lib);
    modelDef.nodeConnections.insert(macroConnection);

    dynamicModels_.models.insert({automaton.id, modelDef});
  } else {
    auto &modelDef = dynamicModels_.models.at(automaton.id);
    modelDef.nodeConnections.insert(macroConnection);
  }
}

void DynModelAlgorithm::operator()(const NodePtr &node, std::shared_ptr<AlgorithmsResults> &) {
  auto vl = node->voltageLevel.lock();
  if (macroConnectByVlForBusesId_.count(vl->id) > 0) {
    connectMacroConnectionForBus(node);
  }
  if (macroConnectByVlForShuntsId_.count(vl->id) > 0) {
    connectMacroConnectionForMultipleShunts(node);
  }
  for (const auto &shunt : node->shunts) {
    if (macroConnectByShuntName_.count(shunt.id) > 0) {
      connectMacroConnectionForSingleShunt(shunt);
    }
  }
  for (const auto &line_ptr : node->lines) {
    auto line = line_ptr.lock();
    if (macroConnectByLineName_.count(line->id) > 0) {
      connectMacroConnectionForLine(line);
    }
  }
  for (const auto &tfo_ptr : node->tfos) {
    auto tfo = tfo_ptr.lock();
    if (macroConnectByTfoName_.count(tfo->id) > 0) {
      connectMacroConnectionForTfo(tfo);
    }
  }
  for (const auto &gen : node->generators) {
    if (macroConnectByGeneratorName_.count(gen.id) > 0) {
      connectMacroConnectionForGenerator(gen);
    }
  }
  for (const auto &load : node->loads) {
    if (macroConnectByLoadName_.count(load.id) > 0) {
      connectMacroConnectionForLoad(load);
    }
  }
  for (const auto &converter : node->converters) {
    if (macroConnectByHvdcName_.count(converter.lock()->hvdcLine->id) > 0) {
      connectMacroConnectionForHvdc(*converter.lock()->hvdcLine);
    }
  }
}

std::size_t DynModelAlgorithm::MacroConnectHash::operator()(const MacroConnect &connect) const noexcept {
  std::size_t seed = 0;
  boost::hash_combine(seed, connect.dynModelId);
  boost::hash_combine(seed, connect.macroConnectionId);
  return seed;
}
}  // namespace algo
}  // namespace dfl
