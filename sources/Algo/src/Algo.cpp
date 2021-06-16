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
#include <boost/make_shared.hpp>
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
  for (auto it = nodes.begin(); it != nodes.end(); ++it) {
    if (markedNodes_.count(*it) == 0) {
      markedNodes_.insert(*it);
      group.push_back(*it);
      updateConnexGroup(group, (*it)->neighbours);
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

  for (auto it = node->loads.begin(); it != node->loads.end(); ++it) {
    loads_.emplace_back(it->id, node->id);
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

AutomatonAlgorithm::AutomatonAlgorithm(AutomatonDefinitions& automatons, const inputs::AutomatonConfigurationManager& manager) :
    automatons_(automatons),
    manager_(manager) {
  extractAutomatons();
}

boost::optional<boost::filesystem::path>
AutomatonAlgorithm::computeLibPath(const std::string& lib) {
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
AutomatonAlgorithm::doesLibraryExist(const std::string& lib) {
  try {
    // check DFL local path
    auto libPath = computeLibPath(lib);
    if (!libPath) {
      return false;
    }
    auto sharedLib = boost::make_shared<boost::dll::shared_library>(libPath->generic_string());
    static_cast<void>(sharedLib);  // do nothing as we just want to check that the library can be loaded
    return true;
  } catch (const std::exception& e) {
    LOG(warn) << MESS(CannotLoadLibrary, lib, e.what()) << LOG_ENDL;
    return false;
  }
}

void
AutomatonAlgorithm::dispatchAutomatonMulti(const inputs::AssemblyXmlDocument::DynamicAutomaton& automaton,
                                           const inputs::AssemblyXmlDocument::MacroConnect& macro,
                                           const inputs::AssemblyXmlDocument::MultipleAssociation& multiassoc) {
  MacroConnect connection(automaton.id, macro.macroConnection);
  const auto& vlid = multiassoc.shunt.voltageLevel;
  macroConnectByVlId_[vlid] = connection;
}

void
AutomatonAlgorithm::dispatchAutomatonSingle(const inputs::AssemblyXmlDocument::DynamicAutomaton& automaton,
                                            const inputs::AssemblyXmlDocument::MacroConnect& macro,
                                            const inputs::AssemblyXmlDocument::SingleAssociation& singleassoc) {
  MacroConnect connection(automaton.id, macro.macroConnection);
  if (singleassoc.buses.size() > 0) {
    // The order of nodes ids in the vector is relevant to find afterwards the correct node to connect to
    std::vector<inputs::Node::NodeId> nodeIds;
    for (const auto& bus : singleassoc.buses) {
      // assuming that bus name in xml config follows: <vlid><number>.<sub_number>
      // and assuming that the bus name in network follows one of those:
      // - <vlid><number>
      // - <vlid><number>.<sub_number>
      //
      // We link the connection to all candidate nodes. The main algorithm will decide which node must eventually have the one connection
      auto pos = bus.name.find(".");
      if (pos != std::string::npos) {
        auto nodeid = bus.voltageLevel + bus.name.substr(0, bus.name.find("."));
        macroConnectByNodeId_[nodeid] = connection;
        nodeIds.push_back(nodeid);
      }

      auto nodeid = bus.voltageLevel + bus.name;
      macroConnectByNodeId_[nodeid] = connection;
      nodeIds.push_back(nodeid);
    }
    macroConnectNodeChoices_.insert({connection, FirstNodeStrategy(nodeIds)});
  } else if (singleassoc.line) {
    macroConnectByLineName_[singleassoc.line->name].push_back(connection);
  } else if (singleassoc.tfo) {
    macroConnectByTfoName_[singleassoc.tfo->name].push_back(connection);
  } else {
// Cannot happen if xml file is formed correctly
#if _DEBUG_
    assert(false);
#endif
  }
}

void
AutomatonAlgorithm::extractAutomatons() {
  const auto& automatons = manager_.assemblyDocument().dynamicAutomatons();

  for (const auto& automaton : automatons) {
    // Check that the automaton library is accessible
    if (!doesLibraryExist(automaton.lib)) {
      LOG(warn) << MESS(AutomatonLibraryNotFound, automaton.lib, automaton.id) << LOG_ENDL;
      continue;
    }
    automatonsById_[automaton.id] = automaton;

    const auto& singleassociations = manager_.assemblyDocument().singleAssociations();
    const auto& multiassociations = manager_.assemblyDocument().multipleAssociations();
    for (const auto& macro : automaton.macroConnects) {
      auto singleassoc = std::find_if(singleassociations.begin(), singleassociations.end(),
                                      [&macro](const inputs::AssemblyXmlDocument::SingleAssociation& singleassoc) { return singleassoc.id == macro.id; });
      if (singleassoc != singleassociations.end()) {
        dispatchAutomatonSingle(automaton, macro, *singleassoc);
        continue;
      }

      auto multiassoc = std::find_if(multiassociations.begin(), multiassociations.end(),
                                     [&macro](const inputs::AssemblyXmlDocument::MultipleAssociation& multiassoc) { return multiassoc.id == macro.id; });
      if (multiassoc != multiassociations.end()) {
        dispatchAutomatonMulti(automaton, macro, *multiassoc);
        continue;
      }
    }
  }
}

bool
AutomatonAlgorithm::MacroConnect::operator==(const MacroConnect& other) const {
  return automatonId == other.automatonId && macroConnectionId == other.macroConnectionId;
}

bool
AutomatonAlgorithm::MacroConnect::operator!=(const MacroConnect& other) const {
  return !((*this) == other);
}

bool
AutomatonDefinition::MacroConnection::operator==(const MacroConnection& other) const {
  return id == other.id && connectedElementId == other.connectedElementId;
}

bool
AutomatonDefinition::MacroConnection::operator!=(const MacroConnection& other) const {
  return !((*this) == other);
}

bool
AutomatonDefinition::MacroConnection::operator<(const MacroConnection& other) const {
  return (id + connectedElementId) < (other.id + other.connectedElementId);
}

bool
AutomatonDefinition::MacroConnection::operator<=(const MacroConnection& other) const {
  return (*this) < other || (*this) == other;
}

bool
AutomatonDefinition::MacroConnection::operator>(const MacroConnection& other) const {
  return !((*this) <= other);
}

bool
AutomatonDefinition::MacroConnection::operator>=(const MacroConnection& other) const {
  return (*this) > other || (*this) == other;
}

void
AutomatonAlgorithm::processAutomatonShuntConnection(const NodePtr& node) {
  // Connect All nodes of voltage level
  auto vl = node->voltageLevel.lock();
  const auto& macroConnection = macroConnectByVlId_.at(vl->id);

  automatons_.usedMacroConnections.insert(macroConnection.macroConnectionId);
  const auto& automaton = automatonsById_.at(macroConnection.automatonId);

  const auto& nodes = vl->nodes;
  for (const auto& node : nodes) {
    if (node->nbShunts > 0) {
      addMacroConnectionToDef(automaton, AutomatonDefinition::MacroConnection(macroConnection.macroConnectionId, node->id));
    }
  }
}

void
AutomatonAlgorithm::processAutomatonLineConnection(const std::shared_ptr<inputs::Line>& line) {
  const auto& macroConnections = macroConnectByLineName_.at(line->id);
  for (const auto& macroConnection : macroConnections) {
    automatons_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto& automaton = automatonsById_.at(macroConnection.automatonId);

    addMacroConnectionToDef(automaton, AutomatonDefinition::MacroConnection(macroConnection.macroConnectionId, line->id));
  }
}

void
AutomatonAlgorithm::FirstNodeStrategy::choose(const std::shared_ptr<inputs::VoltageLevel>& vl) {
  // Find the first candidate node in the voltage level node list
  for (const auto& nodeId : candidateNodes_) {
    auto found = std::find_if(vl->nodes.begin(), vl->nodes.end(), [&nodeId](const NodePtr& node) { return nodeId == node->id; });
    if (found != vl->nodes.end()) {
      chosenNodeId_ = nodeId;
      return;
    }
  }
}

void
AutomatonAlgorithm::processAutomatonBusConnection(const NodePtr& node) {
  const auto& macroConnection = macroConnectByNodeId_.at(node->id);

  // In case of macroconnection on a bus, macroConnectByNodeId_ shows only macro connects for which the node is candidate
  // We have to process the voltage level of this node and add only one "real" macro connection
  // according to its specific algorithm
  auto& strategy = macroConnectNodeChoices_.at(macroConnection);
  if (strategy.isProcessed()) {
    return;
  }
  strategy.choose(node->voltageLevel.lock());
  if (!strategy.isProcessed()) {
    // case no node of the voltage level matches a candidate node: Possible configuration error
    LOG(debug) << "No node of the voltage level " << node->voltageLevel.lock()->id << " matches the candidate nodes for macro connection "
               << macroConnection.macroConnectionId << " for automaton " << macroConnection.automatonId << LOG_ENDL;
    return;
  }
  const auto& nodeId = strategy.chosenNodeId();

  automatons_.usedMacroConnections.insert(macroConnection.macroConnectionId);  // Tag the used macro connection

  const auto& automaton = automatonsById_.at(macroConnection.automatonId);
  addMacroConnectionToDef(automaton, AutomatonDefinition::MacroConnection(macroConnection.macroConnectionId, nodeId));
}

void
AutomatonAlgorithm::processAutomatonTfoConnection(const std::shared_ptr<inputs::Tfo>& tfo) {
  const auto& macroConnections = macroConnectByTfoName_.at(tfo->id);
  for (const auto& macroConnection : macroConnections) {
    automatons_.usedMacroConnections.insert(macroConnection.macroConnectionId);
    const auto& automaton = automatonsById_.at(macroConnection.automatonId);

    addMacroConnectionToDef(automaton, AutomatonDefinition::MacroConnection(macroConnection.macroConnectionId, tfo->id));
  }
}

void
AutomatonAlgorithm::addMacroConnectionToDef(const dfl::inputs::AssemblyXmlDocument::DynamicAutomaton& automaton,
                                            const AutomatonDefinition::MacroConnection& macroConnection) {
  if (automatons_.automatons.count(automaton.id) == 0) {
    AutomatonDefinition automatonDef(automaton.id, automaton.lib);
    automatonDef.nodeConnections.insert(macroConnection);

    automatons_.automatons.insert({automaton.id, automatonDef});
  } else {
    auto& automatonDef = automatons_.automatons.at(automaton.id);
    automatonDef.nodeConnections.insert(macroConnection);
  }
}

void
AutomatonAlgorithm::operator()(const NodePtr& node) {
  if (macroConnectByNodeId_.count(node->id) > 0) {
    processAutomatonBusConnection(node);
  }
  if (macroConnectByVlId_.count(node->voltageLevel.lock()->id) > 0) {
    processAutomatonShuntConnection(node);
  }
  for (const auto& line_ptr : node->lines) {
    auto line = line_ptr.lock();
    if (macroConnectByLineName_.count(line->id) > 0) {
      processAutomatonLineConnection(line);
    }
  }
  for (const auto& tfo_ptr : node->tfos) {
    auto tfo = tfo_ptr.lock();
    if (macroConnectByTfoName_.count(tfo->id) > 0) {
      processAutomatonTfoConnection(tfo);
    }
  }
}

std::size_t
AutomatonAlgorithm::MacroConnectHash::operator()(const MacroConnect& connect) const noexcept {
  std::size_t seed = 0;
  boost::hash_combine(seed, connect.automatonId);
  boost::hash_combine(seed, connect.macroConnectionId);
  return seed;
}

/////////////////////////////////////////////////////////////////////////////////

CounterAlgorithm::CounterAlgorithm(CounterDefinitions& defs) : defs_(defs) {}

void
CounterAlgorithm::operator()(const NodePtr& node) {
  if (node->voltageLevel.expired()) {
    return;
  }
  auto vl = node->voltageLevel.lock();
  defs_.nbShunts[vl->id] += node->nbShunts;
}

}  // namespace algo
}  // namespace dfl
