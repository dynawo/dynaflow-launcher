//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  ContingenciesManager.cpp
 *
 * @brief ContingenciesManager implementation file
 *
 */

#include "ContingenciesManager.h"

#include "Log.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace dfl {
namespace inputs {

ContingenciesManager::ContingenciesManager(const boost::filesystem::path& filepath) {
  if (!filepath.empty()) {
    load(filepath);
  }
}

void
ContingenciesManager::load(const boost::filesystem::path& filepath) {
  try {
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(filepath.generic_string(), tree);

    /**
     * The JSON format for contingencies is inherited from Powsybl:
     *
     * {
     *   "version" : "1.0",
     *   "name" : "list",
     *   "contingencies" : [ {
     *     "id" : "contingency1",
     *     "elements" : [ {
     *       "id" : "element11",
     *       "type" : "BRANCH"
     *     } ]
     *   }, {
     *     "id" : "contingency2",
     *     "elements" : [ {
     *        "id" : "element21",
     *        "type" : "GENERATOR"
     *     }, {
     *        "id" : "element22"
     *        "type" : "LOAD"
     *     } ]
     *   }
     * }
     */

    LOG(info, ContingenciesReadingFrom, filepath.generic_string());
    contingencies_.reserve(tree.get_child("contingencies").size());
    for (const auto& contingencyPtree : tree.get_child("contingencies")) {
      const auto& contingencyId = contingencyPtree.second.get<std::string>("id");

      Contingency contingency(contingencyId);
      bool valid = true;
      for (const auto& elementPtree : contingencyPtree.second.get_child("elements")) {
        const auto& elementId = elementPtree.second.get<std::string>("id");
        const auto& elementTypeStr = elementPtree.second.get<std::string>("type");

        const auto elementType = ContingencyElement::typeFromString(elementTypeStr);
        if (elementType) {
          // We follow the same strategy applied in Dynawo: converting a 3-winding transformer to 3 2-winding transformers.
          // The 3 2-winding transformers representing each leg will always be connected at least to the fictitious star bus.
          // If any of the legs is connected to the main connected component, then the star bus will be also in the main cc.
          // Even if one leg is disconnected, the contingency as a whole will be considered,
          // because all 3 2-winding transformers will be connected at least at the start bus.
          if (*elementType == ContingencyElement::Type::THREE_WINDINGS_TRANSFORMER) {
            contingency.elements.emplace_back(elementId + "_1", ContingencyElement::Type::TWO_WINDINGS_TRANSFORMER);
            contingency.elements.emplace_back(elementId + "_2", ContingencyElement::Type::TWO_WINDINGS_TRANSFORMER);
            contingency.elements.emplace_back(elementId + "_3", ContingencyElement::Type::TWO_WINDINGS_TRANSFORMER);
            LOG(debug, Contingency2WIsFrom3W, elementId);
            LOG(debug, Contingency2WIsFrom3WLeg, elementId + "_1", "1");
            LOG(debug, Contingency2WIsFrom3WLeg, elementId + "_2", "2");
            LOG(debug, Contingency2WIsFrom3WLeg, elementId + "_3", "3");
          } else {
            contingency.elements.emplace_back(elementId, *elementType);
          }
        } else {
          valid = false;
          LOG(warn, ContingencyInvalidBadElemType, contingency.id, elementId, elementTypeStr);
        }
      }
      if (valid) {
        contingencies_.push_back(contingency);
      }
    }
  } catch (std::exception& e) {
    throw Error(ContingenciesReadError, filepath.generic_string(), e.what());
  }
}

}  // namespace inputs
}  // namespace dfl
