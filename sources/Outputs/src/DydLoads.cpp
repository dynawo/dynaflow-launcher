//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydLoads.h"

#include "DydCommon.h"
#include "OutputsConstants.h"

#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>

namespace dfl {
namespace outputs {

void DydLoads::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const std::string &basename) {
  for (const auto &load : loadsDefinitions_) {
    if (load.isNetwork()) {
      continue;
    }
    auto blackBoxModel = helper::buildBlackBoxStaticId(load.id, load.id, "DYNModelLoadRestorativeWithLimits", basename + ".par", constants::loadParId);
    blackBoxModel->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefLoadName_));
    auto loadMacroConnectRef = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorLoadName_, load.id, constants::networkModelName);
    dynamicModelsToConnect->addModel(blackBoxModel);
    dynamicModelsToConnect->addMacroConnect(loadMacroConnectRef);
  }
  writeMacroConnector(dynamicModelsToConnect);
  writeMacroStaticReference(dynamicModelsToConnect);
}

void DydLoads::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect) {
  if (!loadsDefinitions_.empty()) {
    auto connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorLoadName_);
    connector->addConnect("Ur_value", "@STATIC_ID@@NODE@_ACPIN_V_re");
    connector->addConnect("Ui_value", "@STATIC_ID@@NODE@_ACPIN_V_im");
    connector->addConnect("Ir_value", "@STATIC_ID@@NODE@_ACPIN_i_re");
    connector->addConnect("Ii_value", "@STATIC_ID@@NODE@_ACPIN_i_im");
    connector->addConnect("switchOff1_value", "@STATIC_ID@@NODE@_switchOff_value");
    dynamicModelsToConnect->addMacroConnector(connector);
  }
}

void DydLoads::writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect) {
  if (!loadsDefinitions_.empty()) {
    auto macroStaticReference = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefLoadName_);
    macroStaticReference->addStaticRef("PPu_value", "p");
    macroStaticReference->addStaticRef("QPu_value", "q");
    macroStaticReference->addStaticRef("state_value", "state");
    dynamicModelsToConnect->addMacroStaticReference(macroStaticReference);
  }
}
}  // namespace outputs
}  // namespace dfl
