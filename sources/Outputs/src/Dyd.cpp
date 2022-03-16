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
 * @file  Dyd.cpp
 *
 * @brief Dynaflow launcher DYD file writer implementation file
 *
 */

#include "Dyd.h"

#include "Constants.h"
#include "Log.h"

#include <DYDBlackBoxModelFactory.h>
#include <DYDDynamicModelsCollection.h>
#include <DYDDynamicModelsCollectionFactory.h>
#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRef.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>
#include <DYDStaticRef.h>
#include <DYDXmlExporter.h>
#include <DYNCommon.h>

namespace dfl {
namespace outputs {

Dyd::Dyd(DydDefinition&& def) : def_{std::forward<DydDefinition>(def)} {}

void
Dyd::write() const {
  dynamicdata::XmlExporter exporter;
  auto dynamicModelsToConnect = dynamicdata::DynamicModelsCollectionFactory::newCollection();
  def_.dydDynModel_->write(dynamicModelsToConnect, def_.basename_, def_.dynamicDataBaseManager_);

  def_.dydLoads_->write(dynamicModelsToConnect, def_.basename_);
  def_.dydGenerator_->write(dynamicModelsToConnect, def_.basename_, def_.busesWithDynamicModel_, def_.slackNode_->id);
  def_.dydHvdc_->write(dynamicModelsToConnect, def_.basename_);
  def_.dydSVarC_->write(dynamicModelsToConnect, def_.basename_);

  exporter.exportToFile(dynamicModelsToConnect, def_.filename_, constants::xmlEncoding);
}

}  // namespace outputs
}  // namespace dfl
