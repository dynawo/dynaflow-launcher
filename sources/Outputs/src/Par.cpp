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

#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARReference.h>
#include <PARReferenceFactory.h>
#include <PARXmlExporter.h>
#include <type_traits>

namespace dfl {
namespace outputs {

Par::Par(ParDefinition&& def) : def_{std::forward<ParDefinition>(def)} {}

void
Par::write() const {
  parameters::XmlExporter exporter;

  auto paramSetCollection = parameters::ParametersSetCollectionFactory::newCollection();
  // adding load parameter set
  def_.parLoads_->write(paramSetCollection, def_.startingPointMode_);
  def_.parGenerator_->write(paramSetCollection, def_.activePowerCompensation_, def_.basename_, def_.dirname_, def_.busesRegulatedBySeveralGenerators_,
                            def_.startingPointMode_, def_.dynamicDataBaseManager_);
  def_.parSVarC_->write(paramSetCollection, def_.startingPointMode_);
  def_.parHvdc_->write(paramSetCollection, def_.basename_, def_.dirname_, def_.startingPointMode_);
  def_.parDynModel_->write(paramSetCollection, def_.dynamicDataBaseManager_, def_.shuntCounters_, def_.linesByIdDefinitions_, def_.tfosByIdDefinitions_);

  exporter.exportToFile(paramSetCollection, def_.filepath_.generic_string(), constants::xmlEncoding);
}

}  // namespace outputs
}  // namespace dfl
