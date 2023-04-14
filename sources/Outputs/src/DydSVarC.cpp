//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydSVarC.h"

#include "DydCommon.h"
#include "OutputsConstants.h"

#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>

namespace dfl {
namespace outputs {

const std::unordered_map<algo::StaticVarCompensatorDefinition::ModelType, std::string> DydSVarC::svarcModelsNames_ = {
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPV, "StaticVarCompensatorPV"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING, "StaticVarCompensatorPVModeHandling"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROP, "StaticVarCompensatorPVProp"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING, "StaticVarCompensatorPVPropModeHandling"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE, "StaticVarCompensatorPVPropRemote"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING, "StaticVarCompensatorPVPropRemoteModeHandling"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE, "StaticVarCompensatorPVRemote"),
    std::make_pair(algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING, "StaticVarCompensatorPVRemoteModeHandling")};

void
DydSVarC::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename) {
  for (const auto& svarc : svarcsDefinitions_) {
    if (svarc.isNetwork()) {
      continue;
    }
    std::string parId = constants::uuid(svarc.id);
    auto blackBoxModel = helper::buildBlackBoxStaticId(svarc.id, svarc.id, svarcModelsNames_.at(svarc.model), basename + ".par", parId);
    blackBoxModel->addMacroStaticRef(dynamicdata::MacroStaticRefFactory::newMacroStaticRef(macroStaticRefSVarCName_));
    switch (svarc.model) {
    case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING:
    case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING:
    case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING:
    case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING:
      blackBoxModel->addStaticRef("SVarC_modeHandling_mode_value", "regulatingMode");
      break;
    default:
      break;
    }
    auto sVarCMacroConnectRef = dynamicdata::MacroConnectFactory::newMacroConnect(macroConnectorSVarCName_, svarc.id, constants::networkModelName);
    dynamicModelsToConnect->addModel(blackBoxModel);
    dynamicModelsToConnect->addMacroConnect(sVarCMacroConnectRef);
  }
  writeMacroConnector(dynamicModelsToConnect);
  writeMacroStaticReference(dynamicModelsToConnect);
}

void
DydSVarC::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!svarcsDefinitions_.empty()) {
    auto connector = dynamicdata::MacroConnectorFactory::newMacroConnector(macroConnectorSVarCName_);
    connector->addConnect("SVarC_terminal", "@STATIC_ID@@NODE@_ACPIN");
    dynamicModelsToConnect->addMacroConnector(connector);
  }
}

void
DydSVarC::writeMacroStaticReference(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect) {
  if (!svarcsDefinitions_.empty()) {
    auto macroStaticReference = dynamicdata::MacroStaticReferenceFactory::newMacroStaticReference(macroStaticRefSVarCName_);
    macroStaticReference->addStaticRef("SVarC_PInjPu", "p");
    macroStaticReference->addStaticRef("SVarC_QInjPu", "q");
    macroStaticReference->addStaticRef("SVarC_state", "state");
    dynamicModelsToConnect->addMacroStaticReference(macroStaticReference);
  }
}
}  // namespace outputs
}  // namespace dfl
