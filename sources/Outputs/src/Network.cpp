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
 * @file  Network.cpp
 *
 * @brief Dynaflow launcher Network file writer implementation file
 *
 */

#include "Network.h"

#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARXmlExporter.h>
#include <type_traits>
#include "Constants.h"

namespace dfl {
namespace outputs {

Network::Network(NetworkDefinition&& def) : def_{std::forward<NetworkDefinition>(def)} {}

void
Network::write() const {
  parameters::XmlExporter exporter;

  boost::shared_ptr<parameters::ParametersSetCollection> paramSetCollection = parameters::ParametersSetCollectionFactory::newCollection();
  // adding load parameter set
  def_.parNetwork_->write(paramSetCollection, def_.startingPointMode_);

  exporter.exportToFile(paramSetCollection, def_.filepath_.generic_string(), constants::xmlEncoding);
}

}  // namespace outputs
}  // namespace dfl
