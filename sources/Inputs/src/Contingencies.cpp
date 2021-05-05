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
      for (const boost::property_tree::ptree::value_type& pte : ptcontingency.get_child("elements")) {
        LOG(debug) << "  " << pte.second.get<std::string>("id") << " (" << pte.second.get<std::string>("type") << ")" << LOG_ENDL;
        ContingencyElementDefinition element(pte.second.get<std::string>("id"));
        element.type = pte.second.get<std::string>("type");
        contingency.elements.push_back(element);
      }
      contingencies.push_back(contingency);
    }
  } catch (std::exception& e) {
    LOG(error) << "Error while reading contingencies file " << filepath << " : " << e.what() << LOG_ENDL;
    std::exit(EXIT_FAILURE);
  }
}

void
Contingencies::log() {
    LOG(info) << "Contingencies" << LOG_ENDL;
    for (auto c = contingencies.begin(); c != contingencies.end(); ++c) {
      LOG(info) << c->id << LOG_ENDL;
      for (auto e = c->elements.begin(); e != c->elements.end(); ++e) {
        LOG(info) << "  " << e->id << " (" << e->type << ")" << LOG_ENDL;
      }
    }
}

}  // namespace inputs
}  // namespace dfl
