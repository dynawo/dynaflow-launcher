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

Contingencies::Contingencies(const std::string& filepath) {
  try {
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(filepath, tree);

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

    LOG(debug) << "Reading contingencies from " << filepath << " ..." << LOG_ENDL;
    for (const boost::property_tree::ptree::value_type& v : tree.get_child("contingencies")) {
      const boost::property_tree::ptree& ptcontingency = v.second;
      LOG(debug) << ptcontingency.get<std::string>("id") << LOG_ENDL;
      ContingencyDefinition contingency(ptcontingency.get<std::string>("id"));
      std::vector<std::string> elements_unknown_types;
      for (const boost::property_tree::ptree::value_type& pte : ptcontingency.get_child("elements")) {
        const auto element_id = pte.second.get<std::string>("id");
        const auto element_type = pte.second.get<std::string>("type");

        LOG(debug) << "  " << element_id  << " (" << element_type << ")" << LOG_ENDL;

        if (const auto has_type = typeFromString(element_type)) {
          ContingencyElementDefinition element(element_id);
          element.type = *has_type;
          contingency.elements.push_back(element);
        }
        else {
          elements_unknown_types.push_back(element_id);
          LOG(warn) << MESS(ElementTypeUnknown, element_id, element_type) << LOG_ENDL;
        }

      }

      // If any element had an unknown type the contingency is invalid
      if (elements_unknown_types.empty()) {
        contingencies_.push_back(std::make_shared<ContingencyDefinition>(contingency));
      }
    }
  } catch (std::exception& e) {
    LOG(error) << MESS(ContingenciesReadError, filepath, e.what()) << LOG_ENDL;
    std::exit(EXIT_FAILURE);
  }
}

void
Contingencies::log() {
    LOG(info) << "Contingencies" << LOG_ENDL;
    for (auto c = contingencies_.begin(); c != contingencies_.end(); ++c) {
      LOG(info) << (*c)->id << LOG_ENDL;
      for (auto e = (*c)->elements.begin(); e != (*c)->elements.end(); ++e) {
        LOG(info) << "  " << e->id << " (" << toString(e->type) << ")" << LOG_ENDL;
      }
    }
}

boost::optional<Contingencies::Type>
Contingencies::typeFromString(const std::string& str) {
  if (str == "GENERATOR") {
    return Type::GENERATOR;
  } else if (str == "LINE") {
    return Type::LINE;
  } else if (str == "TWO_WINDINGS_TRANSFORMER") {
    return Type::TWO_WINDINGS_TRANSFORMER;
  } else if (str == "BRANCH") {
    return Type::BRANCH; // No problem
  } else if (str == "SHUNT_COMPENSATOR") {
    return Type::SHUNT_COMPENSATOR;
  } else if (str == "LOAD") {
    return Type::LOAD;
  } else if (str == "DANGLING_LINE") {
    return Type::DANGLING_LINE;
  } else if (str == "HVDC_LINE") {
    return Type::HVDC_LINE;
  }   else if (str == "STATIC_VAR_COMPENSATOR") {
    return Type::STATIC_VAR_COMPENSATOR;
  } else if (str == "BUSBAR_SECTION") {
    return Type::BUSBAR_SECTION;
  }

  return boost::none;
}

std::string
Contingencies::toString(ElementInvalidReason reason) {
  switch (reason) {
    case ElementInvalidReason::GENERATOR_NOT_FOUND:
      return MESS(GeneratorNotFound);

    case ElementInvalidReason::TWOWINDINGS_TRANFORMER_NOT_FOUND:
      return MESS(TwoWTransformerNotFound);

    case ElementInvalidReason::BRANCH_NOT_FOUND:
      return MESS(BranchNotFound);

    case ElementInvalidReason::LINE_NOT_FOUND:
      return MESS(LineNotFound);

    case ElementInvalidReason::SHUNT_COMPENSATOR_NOT_FOUND:
      return MESS(ShuntCompensatorNotFound);

    case ElementInvalidReason::LOAD_NOT_FOUND:
      return MESS(LoadNotFound);

    case ElementInvalidReason::DANGLING_LINE_NOT_FOUND:
      return MESS(DanglingLineNotFound);

    case ElementInvalidReason::HVDC_LINE_NOT_FOUND:
      return MESS(HvdcLineNotFound);

    case ElementInvalidReason::STATIC_VAR_COMPENSATOR_NOT_FOUND:
      return MESS(StaticVarCompensatorNotFound);

    case ElementInvalidReason::NOT_IN_MAIN_CONNECTED_COMPONENT:
      return MESS(NotInMainConnectedComponent);

    default:
      throw std::logic_error("Gotten an unexpected error (or a corrupted value)");
  }
}

std::string
Contingencies::toString(Type type) {
  switch(type){
    case Type::GENERATOR:
    return "GENERATOR";

    case Type::LINE:
    return "LINE";

    case Type::TWO_WINDINGS_TRANSFORMER:
    return "TWO_WINDINGS_TRANSFORMER";

    case Type::BRANCH:
    return "BRANCH"; // No problem

    case Type::SHUNT_COMPENSATOR:
    return "SHUNT_COMPENSATOR";

    case Type::LOAD:
    return "LOAD";

    case Type::DANGLING_LINE:
    return "DANGLING_LINE";

    case Type::HVDC_LINE:
    return "HVDC_LINE";

    case Type::STATIC_VAR_COMPENSATOR:
    return "STATIC_VAR_COMPENSATOR";

    case Type::BUSBAR_SECTION:
    return "BUSBAR_SECTION";
  }

  return "UNKNOWN_TYPE";
}

}  // namespace inputs
}  // namespace dfl
