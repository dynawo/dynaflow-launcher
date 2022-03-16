//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParLoads.h"

#include "ParCommon.h"

namespace dfl {
namespace outputs {

void
ParLoads::write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection) {
  if (!loadsDefinitions_.empty()) {
    paramSetCollection->addParametersSet(writeConstantLoadsSet());
  }
}

boost::shared_ptr<parameters::ParametersSet>
ParLoads::writeConstantLoadsSet() {
  // Load
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(constants::loadParId));
  set->addParameter(helper::buildParameter("load_Alpha", 1.5));
  set->addParameter(helper::buildParameter("load_Beta", 2.5));
  set->addParameter(helper::buildParameter("load_UMax0Pu", 1.15));
  set->addParameter(helper::buildParameter("load_UMin0Pu", 0.85));
  set->addParameter(helper::buildParameter("load_UDeadBandPu", 0.01));
  set->addParameter(helper::buildParameter("load_tFilter", 10.));
  set->addReference(helper::buildReference("load_P0Pu", "p0_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_Q0Pu", "q0_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_U0Pu", "v_pu", "DOUBLE"));
  set->addReference(helper::buildReference("load_UPhase0", "angle_pu", "DOUBLE"));
  return set;
}

}  // namespace outputs
}  // namespace dfl
