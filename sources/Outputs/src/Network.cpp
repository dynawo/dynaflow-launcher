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
 * @file  Network.cpp
 *
 * @brief Dynaflow launcher Network model parameters file writer implementation file
 *
 */

#include "Network.h"

#include "OutputsConstants.h"
#include "ParCommon.h"

#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARXmlExporter.h>

namespace dfl {
namespace outputs {

Network::Network(NetworkDefinition&& def) : def_{std::move(def)} {}

void
Network::write() const {
  parameters::XmlExporter exporter;
  std::unique_ptr<parameters::ParametersSetCollection> paramSetCollection = parameters::ParametersSetCollectionFactory::newCollection();
  paramSetCollection->addParametersSet(writeNetworkSet());
  boost::filesystem::path networkFileName(def_.filepath_);
  exporter.exportToFile(std::move(paramSetCollection), networkFileName.generic_string(), constants::xmlEncoding);
}

std::shared_ptr<parameters::ParametersSet>
Network::writeNetworkSet() const {
  // Network
  auto set = parameters::ParametersSetFactory::newParametersSet(constants::networkParId);

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

  switch (def_.startingPointMode_) {
  case dfl::inputs::Configuration::StartingPointMode::WARM:
    set->addParameter(helper::buildParameter("startingPointMode", std::string("warm")));
    break;
  case dfl::inputs::Configuration::StartingPointMode::FLAT:
    set->addParameter(helper::buildParameter("startingPointMode", std::string("flat")));
    break;
  }

  return set;
}

}  // namespace outputs
}  // namespace dfl
