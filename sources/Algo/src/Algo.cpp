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
  for (const auto& generator : node_generators) {
    auto it = busMap_.find(generator.regulatedBusId);
    assert(it != busMap_.end());
    auto nbOfRegulatingGenerators = it->second;
    GeneratorDefinition::ModelType model = GeneratorDefinition::ModelType::SIGNALN;
    if (node_generators.size() == 1 && IsOtherGeneratorConnectedBySwitches(node)) {
      model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::REMOTE_SIGNALN : GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN;
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
        busesWithDynamicModel_.insert({generator.regulatedBusId, generator.id});
        break;
      default:  //  impossible by definition of the enum
        break;
      }
    }
    if ((model == GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN || model == GeneratorDefinition::ModelType::REMOTE_DIAGRAM_PQ_SIGNALN ||
         model == GeneratorDefinition::ModelType::PROP_DIAGRAM_PQ_SIGNALN || model == GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN) &&
        !isDiagramValid(generator)) {
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
}  // namespace algo
}  // namespace dfl
