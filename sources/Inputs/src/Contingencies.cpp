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
 * @file  Contingencies.cpp
 *
 * @brief Contingencies implementation file
 *
 */

#include "Contingencies.h"

#include "Log.h"
#include "Message.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

namespace dfl {
namespace inputs {

Contingencies::Contingencies(const boost::filesystem::path& filepath) {
  loadDefinitions(filepath);
  init();
}

void Contingencies::loadDefinitions(const boost::filesystem::path& filepath)
{
  try {
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(filepath.native(), tree);

    /**
     * The JSON format for contingency definitions is inherited from Powsybl:
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

    LOG(debug) << MESS(ContingenciesReadingFrom, filepath) << LOG_ENDL;
    for (const boost::property_tree::ptree::value_type& v : tree.get_child("contingencies")) {
      const boost::property_tree::ptree& ptcontingency = v.second;
      LOG(debug) << ptcontingency.get<std::string>("id") << LOG_ENDL;

      ContingencyDefinition contingency(ptcontingency.get<std::string>("id"));
      bool valid = true;
      for (const boost::property_tree::ptree::value_type& pte : ptcontingency.get_child("elements")) {
        const auto element_id = pte.second.get<std::string>("id");
        const auto element_type = pte.second.get<std::string>("type");
        LOG(debug) << "  " << element_id << " (" << element_type << ")" << LOG_ENDL;

        if (const auto has_type = elementTypeFromString(element_type)) {
          ContingencyElementDefinition element(element_id);
          element.type = *has_type;
          contingency.elements.push_back(element);
        } else {
          valid = false;
          LOG(warn) << MESS(ContingencyInvalidBadElemType, contingency.id, element_id, element_type) << LOG_ENDL;
        }
      }
      if (valid) {
        contingencies_.push_back(std::make_shared<ContingencyDefinition>(contingency));
      }
    }
  } catch (std::exception& e) {
    LOG(error) << MESS(ContingenciesReadError, filepath, e.what()) << LOG_ENDL;
    std::exit(EXIT_FAILURE);
  }
}

void
Contingencies::init() {
  for (auto c : contingencies_) {
    for (auto e : c->elements) {
      if (elementContingencies_.find(e.id) == elementContingencies_.end()) {
        elementContingencies_.insert({e.id, {c}});
      } else {
        elementContingencies_[e.id].push_back(c);
      }
    }
  }
}

boost::optional<Contingencies::ElementType>
Contingencies::elementTypeFromString(const std::string& str) {
  if (str == "LOAD") {
    return ElementType::LOAD;
  } else if (str == "GENERATOR") {
    return ElementType::GENERATOR;
  } else if (str == "BRANCH") {
    return ElementType::BRANCH;
  } else if (str == "LINE") {
    return ElementType::LINE;
  } else if (str == "TWO_WINDINGS_TRANSFORMER") {
    return ElementType::TWO_WINDINGS_TRANSFORMER;
  } else if (str == "THREE_WINDINGS_TRANSFORMER") {
    return ElementType::THREE_WINDINGS_TRANSFORMER;
  } else if (str == "SHUNT_COMPENSATOR") {
    return ElementType::SHUNT_COMPENSATOR;
  } else if (str == "STATIC_VAR_COMPENSATOR") {
    return ElementType::STATIC_VAR_COMPENSATOR;
  } else if (str == "DANGLING_LINE") {
    return ElementType::DANGLING_LINE;
  } else if (str == "HVDC_LINE") {
    return ElementType::HVDC_LINE;
  } else if (str == "BUSBAR_SECTION") {
    return ElementType::BUSBAR_SECTION;
  }
  return boost::none;
}

std::string
Contingencies::toString(ElementType type) {
  switch (type) {
  case ElementType::LOAD:
    return "LOAD";
  case ElementType::GENERATOR:
    return "GENERATOR";
  case ElementType::BRANCH:
    return "BRANCH";
  case ElementType::LINE:
    return "LINE";
  case ElementType::TWO_WINDINGS_TRANSFORMER:
    return "TWO_WINDINGS_TRANSFORMER";
  case ElementType::THREE_WINDINGS_TRANSFORMER:
    return "THREE_WINDINGS_TRANSFORMER";
  case ElementType::SHUNT_COMPENSATOR:
    return "SHUNT_COMPENSATOR";
  case ElementType::STATIC_VAR_COMPENSATOR:
    return "STATIC_VAR_COMPENSATOR";
  case ElementType::DANGLING_LINE:
    return "DANGLING_LINE";
  case ElementType::HVDC_LINE:
    return "HVDC_LINE";
  case ElementType::BUSBAR_SECTION:
    return "BUSBAR_SECTION";
  }
  return "UNKNOWN_TYPE";
}

std::string
Contingencies::toString(ContingencyElementDefinition::ValidationStatus status)
 {
  using Status = ContingencyElementDefinition::ValidationStatus;
  switch (status) {
    case Status::MAIN_CC_VALID_TYPE:
    return "MAIN_CC_VALID_TYPE";
    case Status::MAIN_CC_INVALID_TYPE:
    return "MAIN_CC_INVALID_TYPE";
    case Status::NOT_IN_NETWORK_OR_NOT_IN_MAIN_CC:
    return "NOT_IN_NETWORK_OR_NOT_IN_MAIN_CC";
  }
  return "STATUS_UNKNOWN";
}

void
Contingencies::markElementValid(const std::string id, ElementType type) {
  using Status = ContingencyElementDefinition::ValidationStatus;
  auto ec = elementContingencies_.find(id);
  if (ec != elementContingencies_.end()) {
    // For all contingencies where the element is referred ...
    for (auto& c : ec->second) {
      for (auto& e : c->elements) {
        // Find the contingency definition for the element inside the contingency ...
        if (e.id == id) {
          // And check it has been given with a valid type,
          // according to the reference type found in the Network
          e.status = isValidType(e.type, type) ? Status::MAIN_CC_VALID_TYPE : Status::MAIN_CC_INVALID_TYPE;
        }
      }
    }
  }
}

bool
Contingencies::isValidType(ElementType type, ElementType referenceType) {
  if (type == ElementType::BRANCH) {
    return referenceType == ElementType::LINE || referenceType == ElementType::TWO_WINDINGS_TRANSFORMER;
  } else if (referenceType == ElementType::BRANCH) {
    return type == ElementType::LINE || type == ElementType::TWO_WINDINGS_TRANSFORMER;
  }
  return type == referenceType;
}

bool
Contingencies::isValidForSimulation(const ContingencyDefinition& c) {
  // A contigency is valid for simulation if all its elements are in the main connected component and have valid types
  for (auto e : c.elements) {
    if (e.status != ContingencyElementDefinition::ValidationStatus::MAIN_CC_VALID_TYPE) {
      LOG(warn) << MESS(ContingencyInvalidForSimulation, c.id, e.id, toString(e.status));
      return false;
    }
  }
  return true;
}

}  // namespace inputs
}  // namespace dfl
