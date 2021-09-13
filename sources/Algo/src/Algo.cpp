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
 * @file  Algo.cpp
 *
 * @brief Dynaflow launcher algorithms implementation file
 *
 */
#include "Algo.h"

#include "HvdcLine.h"
#include "Log.h"
#include "Message.hpp"

#include <DYNCommon.h>
#include <DYNExecUtils.h>
#include <boost/dll/import.hpp>
#include <boost/functional.hpp>
#include <tuple>

namespace dfl {
namespace algo {

SlackNodeAlgorithm::SlackNodeAlgorithm(NodePtr& slackNode) : NodeAlgorithm(), slackNode_(slackNode) {}

void
SlackNodeAlgorithm::operator()(const NodePtr& node) {
  if (!slackNode_) {
    slackNode_ = node;
  } else {
    if (std::forward_as_tuple(slackNode_->nominalVoltage, slackNode_->neighbours.size()) <
        std::forward_as_tuple(node->nominalVoltage, node->neighbours.size())) {
      slackNode_ = node;
    }
  }
}

/////////////////////////////////////////////////////////

MainConnexComponentAlgorithm::MainConnexComponentAlgorithm(ConnexGroup& mainConnexity) : NodeAlgorithm(), markedNodes_{}, mainConnexity_(mainConnexity) {}

void
MainConnexComponentAlgorithm::updateConnexGroup(ConnexGroup& group, const std::vector<NodePtr>& nodes) {
  for (const auto& node : nodes) {
    if (markedNodes_.count(node) == 0) {
      markedNodes_.insert(node);
      group.push_back(node);
      updateConnexGroup(group, (node)->neighbours);
    }
  }
}

void
MainConnexComponentAlgorithm::operator()(const NodePtr& node) {
  if (markedNodes_.count(node) > 0) {
    // already processed
    return;
  }

  ConnexGroup group;
  markedNodes_.insert(node);
  group.push_back(node);
  updateConnexGroup(group, node->neighbours);

  if (mainConnexity_.size() < group.size()) {
    mainConnexity_.swap(group);
  }
}

////////////////////////////////////////////////////////////////

GeneratorDefinitionAlgorithm::GeneratorDefinitionAlgorithm(Generators& gens, BusGenMap& busesWithDynamicModel,
                                                           const inputs::NetworkManager::BusMapRegulating& busMap, bool infinitereactivelimits,
                                                           const boost::shared_ptr<DYN::ServiceManagerInterface>& serviceManager) :
    NodeAlgorithm(),
    generators_(gens),
    busesWithDynamicModel_(busesWithDynamicModel),
    busMap_(busMap),
    useInfiniteReactivelimits_{infinitereactivelimits},
    serviceManager_(serviceManager) {}

void
GeneratorDefinitionAlgorithm::operator()(const NodePtr& node) {
  auto& node_generators = node->generators;

  auto isModelWithInvalidDiagram = [](GeneratorDefinition::ModelType model, inputs::Generator generator) {
    return (model == GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN || model == GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN ||
            model == GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN) &&
           !isDiagramValid(generator);
  };

  for (const auto& generator : node_generators) {
    auto it = busMap_.find(generator.regulatedBusId);
    assert(it != busMap_.end());
    auto nbOfRegulatingGenerators = it->second;
    GeneratorDefinition::ModelType model = GeneratorDefinition::ModelType::SIGNALN;
    if (node_generators.size() == 1 && IsOtherGeneratorConnectedBySwitches(node)) {
      model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::PROP_SIGNALN : GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN;
      if (!isModelWithInvalidDiagram(model, generator)) {
        busesWithDynamicModel_.insert({generator.regulatedBusId, generator.id});
      }
    } else {
      switch (nbOfRegulatingGenerators) {
      case dfl::inputs::NetworkManager::NbOfRegulatingGenerators::ONE:
        if (generator.regulatedBusId == generator.connectedBusId) {
          model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::SIGNALN : GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN;
        } else {
          model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::REMOTE_SIGNALN : GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN;
        }
        break;
      case dfl::inputs::NetworkManager::NbOfRegulatingGenerators::MULTIPLES:
        model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::PROP_SIGNALN : GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN;
        if (!isModelWithInvalidDiagram(model, generator)) {
          busesWithDynamicModel_.insert({generator.regulatedBusId, generator.id});
        }
        break;
      default:  //  impossible by definition of the enum
        break;
      }
    }
    if (isModelWithInvalidDiagram(model, generator)) {
      continue;
    }
    generators_.emplace_back(generator.id, model, node->id, generator.points, generator.qmin, generator.qmax, generator.pmin, generator.pmax, generator.targetP,
                             generator.regulatedBusId);
  }
}

bool
GeneratorDefinitionAlgorithm::isDiagramValid(const inputs::Generator& generator) {
  // If there are no points, the diagram will be constructed from the pmin, pmax, qmin and qmax values.
  // We check the validity of pmin,pmax and qmin,qmax values
  if (generator.points.empty()) {
    if (DYN::doubleEquals(generator.pmin, generator.pmax)) {
      LOG(warn) << MESS(InvalidDiagramAllPEqual, generator.id) << LOG_ENDL;
      return false;
    }
    if (DYN::doubleEquals(generator.qmin, generator.qmax)) {
      LOG(warn) << MESS(InvalidDiagramQminsEqualQmaxs, generator.id) << LOG_ENDL;
      return false;
    }
    return true;
  }

  // If there is only one point, the diagram is not valid
  if (generator.points.size() == 1) {
    LOG(warn) << MESS(InvalidDiagramOnePoint, generator.id) << LOG_ENDL;
    return false;
  }

  auto firstP = generator.points.front().p;
  bool allQminEqualQmax = true;
  bool allPEqual = true;
  auto it = generator.points.begin();
  while ((allQminEqualQmax || allPEqual) && it != generator.points.end()) {
    allQminEqualQmax = allQminEqualQmax && it->qmin == it->qmax;
    allPEqual = allPEqual && it->p == firstP;
    ++it;
  }
  bool valid = !allQminEqualQmax && !allPEqual;

  if (!valid) {
    if (allQminEqualQmax && allPEqual) {
      LOG(warn) << MESS(InvalidDiagramBothError, generator.id) << LOG_ENDL;
    } else if (allQminEqualQmax) {
      LOG(warn) << MESS(InvalidDiagramQminsEqualQmaxs, generator.id) << LOG_ENDL;
    } else if (allPEqual) {
      LOG(warn) << MESS(InvalidDiagramAllPEqual, generator.id) << LOG_ENDL;
    }
  }
  return valid;
}

bool
GeneratorDefinitionAlgorithm::IsOtherGeneratorConnectedBySwitches(const NodePtr& node) const {
  auto vl = node->voltageLevel.lock();
  auto buses = serviceManager_->getBusesConnectedBySwitch(node->id, vl->id);

  if (buses.size() == 0) {
    return false;
  }

  for (const auto& id : buses) {
    auto found = std::find_if(vl->nodes.begin(), vl->nodes.end(), [&id](const NodePtr& node) { return node->id == id; });
#ifdef _DEBUG_
    // shouldn't happen by construction of the elements
    assert(found != vl->nodes.end());
#endif
    if ((*found)->generators.size() != 0) {
      return true;
    }
  }

  return false;
}

/////////////////////////////////////////////////////////////////

LoadDefinitionAlgorithm::LoadDefinitionAlgorithm(Loads& loads, double dsoVoltageLevel) : NodeAlgorithm(), loads_(loads), dsoVoltageLevel_(dsoVoltageLevel) {}

void
LoadDefinitionAlgorithm::operator()(const NodePtr& node) {
  if (DYN::doubleNotEquals(node->nominalVoltage, dsoVoltageLevel_) && node->nominalVoltage < dsoVoltageLevel_) {
    return;
  }

  for (const auto& load : node->loads) {
    loads_.emplace_back(load.id, node->id);
  }
}
/////////////////////////////////////////////////////////////////

ControllerInterfaceDefinitionAlgorithm::ControllerInterfaceDefinitionAlgorithm(HvdcLineMap& hvdcLines) : hvdcLines_(hvdcLines) {}

void
ControllerInterfaceDefinitionAlgorithm::operator()(const NodePtr& node) {
  for (const auto& converter : node->converterInterfaces) {
    const auto& hvdcLine = converter.hvdcLine;
    HvdcLineDefinition createdHvdcLine = HvdcLineDefinition(hvdcLine->id, hvdcLine->converterType, hvdcLine->converter1_id, hvdcLine->converter1_busId,
                                                            hvdcLine->converter1_voltageRegulationOn, hvdcLine->converter2_id, hvdcLine->converter2_busId,
                                                            hvdcLine->converter2_voltageRegulationOn, HvdcLineDefinition::Position::BOTH_IN_MAIN_COMPONENT);
    auto it = hvdcLines_.find(hvdcLine->id);
    bool alreadyInserted = it != hvdcLines_.end();
    if (alreadyInserted) {
      it->second.position = HvdcLineDefinition::Position::BOTH_IN_MAIN_COMPONENT;
      // Once we want to use the hvdcLine with both converters in main component, we should remove the line that erase the hvdcLine
      // from the hvdcLines_ map.
      hvdcLines_.erase(it);
      continue;
    }

    if (converter.converterId == hvdcLine->converter1_id) {
      createdHvdcLine.position = HvdcLineDefinition::Position::FIRST_IN_MAIN_COMPONENT;
    } else if (converter.converterId == hvdcLine->converter2_id) {
      createdHvdcLine.position = HvdcLineDefinition::Position::SECOND_IN_MAIN_COMPONENT;
    } else {
      LOG(error) << MESS(HvdcLineBadInitialization, hvdcLine->id) << LOG_ENDL;
      continue;
    }

    hvdcLines_.emplace(hvdcLine->id, createdHvdcLine);
  }
}

///////////////////////////////////////////////////////////////////////////

DynModelAlgorithm::DynModelAlgorithm(DynamicModelDefinitions& models, const inputs::DynamicDataBaseManager& manager) :
    dynamicModels_(models),
    manager_(manager) {
  extractDynModels();
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
    LOG(warn) << MESS(CannotLoadLibrary, lib, e.what()) << LOG_ENDL;
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
DynModelAlgorithm::extractDynModels() {
  using inputs::AssemblingXmlDocument;

  const auto& automatons = manager_.assemblingDocument().dynamicAutomatons();
  const auto& singleAssociations = manager_.assemblingDocument().singleAssociations();
  const auto& multiAssociations = manager_.assemblingDocument().multipleAssociations();

  // Use map instead of vector to find the associations
  std::unordered_map<std::string, AssemblingXmlDocument::SingleAssociation> singleAssociationsMap;
  for (const auto& asso : singleAssociations) {
    singleAssociationsMap[asso.id] = asso;
  }
  std::unordered_map<std::string, AssemblingXmlDocument::MultipleAssociation> multiAssociationsMap;
  for (const auto& asso : multiAssociations) {
    multiAssociationsMap[asso.id] = asso;
  }

  for (const auto& automaton : automatons) {
    // Check that the automaton library is available
    if (!libraryExists(automaton.lib)) {
      LOG(warn) << MESS(DynModelLibraryNotFound, automaton.lib, automaton.id) << LOG_ENDL;
      continue;
    }
    dynamicAutomatonsById_[automaton.id] = automaton;

    for (const auto& macro : automaton.macroConnects) {
      auto singleassoc = singleAssociationsMap.find(macro.id);
      if (singleassoc != singleAssociationsMap.end()) {
        extractSingleAssociationInfo(automaton, macro, singleassoc->second);
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

/////////////////////////////////////////////////////////////////////////////////

ShuntCounterAlgorithm::ShuntCounterAlgorithm(ShuntCounterDefinitions& defs) : shuntCounterDefs_(defs) {}

void
ShuntCounterAlgorithm::operator()(const NodePtr& node) {
  auto vl = node->voltageLevel.lock();
  shuntCounterDefs_.nbShunts[vl->id] += node->shunts.size();
}

//////////////////////////////////////////////////////////////////////////////////

LinesByIdAlgorithm::LinesByIdAlgorithm(LinesByIdDefinitions& linesByIdDefinition) : linesByIdDefinition_(linesByIdDefinition) {}

void
LinesByIdAlgorithm::operator()(const NodePtr& node) {
  const auto& lines = node->lines;
  for (const auto& line_ptr : lines) {
    auto line = line_ptr.lock();
    if (linesByIdDefinition_.linesMap.count(line->id) > 0) {
      continue;
    }

    linesByIdDefinition_.linesMap.insert({line->id, *line});
  }
}

////////////////////////////////////////////////////////////////////////////////////

StaticVarCompensatorAlgorithm::StaticVarCompensatorAlgorithm(StaticVarCompensatorDefinitions& svarcsDefinitions) : svarcsDefinitions_(svarcsDefinitions) {}

void
StaticVarCompensatorAlgorithm::operator()(const NodePtr& node) {
  const auto& svarcs = node->svarcs;
  std::transform(svarcs.begin(), svarcs.end(), std::back_inserter(svarcsDefinitions_.svarcs),
                 [](const inputs::StaticVarCompensator& svarc) { return std::ref(svarc); });
}

}  // namespace algo
}  // namespace dfl
