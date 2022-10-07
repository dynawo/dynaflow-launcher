//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "GeneratorDefinitionAlgorithm.h"

#include "Log.h"

#include <DYNCommon.h>

namespace dfl {
namespace algo {

GeneratorDefinitionAlgorithm::GeneratorDefinitionAlgorithm(Generators& gens, BusGenMap& busesWithDynamicModel,
                                                           const inputs::NetworkManager::BusMapRegulating& busMap, bool infinitereactivelimits,
                                                           double tfoVoltageLevel) :
    generators_(gens),
    busesWithDynamicModel_(busesWithDynamicModel),
    busMap_(busMap),
    useInfiniteReactivelimits_{infinitereactivelimits},
    tfoVoltageLevel_(tfoVoltageLevel) {}

void
GeneratorDefinitionAlgorithm::operator()(const NodePtr& node, std::shared_ptr<AlgorithmsResults>& algoRes) {
  auto& node_generators = node->generators;

  for (const auto& generator : node_generators) {
    ModelType model = ModelType::NETWORK;
    if (isTargetPValid(generator) && generator.isVoltageRegulationOn && isDiagramValid(generator)) {
      dfl::inputs::NetworkManager::BusMapRegulating::const_iterator it = busMap_.find(generator.regulatedBusId);
      assert(it != busMap_.end());
      dfl::inputs::NetworkManager::NbOfRegulating nbOfRegulatingGenerators = it->second;
      model = ModelType::SIGNALN_INFINITE;
      algoRes->isAtLeastOneGeneratorRegulating = true;
      if (generator.targetV > tfoVoltageLevel_) {
        if (generator.regulatedBusId != generator.connectedBusId) {
          throw Error(UnsuportedGeneratorRemoteRegulationWithTfo, generator.id);
        }
        // Generator is assumed to have its transfo in the static model
        // Several generators regulate this node
        if (useInfiniteReactivelimits_) {
          model = ModelType::SIGNALN_TFO_INFINITE;
        } else {
          model = isDiagramRectangular(generator) ? ModelType::SIGNALN_TFO_RECTANGULAR : ModelType::DIAGRAM_PQ_TFO_SIGNALN;
        }
        busesWithDynamicModel_.insert({generator.regulatedBusId, generator.id});
      } else {
        // Generator is assumed to have its transfo in the static model
        if (node_generators.size() == 1 && IsOtherGeneratorConnectedBySwitches(node)) {
          // Several generators regulate this node
          if (useInfiniteReactivelimits_) {
            model = ModelType::PROP_SIGNALN_INFINITE;
          } else {
            model = isDiagramRectangular(generator) ? ModelType::PROP_SIGNALN_RECTANGULAR : ModelType::PROP_DIAGRAM_PQ_SIGNALN;
          }
          busesWithDynamicModel_.insert({generator.regulatedBusId, generator.id});
        } else {
          switch (nbOfRegulatingGenerators) {
          case dfl::inputs::NetworkManager::NbOfRegulating::ONE:
            // Only one generator regulates this node
            if (generator.regulatedBusId == generator.connectedBusId) {
              // local regulation
              if (useInfiniteReactivelimits_) {
                model = ModelType::SIGNALN_INFINITE;
              } else {
                model = isDiagramRectangular(generator) ? ModelType::SIGNALN_RECTANGULAR : ModelType::DIAGRAM_PQ_SIGNALN;
              }
            } else {
              // remote regulation
              if (useInfiniteReactivelimits_) {
                model = ModelType::REMOTE_SIGNALN_INFINITE;
              } else {
                model = isDiagramRectangular(generator) ? ModelType::REMOTE_SIGNALN_RECTANGULAR : ModelType::REMOTE_DIAGRAM_PQ_SIGNALN;
              }
            }
            break;
          case dfl::inputs::NetworkManager::NbOfRegulating::MULTIPLES:
            // Several generators regulate this node
            if (useInfiniteReactivelimits_) {
              model = ModelType::PROP_SIGNALN_INFINITE;
            } else {
              model = isDiagramRectangular(generator) ? ModelType::PROP_SIGNALN_RECTANGULAR : ModelType::PROP_DIAGRAM_PQ_SIGNALN;
            }
            busesWithDynamicModel_.insert({generator.regulatedBusId, generator.id});
            break;
          default:  //  impossible by definition of the enum
            break;
          }
        }
      }
    }
    generators_.emplace_back(generator.id, model, node->id, generator.points, generator.qmin, generator.qmax, generator.pmin, generator.pmax, generator.targetP,
                             generator.regulatedBusId);
  }
}

bool
GeneratorDefinitionAlgorithm::isDiagramValid(const inputs::Generator& generator) {
  if (useInfiniteReactivelimits_) {
    return true;
  }
  // If there are no points, the diagram will be constructed from the pmin, pmax, qmin and qmax values.
  // We check the validity of pmin,pmax and qmin,qmax values
  if (generator.points.empty()) {
    if (DYN::doubleEquals(generator.pmin, generator.pmax)) {
      LOG(warn, InvalidDiagramAllPEqual, generator.id);
      return false;
    }
    if (DYN::doubleEquals(generator.qmin, generator.qmax)) {
      LOG(warn, InvalidDiagramQminsEqualQmaxs, generator.id);
      return false;
    }
    return true;
  }

  // If there is only one point, the diagram is not valid
  if (generator.points.size() == 1) {
    LOG(warn, InvalidDiagramOnePoint, generator.id);
    return false;
  }

  auto firstP = generator.points.front().p;
  bool allQminEqualQmax = true;
  bool allPEqual = true;
  auto it = generator.points.begin();
  while ((allQminEqualQmax || allPEqual) && it != generator.points.end()) {
    allQminEqualQmax = allQminEqualQmax && DYN::doubleEquals(it->qmin, it->qmax);
    allPEqual = allPEqual && DYN::doubleEquals(it->p, firstP);
    ++it;
  }
  bool valid = !allQminEqualQmax && !allPEqual;

  if (!valid) {
    if (allQminEqualQmax && allPEqual) {
      LOG(warn, InvalidDiagramBothError, generator.id);
    } else if (allQminEqualQmax) {
      LOG(warn, InvalidDiagramQminsEqualQmaxs, generator.id);
    } else if (allPEqual) {
      LOG(warn, InvalidDiagramAllPEqual, generator.id);
    }
  }
  return valid;
}

bool
GeneratorDefinitionAlgorithm::IsOtherGeneratorConnectedBySwitches(const NodePtr& node) const {
  auto& buses = node->getBusesConnectedByVoltageLevel();

  if (buses.size() == 0) {
    return false;
  }
  auto vl = node->voltageLevel.lock();

  for (const auto& id : buses) {
    auto found = std::find_if(vl->nodes.begin(), vl->nodes.end(), [&id](const NodePtr& nodeLocal) { return nodeLocal->id == id; });
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

bool
GeneratorDefinitionAlgorithm::isTargetPValid(const inputs::Generator& generator) const {
  return (DYN::doubleEquals(-generator.targetP, generator.pmin) || -generator.targetP > generator.pmin) &&
         (DYN::doubleEquals(-generator.targetP, generator.pmax) || -generator.targetP < generator.pmax);
}

bool
notEqual(const inputs::Generator::ReactiveCurvePoint& firstPoint, const inputs::Generator::ReactiveCurvePoint& secondPoint) {
  return DYN::doubleNotEquals(firstPoint.qmax, secondPoint.qmax) || DYN::doubleNotEquals(firstPoint.qmin, secondPoint.qmin);
}

bool
GeneratorDefinitionAlgorithm::isDiagramRectangular(const inputs::Generator& generator) const {
  return (std::adjacent_find(generator.points.begin(), generator.points.end(), notEqual) == generator.points.end());
}

}  // namespace algo
}  // namespace dfl
