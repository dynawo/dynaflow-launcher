//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParNetwork.h"
#include "ParCommon.h"

namespace dfl {
namespace outputs {

void
ParNetwork::write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection,
                  dfl::inputs::Configuration::StartingPointMode startingPointMode) {
    paramSetCollection->addParametersSet(writeNetworkSet(startingPointMode));
}

boost::shared_ptr<parameters::ParametersSet>
ParNetwork::writeNetworkSet(dfl::inputs::Configuration::StartingPointMode startingPointMode) {
  // Network
  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(constants::networkParId));

  set->addParameter(helper::buildParameter("capacitor_no_reclosing_delay", 300.));
  set->addParameter(helper::buildParameter("dangling_line_currentLimit_maxTimeOperation", 90.));
  set->addParameter(helper::buildParameter("line_currentLimit_maxTimeOperation", 90.));
  set->addParameter(helper::buildParameter("load_Tp", 90.));
  set->addParameter(helper::buildParameter("load_Tq", 90.));
  set->addParameter(helper::buildParameter("load_alpha", 0.));
  set->addParameter(helper::buildParameter("load_alphaLong", 0.));
  set->addParameter(helper::buildParameter("load_beta", 0.));
  set->addParameter(helper::buildParameter("load_betaLong", 0.));
  set->addParameter(helper::buildParameter("load_isControllable", false));
  set->addParameter(helper::buildParameter("load_isRestorative", false));
  set->addParameter(helper::buildParameter("load_zPMax", 100.));
  set->addParameter(helper::buildParameter("load_zQMax", 100.));
  set->addParameter(helper::buildParameter("reactance_no_reclosing_delay", 0.));
  set->addParameter(helper::buildParameter("transformer_currentLimit_maxTimeOperation", 90.));
  set->addParameter(helper::buildParameter("transformer_t1st_HT", 60.));
  set->addParameter(helper::buildParameter("transformer_t1st_THT", 30.));
  set->addParameter(helper::buildParameter("transformer_tNext_HT", 10.));
  set->addParameter(helper::buildParameter("transformer_tNext_THT", 10.));
  set->addParameter(helper::buildParameter("transformer_tolV", 0.01499999970000000));

  switch (startingPointMode) {
  case dfl::inputs::Configuration::StartingPointMode::WARM:
      set->addParameter(helper::buildParameter("startingPointMode", std::string("WARM")));
    break;
  case dfl::inputs::Configuration::StartingPointMode::FLAT:
      set->addParameter(helper::buildParameter("startingPointMode", std::string("FLAT")));
    break;
  }

  return set;
}

}  // namespace outputs
}  // namespace dfl
