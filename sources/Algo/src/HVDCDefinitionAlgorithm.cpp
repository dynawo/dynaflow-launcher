//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "HVDCDefinitionAlgorithm.h"
#include "Log.h"
#include "Message.hpp"
#include <DYNCommon.h>

namespace dfl {
namespace algo {

std::size_t
HVDCDefinitionAlgorithm::HVDCModelDefinition::VSCBusPairHash::operator()(const HVDCModelDefinition::VSCBusPair& pair) const noexcept {
  std::size_t seed = 0;
  boost::hash_combine(seed, pair.first);
  boost::hash_combine(seed, pair.second);
  return seed;
}

HVDCDefinitionAlgorithm::HVDCDefinitionAlgorithm(HVDCLineDefinitions& hvdcLinesDefinitions, bool infiniteReactiveLimits,
                                                 const std::unordered_set<std::shared_ptr<inputs::Converter>>& converters,
                                                 const inputs::NetworkManager::BusMapRegulating& mapBusVSCConvertersBusId,
                                                 const boost::shared_ptr<DYN::ServiceManagerInterface>& serviceManager) :
    hvdcLinesDefinitions_(hvdcLinesDefinitions),
    infiniteReactiveLimits_(infiniteReactiveLimits),
    mapBusVSCConvertersBusId_(mapBusVSCConvertersBusId),
    serviceManager_(serviceManager) {
  std::transform(converters.begin(), converters.end(), std::inserter(vscConverters_, vscConverters_.begin()),
                 [](const std::shared_ptr<inputs::Converter>& converter) { return std::make_pair(converter->busId, converter); });
}

auto
HVDCDefinitionAlgorithm::getBusRegulatedByMultipleVSC(const inputs::HvdcLine& hvdcLine, HVDCDefinition::Position position) const
    -> HVDCModelDefinition::VSCBusPairSet {
  auto set = getBusesByPosition(hvdcLine, position);
  assert(set.size() <= 2);  // by definition of getBusesByPosition
  if (set.empty()) {
    return set;
  }

  auto it = set.begin();
  auto found = mapBusVSCConvertersBusId_.find(set.begin()->first);
  if (found != mapBusVSCConvertersBusId_.end() && found->second == inputs::NetworkManager::NbOfRegulating::MULTIPLES) {
    // always put both converters of the hvdc link even if only one of them is connected to a bus regulated by several VSC
    return set;
  }

  if (set.size() == 2) {
    ++it;
    auto found2 = mapBusVSCConvertersBusId_.find(it->first);
    if (found2 != mapBusVSCConvertersBusId_.end() && found2->second == inputs::NetworkManager::NbOfRegulating::MULTIPLES) {
      // always put both converters of the hvdc link even if only one of them is connected to a bus regulated by several VSC
      return set;
    }
  }

  return {};
}

auto
HVDCDefinitionAlgorithm::getBusesByPosition(const inputs::HvdcLine& hvdcLine, HVDCDefinition::Position position) const -> HVDCModelDefinition::VSCBusPairSet {
  HVDCModelDefinition::VSCBusPairSet ret;
  switch (position) {
  case HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT: {
    ret.insert(std::make_pair(hvdcLine.converter1->busId, hvdcLine.converter1->converterId));
    break;
  }
  case HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT: {
    ret.insert(std::make_pair(hvdcLine.converter2->busId, hvdcLine.converter2->converterId));
    break;
  }
  case HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT: {
    ret.insert(std::make_pair(hvdcLine.converter1->busId, hvdcLine.converter1->converterId));
    ret.insert(std::make_pair(hvdcLine.converter2->busId, hvdcLine.converter2->converterId));
    break;
  }
  default:  // impossible case by definition of the enum
    break;
  }

  return ret;
}

auto
HVDCDefinitionAlgorithm::getVSCConnectedBySwitches(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, const NodePtr& node) const
    -> HVDCModelDefinition::VSCBusPairSet {
  auto vl = node->voltageLevel.lock();
  auto buses = serviceManager_->getBusesConnectedBySwitch(node->id, vl->id);
  HVDCModelDefinition::VSCBusPairSet ret;
  for (const auto& id : buses) {
    auto found = std::find_if(vl->nodes.begin(), vl->nodes.end(), [&id](const NodePtr& nodeLocal) { return nodeLocal->id == id; });
    assert(found != vl->nodes.end());
    for (const auto& converter_ptr : (*found)->converters) {
      auto converter = converter_ptr.lock();
      if (converter->hvdcLine->converterType != inputs::HvdcLine::ConverterType::VSC) {
        continue;
      }
      ret.insert(std::make_pair(converter->busId, converter->converterId));
    }
  }

  if (!ret.empty()) {
    // Add itself to ensure all converters are taken into account
    auto set = getBusesByPosition(hvdcline, position);
    ret.insert(set.begin(), set.end());
  }

  return ret;
}

auto
HVDCDefinitionAlgorithm::computeModelVSC(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position,
                                         HVDCDefinition::HVDCModel multipleVSCInfiniteReactive, HVDCDefinition::HVDCModel multipleVSCFiniteReactive,
                                         HVDCDefinition::HVDCModel oneVSCInfiniteReactive, HVDCDefinition::HVDCModel oneVSCFiniteReactive,
                                         const NodePtr& node) const -> HVDCModelDefinition {
  auto vscBusIdsWithMultipleRegulation = getBusRegulatedByMultipleVSC(hvdcline, position);
  auto vscBusIdsConnectedBySwitches = getVSCConnectedBySwitches(hvdcline, position, node);
  vscBusIdsWithMultipleRegulation.insert(vscBusIdsConnectedBySwitches.begin(), vscBusIdsConnectedBySwitches.end());
  if (vscBusIdsWithMultipleRegulation.size() > 0) {
    return HVDCModelDefinition{infiniteReactiveLimits_ ? multipleVSCInfiniteReactive : multipleVSCFiniteReactive, vscBusIdsWithMultipleRegulation};
  } else {
    return HVDCModelDefinition{infiniteReactiveLimits_ ? oneVSCInfiniteReactive : oneVSCFiniteReactive, vscBusIdsWithMultipleRegulation};
  }
}

auto
HVDCDefinitionAlgorithm::computeModel(const inputs::HvdcLine& hvdcline, HVDCDefinition::Position position, inputs::HvdcLine::ConverterType type,
                                      const NodePtr& node) const -> HVDCModelDefinition {
  if (position == HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
    if (type == inputs::HvdcLine::ConverterType::LCC) {
      return HVDCModelDefinition{infiniteReactiveLimits_ ? HVDCDefinition::HVDCModel::HvdcPTanPhi : HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ, {}};
    } else {
      const bool hvdcAngleDroopActivePowerControlIsEnabled = hvdcline.activePowerControl.has_value();
      if (!hvdcAngleDroopActivePowerControlIsEnabled) {
        return computeModelVSC(hvdcline, position, HVDCDefinition::HVDCModel::HvdcPQProp, HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ,
                               HVDCDefinition::HVDCModel::HvdcPV, HVDCDefinition::HVDCModel::HvdcPVDiagramPQ, node);
      } else {
        return computeModelVSC(hvdcline, position, HVDCDefinition::HVDCModel::HvdcPQPropEmulation, HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulation,
                               HVDCDefinition::HVDCModel::HvdcPVEmulation, HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulation, node);
      }
    }
  } else {
    // case only one of them is in the main connex component
    if (type == inputs::HvdcLine::ConverterType::LCC) {
      return HVDCModelDefinition{
          infiniteReactiveLimits_ ? HVDCDefinition::HVDCModel::HvdcPTanPhiDangling : HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ, {}};
    } else {
      return computeModelVSC(hvdcline, position, HVDCDefinition::HVDCModel::HvdcPQPropDangling, HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ,
                             HVDCDefinition::HVDCModel::HvdcPVDangling, HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ, node);
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
    boost::optional<bool> voltageRegulation1;
    boost::optional<bool> voltageRegulation2;
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
      voltageRegulation1 = converterVSC1->voltageRegulationOn;
      voltageRegulation2 = converterVSC2->voltageRegulationOn;
    }

    boost::optional<double> droop = (hvdcLine.activePowerControl) ? hvdcLine.activePowerControl->droop : boost::optional<double>();
    HVDCDefinition createdHvdcLine(hvdcLine.id, hvdcLine.converterType, hvdcLine.converter1->converterId, hvdcLine.converter1->busId, voltageRegulation1,
                                   hvdcLine.converter2->converterId, hvdcLine.converter2->busId, voltageRegulation2,
                                   HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT, HVDCDefinition::HVDCModel::HvdcPTanPhi, powerFactors, hvdcLine.pMax, def1,
                                   def2, droop);
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

    auto modelDef = computeModel(*hvdcLine, hvdcLineDefinition.position, hvdcLineDefinition.converterType, node);
    hvdcLineDefinition.model = modelDef.model;

    std::transform(modelDef.vscBusIdsMultipleRegulated.begin(), modelDef.vscBusIdsMultipleRegulated.end(),
                   std::inserter(hvdcLinesDefinitions_.vscBusVSCDefinitionsMap, hvdcLinesDefinitions_.vscBusVSCDefinitionsMap.end()),
                   [this](const HVDCModelDefinition::VSCBusPair& pair) {
                     // If VSC bus definitions map is defined, it means that the converter is a VSC and we can cast it. If not defined, the dynamic cast
                     // returns a null pointer and the transform has no effect since the bus map is empty
                     auto vscConverter = std::dynamic_pointer_cast<inputs::VSCConverter>(vscConverters_.at(pair.first));
                     return std::make_pair(
                         pair.first, VSCDefinition(pair.second, vscConverter->qMax, vscConverter->qMin, vscConverter->hvdcLine->pMax, vscConverter->points));
                   });
  }
}

}  // namespace algo
}  // namespace dfl
