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

void
DydLoads::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename) {
  for (const auto& load : loadsDefinitions_) {
    if (load.isNetwork()) {
      continue;
    }
    std::unique_ptr<dynamicdata::BlackBoxModel> blackBoxModel =
        helper::buildBlackBoxStaticId(load.id, load.id, "DYNModelLoadRestorativeWithLimits", basename + ".par", constants::loadParId);
    blackBoxModel->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefLoadName_));
    std::unique_ptr<dynamicdata::MacroConnect> loadMacroConnectRef =
        dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorLoadName_, load.id, constants::networkModelName);
    dynamicModelsToConnect->addModel(std::move(blackBoxModel));
    dynamicModelsToConnect->addMacroConnect(std::move(loadMacroConnectRef));
  }
  writeMacroConnector(dynamicModelsToConnect);
  writeMacroStaticReference(dynamicModelsToConnect);
}

void
DydLoads::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!loadsDefinitions_.empty()) {
    std::unique_ptr<dynamicdata::MacroConnector> connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorLoadName_);
    connector->addConnect("Ur_value", "@STATIC_ID@@NODE@_ACPIN_V_re");
    connector->addConnect("Ui_value", "@STATIC_ID@@NODE@_ACPIN_V_im");
    connector->addConnect("Ir_value", "@STATIC_ID@@NODE@_ACPIN_i_re");
    connector->addConnect("Ii_value", "@STATIC_ID@@NODE@_ACPIN_i_im");
    connector->addConnect("switchOff1_value", "@STATIC_ID@@NODE@_switchOff_value");
    dynamicModelsToConnect->addMacroConnector(std::move(connector));
  }
}

void
DydLoads::writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!loadsDefinitions_.empty()) {
    std::unique_ptr<dynamicdata::MacroStaticReference> macroStaticReference =
        dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefLoadName_);
    macroStaticReference->addStaticRef("PPu_value", "p");
    macroStaticReference->addStaticRef("QPu_value", "q");
    macroStaticReference->addStaticRef("state_value", "state");
    dynamicModelsToConnect->addMacroStaticReference(std::move(macroStaticReference));
  }
}
}  // namespace outputs
}  // namespace dfl
