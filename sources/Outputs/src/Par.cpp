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
 * @file  Par.cpp
 *
 * @brief Dynaflow launcher PAR file writer implementation file
 *
 */

#include "Par.h"

#include "Constants.h"

#include <PARParameter.h>
#include <PARParameterFactory.h>
#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARParametersSetFactory.h>
#include <PARReference.h>
#include <PARReferenceFactory.h>
#include <PARXmlExporter.h>
#include <boost/filesystem.hpp>

namespace dfl {
namespace outputs {

namespace helper {

template<class T>
static boost::shared_ptr<parameters::Parameter>
buildParameter(const std::string& name, const T& value) {
  return parameters::ParameterFactory::newParameter(name, value);
}

static boost::shared_ptr<parameters::Reference>
buildReference(const std::string& name, const std::string& origName, const std::string& type) {
  auto ref = parameters::ReferenceFactory::newReference(name);
  ref->setOrigData("IIDM");
  ref->setOrigName(origName);
  ref->setType(type);

  return ref;
}

}  // namespace helper

Par::Par(ParDefinition&& def) : def_{std::forward<ParDefinition>(def)} {}

void
Par::write() {
  parameters::XmlExporter exporter;

  auto collection = parameters::ParametersSetCollectionFactory::newCollection();
  auto constants = writeConstantSets(def_.generators.size());
  for (auto it = constants.begin(); it != constants.end(); ++it) {
    collection->addParametersSet(*it);
  }

  for (auto it = def_.generators.begin(); it != def_.generators.end(); ++it) {
    auto set = writeGenerator(*it, def_.basename, def_.dirname);
    if (set) {
      collection->addParametersSet(set);
    }
  }

  exporter.exportToFile(collection, def_.filepath, constants::xmlEncoding);
}

void
Par::updateSignalNGenerator(boost::shared_ptr<parameters::ParametersSet> set) {
  set->addParameter(helper::buildParameter("generator_KGover", 1.));
  set->addParameter(helper::buildParameter("generator_QMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_QMax", constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMin", -constants::powerValueMax));
  set->addParameter(helper::buildParameter("generator_PMax", constants::powerValueMax));
  set->addReference(helper::buildReference("generator_PNom", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));
}

std::vector<boost::shared_ptr<parameters::ParametersSet>>
Par::writeConstantSets(unsigned int nb_generators) {
  std::vector<boost::shared_ptr<parameters::ParametersSet>> ret;

  // Load
  auto set = parameters::ParametersSetFactory::newInstance(constants::loadParId);
  set->addParameter(helper::buildParameter("load_Alpha", 1.5));
  set->addParameter(helper::buildParameter("load_Beta", 2.5));
  set->addParameter(helper::buildParameter("load_UMaxPu", 1.05));
  set->addParameter(helper::buildParameter("load_UMinPu", 0.95));
  set->addParameter(helper::buildParameter("load_tFilter", 10.));
  set->addReference(helper::buildReference("load_P0Pu", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_Q0Pu", "q_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_UPhase0", "angle_pu", "DOUBLE"));

  ret.push_back(set);

  // Signal N
  set = parameters::ParametersSetFactory::newInstance(constants::signalNParId);
  set->addParameter(helper::buildParameter("nbGen", static_cast<int>(nb_generators)));  // static cast because of Dynawo API
  ret.push_back(set);

  // Signal N generator
  set = parameters::ParametersSetFactory::newInstance(constants::signalNGeneratorParId);
  updateSignalNGenerator(set);
  ret.push_back(set);

  // Signal N generator with impendance
  set = parameters::ParametersSetFactory::newInstance(constants::impSignalNGeneratorParId);
  updateSignalNGenerator(set);
  updateCouplingParameters(set);
  ret.push_back(set);

  return ret;
}

boost::shared_ptr<parameters::ParametersSet>
Par::writeGenerator(const algo::GeneratorDefinition& def, const std::string& basename, const std::string& dirname) {
  if (def.model == algo::GeneratorDefinition::ModelType::SIGNALN || def.model == algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN) {
    // already processed by constant
    return nullptr;
  }
  std::size_t hashId = constants::hash(def.id);
  std::string hashIdStr = std::to_string(hashId);
  auto set = parameters::ParametersSetFactory::newInstance(hashIdStr);
  //  Use the hash id in exported files to prevent use of non-ascii characters

  set->addParameter(helper::buildParameter("generator_KGover", 1.));
  if (def.model == algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN ||
      def.model == algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_SIGNALN) {
    updateCouplingParameters(set);
  }

  set->addParameter(helper::buildParameter("generator_tFilter", 0.001));
  // Qmax and QMin are determined in dynawo according to reactive capabilities curves and min max
  // we need a small numerical tolerance in case the starting point of the reactive injection is exactly
  // on the limit of the reactive capability curve
  set->addParameter(helper::buildParameter("generator_QMin0", def.qmin - 1));
  set->addParameter(helper::buildParameter("generator_QMax0", def.qmax + 1));

  auto dirname_diagram = boost::filesystem::path(dirname);
  dirname_diagram.append(basename + constants::diagramDirectorySuffix).append(constants::diagramFilename(def));

  set->addParameter(helper::buildParameter("generator_QMaxTableFile", dirname_diagram.generic_string()));
  set->addParameter(helper::buildParameter("generator_QMaxTableName", hashIdStr + constants::diagramMaxTableSuffix));
  set->addParameter(helper::buildParameter("generator_QMinTableFile", dirname_diagram.generic_string()));
  set->addParameter(helper::buildParameter("generator_QMinTableName", hashIdStr + constants::diagramMinTableSuffix));

  set->addReference(helper::buildReference("generator_PNom", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_PMin", "pMin", "DOUBLE"));
  set->addReference(helper::buildReference("generator_PMax", "pMax", "DOUBLE"));
  set->addReference(helper::buildReference("generator_P0Pu", "p_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_Q0Pu", "q_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("generator_UPhase0", "angle_pu", "DOUBLE"));

  return set;
}

void
Par::updateCouplingParameters(boost::shared_ptr<parameters::ParametersSet> set) {
  set->addParameter(helper::buildParameter("line_BPu", 0.));
  set->addParameter(helper::buildParameter("line_GPu", 0.));
  set->addParameter(helper::buildParameter("line_RPu", 0.));
  set->addParameter(helper::buildParameter("line_XPu", 0.0001));
}

}  // namespace outputs
}  // namespace dfl
