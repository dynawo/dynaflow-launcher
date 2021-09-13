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
      case dfl::inputs::NetworkManager::NbOfRegulating::ONE:
        if (generator.regulatedBusId == generator.connectedBusId) {
          model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::SIGNALN : GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN;
        } else {
          model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::REMOTE_SIGNALN : GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN;
        }
        break;
      case dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES:
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

HVDCDefinitionAlgorithm::HVDCDefinitionAlgorithm(HVDCLineDefinitions& hvdcLinesDefinitions, bool infiniteReactiveLimits,
                                                 const inputs::NetworkManager::BusMapRegulating& mapBusVSCConvertersBusId) :
    hvdcLinesDefinitions_(hvdcLinesDefinitions),
    infiniteReactiveLimits_(infiniteReactiveLimits),
    mapBusVSCConvertersBusId_(mapBusVSCConvertersBusId) {}

auto
HVDCDefinitionAlgorithm::getBusRegulatedByMultipleVSC(const inputs::HvdcLine& hvdcLine, HVDCDefinition::Position position) const
    -> std::vector<HVDCModelDefinition::VSCBusPair> {
  std::vector<HVDCModelDefinition::VSCBusPair> ret;
  switch (position) {
  case HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT: {
    auto found = mapBusVSCConvertersBusId_.find(hvdcLine.converter1->busId);
    if (found != mapBusVSCConvertersBusId_.end() && found->second == inputs::NetworkManager::NbOfRegulating::MULTIPLES) {
      ret.push_back(std::make_pair(hvdcLine.converter1->busId, hvdcLine.converter1->converterId));
    }
    break;
  }
  case HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT: {
    auto found = mapBusVSCConvertersBusId_.find(hvdcLine.converter2->busId);
    if (found != mapBusVSCConvertersBusId_.end() && found->second == inputs::NetworkManager::NbOfRegulating::MULTIPLES) {
      ret.push_back(std::make_pair(hvdcLine.converter2->busId, hvdcLine.converter2->converterId));
    }
    break;
  }
  case HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT: {
    auto found = mapBusVSCConvertersBusId_.find(hvdcLine.converter1->busId);
    auto found2 = mapBusVSCConvertersBusId_.find(hvdcLine.converter2->busId);
    if ((found != mapBusVSCConvertersBusId_.end() && found->second == inputs::NetworkManager::NbOfRegulating::MULTIPLES) ||
        (found2 != mapBusVSCConvertersBusId_.end() && found2->second == inputs::NetworkManager::NbOfRegulating::MULTIPLES)) {
      // always put both converters even if only one has multiple VSC
      ret.push_back(std::make_pair(hvdcLine.converter1->busId, hvdcLine.converter1->converterId));
      ret.push_back(std::make_pair(hvdcLine.converter2->busId, hvdcLine.converter2->converterId));
    }
    break;
  }
  default:  // impossible case by definition of the enum
    break;
  }

  return ret;
}

auto
HVDCDefinitionAlgorithm::computeModelVSC(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, HVDCDefinition::HVDCModel plusVSCInfiniteReactive,
                                         HVDCDefinition::HVDCModel plusVSCFiniteReactive, HVDCDefinition::HVDCModel oneVSCInfiniteReactive,
                                         HVDCDefinition::HVDCModel oneVSCFiniteReactive) const -> HVDCModelDefinition {
  auto vscBusIdsWithMultipleRegulation = getBusRegulatedByMultipleVSC(hvdcline, position);
  if (vscBusIdsWithMultipleRegulation.size() > 0) {
    return HVDCModelDefinition{infiniteReactiveLimits_ ? plusVSCInfiniteReactive : plusVSCFiniteReactive, vscBusIdsWithMultipleRegulation};
  } else {
    return HVDCModelDefinition{infiniteReactiveLimits_ ? oneVSCInfiniteReactive : oneVSCFiniteReactive, vscBusIdsWithMultipleRegulation};
  }
}

auto
HVDCDefinitionAlgorithm::computeModel(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, inputs::HvdcLine::ConverterType type) const
    -> HVDCModelDefinition {
  if (position == HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
    if (type == inputs::HvdcLine::ConverterType::LCC) {
      return HVDCModelDefinition{infiniteReactiveLimits_ ? HVDCDefinition::HVDCModel::HvdcPTanPhi : HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ, {}};
    } else {
      const bool hvdcAngleDroopActivePowerControlIsEnabled = hvdcline.activePowerControl.has_value();
      if (!hvdcAngleDroopActivePowerControlIsEnabled) {
        return computeModelVSC(hvdcline, position, HVDCDefinition::HVDCModel::HvdcPQProp, HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ,
                               HVDCDefinition::HVDCModel::HvdcPV, HVDCDefinition::HVDCModel::HvdcPVDiagramPQ);
      } else {
        return computeModelVSC(hvdcline, position, HVDCDefinition::HVDCModel::HvdcPQPropEmulation, HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulation,
                               HVDCDefinition::HVDCModel::HvdcPVEmulation, HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulation);
      }
    }
  } else {
    // case only one of them is in the main connex
    if (type == inputs::HvdcLine::ConverterType::LCC) {
      return HVDCModelDefinition{
          infiniteReactiveLimits_ ? HVDCDefinition::HVDCModel::HvdcPTanPhiDangling : HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ, {}};
    } else {
      return computeModelVSC(hvdcline, position, HVDCDefinition::HVDCModel::HvdcPQPropDangling, HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ,
                             HVDCDefinition::HVDCModel::HvdcPVDangling, HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ);
    }
  }
}

