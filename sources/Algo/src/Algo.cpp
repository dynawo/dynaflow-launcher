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

GeneratorDefinitionAlgorithm::GeneratorDefinitionAlgorithm(Generators& gens, bool infinitereactivelimits) :
    NodeAlgorithm(),
    generators_(gens),
    useInfiniteReactivelimits_{infinitereactivelimits} {}

void
GeneratorDefinitionAlgorithm::operator()(const NodePtr& node) {
  auto& node_generators = node->generators;
  if (node_generators.size() == 1) {
    auto model = useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::SIGNALN : GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN;
    const auto& gen = node_generators.front();

    if (model == GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN && !isDiagramValid(gen)) {
      return;
    }

    generators_.emplace_back(gen.id, model, node->id, gen.points, gen.qmin, gen.qmax, gen.pmin, gen.pmax);
  } else {
    auto model =
        useInfiniteReactivelimits_ ? GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN : GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN;
    for (auto it = node_generators.begin(); it != node_generators.end(); ++it) {
      if (model == GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN && !isDiagramValid(*it)) {
        continue;
      }
      generators_.emplace_back(it->id, model, node->id, it->points, it->qmin, it->qmax, it->pmin, it->pmax);
    }
  }
}

bool
GeneratorDefinitionAlgorithm::isDiagramValid(const inputs::Generator& generator) {
  //If there are no points, the diagram will be constructed from the pmin, pmax, qmin and qmax values.
  //We check the validity of pmin,pmax and qmin,qmax values
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

  //If there is only one point, the diagram is not valid
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

ControllerInterfaceDefinitionAlgorithm::ControllerInterfaceDefinitionAlgorithm(HvdcLineSet& hvdcLines) : hvdcLines_(hvdcLines) {}

void
ControllerInterfaceDefinitionAlgorithm::operator()(const NodePtr& node) {
  for (const auto& converter : node->converterInterfaces) {
    const auto& hvdcLine = converter.hvdcLine;
    HvdcLineDefinition createdHvdcLine = HvdcLineDefinition(hvdcLine->id, hvdcLine->converterType, hvdcLine->converter1_id, hvdcLine->converter1_busId,
                                                            hvdcLine->converter1_voltageRegulationOn, hvdcLine->converter2_id, hvdcLine->converter2_busId,
                                                            hvdcLine->converter2_voltageRegulationOn, HvdcLineDefinition::Position::BOTH_IN_MAIN_COMPONENT);
    auto it = hvdcLines_.find(createdHvdcLine);
    bool alreadyInserted = it != hvdcLines_.end();
    if (alreadyInserted) {
      it->position = HvdcLineDefinition::Position::BOTH_IN_MAIN_COMPONENT;
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

    hvdcLines_.emplace(createdHvdcLine);
  }
}
/////////////////////////////////////////////////////////////////

bool
HvdcLineDefinition::operator==(const HvdcLineDefinition& other) const {
  return id == other.id && converterType == other.converterType && converter1_id == other.converter1_id && converter1_busId == other.converter1_busId &&
         converter2_voltageRegulationOn == other.converter2_voltageRegulationOn && converter2_id == other.converter2_id &&
         converter2_busId == other.converter2_busId && converter2_voltageRegulationOn == other.converter2_voltageRegulationOn && position == other.position;
}
}  // namespace algo
}  // namespace dfl
