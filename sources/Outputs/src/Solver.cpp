//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  Solver.cpp
 *
 * @brief Dynaflow launcher solver file writer implementation file
 *
 */

#include "Solver.h"

#include "ParCommon.h"

#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARXmlExporter.h>

namespace dfl {
namespace outputs {

Solver::Solver(SolverDefinition&& def) : def_{std::forward<SolverDefinition>(def)} {}

void
Solver::write() const {
  parameters::XmlExporter exporter;
  auto paramSetCollection = parameters::ParametersSetCollectionFactory::newCollection();
  paramSetCollection->addParametersSet(writeSolverSet());
  boost::filesystem::path solverFileName(def_.outputDir_);
  solverFileName.append(constants::solverParFileName);
  exporter.exportToFile(paramSetCollection, solverFileName.generic_string(), constants::xmlEncoding);
}

boost::shared_ptr<parameters::ParametersSet>
Solver::writeSolverSet() const {
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet("SimplifiedSolver"));
  set->addParameter(helper::buildParameter("fnormtol", 1e-4));
  set->addParameter(helper::buildParameter("fnormtolAlg", 1e-4));
  set->addParameter(helper::buildParameter("fnormtolAlgJ", 1e-4));
  set->addParameter(helper::buildParameter("hMax", def_.timeStep_));
  set->addParameter(helper::buildParameter("hMin", 1.));
  set->addParameter(helper::buildParameter("initialaddtol", 0.1));
  set->addParameter(helper::buildParameter("initialaddtolAlg", 0.1));
  set->addParameter(helper::buildParameter("initialaddtolAlgJ", 0.1));
  set->addParameter(helper::buildParameter("kReduceStep", 0.5));
  set->addParameter(helper::buildParameter("linearSolverName", std::string("KLU")));
  set->addParameter(helper::buildParameter("maxNewtonTry", 10));
  set->addParameter(helper::buildParameter("msbset", 0));
  set->addParameter(helper::buildParameter("msbsetAlg", 1));
  set->addParameter(helper::buildParameter("msbsetAlgJ", 1));
  set->addParameter(helper::buildParameter("mxiter", 15));
  set->addParameter(helper::buildParameter("mxiterAlg", 30));
  set->addParameter(helper::buildParameter("mxiterAlgJ", 50));
  set->addParameter(helper::buildParameter("mxnewtstep", 100000.));
  set->addParameter(helper::buildParameter("mxnewtstepAlg", 100000.));
  set->addParameter(helper::buildParameter("mxnewtstepAlgJ", 100000.));
  set->addParameter(helper::buildParameter("printfl", 0));
  set->addParameter(helper::buildParameter("printflAlg", 0));
  set->addParameter(helper::buildParameter("printflAlgJ", 0));
  set->addParameter(helper::buildParameter("scsteptol", 1.e-4));
  set->addParameter(helper::buildParameter("scsteptolAlg", 1.e-4));
  set->addParameter(helper::buildParameter("scsteptolAlgJ", 1.e-4));
  return set;
}

}  // namespace outputs
}  // namespace dfl