std::pair<std::reference_wrapper<HVDCDefinition>, bool>
HVDCDefinitionAlgorithm::getOrCreateHvdcLineDefinition(const inputs::HvdcLine& hvdcLine) {
  auto& hvdcLines = hvdcLinesDefinitions_.hvdcLines;
  auto it = hvdcLines.find(hvdcLine.id);
  bool alreadyInserted = it != hvdcLines.end();
  if (alreadyInserted) {
    return {std::ref(it->second), alreadyInserted};
  } else {
    std::array<double, 2> powerFactors{0., 0.};
    boost::optional<VSCDefinition> def1;
    boost::optional<VSCDefinition> def2;
    if (hvdcLine.converterType == inputs::HvdcLine::ConverterType::LCC) {
      auto converterLCC1 = std::dynamic_pointer_cast<inputs::LCCConverter>(hvdcLine.converter1);
      auto converterLCC2 = std::dynamic_pointer_cast<inputs::LCCConverter>(hvdcLine.converter2);
      powerFactors.at(0) = converterLCC1->powerFactor;
      powerFactors.at(1) = converterLCC2->powerFactor;
    } else {
      auto converterVSC1 = std::dynamic_pointer_cast<inputs::VSCConverter>(hvdcLine.converter1);
      auto converterVSC2 = std::dynamic_pointer_cast<inputs::VSCConverter>(hvdcLine.converter2);
      def1 = VSCDefinition(converterVSC1->converterId, converterVSC1->qMax, converterVSC1->qMin, hvdcLine.pMax, converterVSC1->points);
      def2 = VSCDefinition(converterVSC2->converterId, converterVSC2->qMax, converterVSC2->qMin, hvdcLine.pMax, converterVSC2->points);
    }

    boost::optional<double> droop = (hvdcLine.activePowerControl) ? hvdcLine.activePowerControl->droop : boost::optional<double>();
    HVDCDefinition createdHvdcLine(hvdcLine.id, hvdcLine.converterType, hvdcLine.converter1->converterId, hvdcLine.converter1->busId,
                                   hvdcLine.converter1->isVoltageRegulationOn(), hvdcLine.converter2->converterId, hvdcLine.converter2->busId,
                                   hvdcLine.converter2->isVoltageRegulationOn(), HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT,
                                   HVDCDefinition::HVDCModel::HvdcPTanPhi, powerFactors, hvdcLine.pMax, def1, def2, droop);
    auto pair = hvdcLines.emplace(hvdcLine.id, createdHvdcLine);
    return {std::ref(pair.first->second), alreadyInserted};
  }
}

void
HVDCDefinitionAlgorithm::operator()(const NodePtr& node) {
  for (const auto& converterPtr : node->converters) {
    auto converter = converterPtr.lock();
    const auto& hvdcLine = converter->hvdcLine;
    auto hvdcLineDefPair = getOrCreateHvdcLineDefinition(*hvdcLine);
    auto& hvdcLineDefinition = hvdcLineDefPair.first.get();

    if (hvdcLineDefPair.second) {
      // If we meet twice the same HVDC line with two different converters in nodes, it means that both extremities are
      // in the component: it is assumed that the algorithm is performed on main connex component
      hvdcLineDefinition.position = HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT;
    } else if (converter->converterId == hvdcLine->converter1->converterId) {
      hvdcLineDefinition.position = HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT;
    } else if (converter->converterId == hvdcLine->converter2->converterId) {
      hvdcLineDefinition.position = HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT;
    } else {
      LOG(error) << MESS(HvdcLineBadInitialization, hvdcLine->id) << LOG_ENDL;
      continue;
    }

    auto modelDef = computeModel(*hvdcLine, hvdcLineDefinition.position, hvdcLineDefinition.converterType);
    hvdcLineDefinition.model = modelDef.model;

    // If VSC bus definitions map is defined, it means that the converter is a VSC and we can cast it. If not defined, the dynamic cast
    // returns a null pointer and the transform has no effect since th eap is empty
    auto vscConverter = std::dynamic_pointer_cast<inputs::VSCConverter>(converter);
    std::transform(modelDef.vscBusIdsMultipleRegulated.begin(), modelDef.vscBusIdsMultipleRegulated.end(),
                   std::inserter(hvdcLinesDefinitions_.vscBusVSCDefinitionsMap, hvdcLinesDefinitions_.vscBusVSCDefinitionsMap.end()),
                   [&vscConverter, &hvdcLine](const HVDCModelDefinition::VSCBusPair& pair) {
                     return std::make_pair(pair.first,
                                           VSCDefinition(pair.second, vscConverter->qMax, vscConverter->qMin, hvdcLine->pMax, vscConverter->points));
                   });
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
