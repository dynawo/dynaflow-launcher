//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  DydEvent.cpp
 *
 * @brief Dynaflow launcher DYD file writer for events in a contingency implementation file
 *
 */

#include "DydEvent.h"

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

DydEvent::DydEvent(DydEventDefinition&& def) : def_{std::forward<DydEventDefinition>(def)} {}

void
DydEvent::write() const {
  using Type = dfl::inputs::ContingencyElement::Type;

  dynamicdata::XmlExporter exporter;

  auto dynamicModels = dynamicdata::DynamicModelsCollectionFactory::newCollection();

  // macro connectors
  const auto& connector = dynamicdata::MacroConnectorFactory::newMacroConnector("MC_EventQuadripoleDisconnection");
  connector->addConnect("event_state1_value", "@NAME@_state_value");
  dynamicModels->addMacroConnector(connector);

  // models and connections
  for (const auto& element : def_.contingency.elements) {
    switch (element.type) {
    case Type::BRANCH:
    case Type::LINE:
    case Type::TWO_WINDINGS_TRANSFORMER:
      dynamicModels->addModel(buildBranchDisconnection(element.id, def_.basename));
      dynamicModels->addMacroConnect(buildBranchDisconnectionConnect(element.id));
      break;
    case Type::LOAD:
      dynamicModels->addModel(buildSwitchOffSignalDisconnection(element.id, def_.basename));
      addSwitchOffSignalDisconnectionConnect(dynamicModels, element.id, "switchOff2");
      break;
    case Type::GENERATOR:
      dynamicModels->addModel(buildSwitchOffSignalDisconnection(element.id, def_.basename));
      addSwitchOffSignalDisconnectionConnect(dynamicModels, element.id, "generator_switchOffSignal2");
      break;
    case Type::HVDC_LINE:
      dynamicModels->addModel(buildSwitchOffSignalDisconnection(element.id, def_.basename));
      addSwitchOffSignalDisconnectionConnect(dynamicModels, element.id, "hvdc_switchOffSignal2Side1");
      addSwitchOffSignalDisconnectionConnect(dynamicModels, element.id, "hvdc_switchOffSignal2Side2");
      break;
    case Type::STATIC_VAR_COMPENSATOR:
      dynamicModels->addModel(buildSwitchOffSignalDisconnection(element.id, def_.basename));
      addSwitchOffSignalDisconnectionConnect(dynamicModels, element.id, "SVarC_switchOffSignal2");
      break;
    default:
      dynamicModels->addModel(buildNetworkStateDisconnection(element.id, def_.basename));
      addNetworkStateDisconnectionConnect(dynamicModels, element.id);
    }
  }

  exporter.exportToFile(dynamicModels, def_.filename, constants::xmlEncoding);
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
DydEvent::buildBranchDisconnection(const std::string& branchId, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel("Disconnect_" + branchId);
  model->setLib("EventQuadripoleDisconnection");
  model->setParFile(basename + ".par");
  model->setParId("Disconnect_" + branchId);
  return model;
}

boost::shared_ptr<dynamicdata::MacroConnect>
DydEvent::buildBranchDisconnectionConnect(const std::string& branchId) {
  auto connect = dynamicdata::MacroConnectFactory::newMacroConnect("MC_EventQuadripoleDisconnection", "Disconnect_" + branchId, constants::networkModelName);
  connect->setName2(branchId);
  return connect;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
DydEvent::buildSwitchOffSignalDisconnection(const std::string& elementId, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel("Disconnect_" + elementId);
  model->setLib("EventSetPointBoolean");
  model->setParFile(basename + ".par");
  model->setParId("Disconnect_" + elementId);
  return model;
}

void
DydEvent::addSwitchOffSignalDisconnectionConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModels, const std::string& elementId,
                                                 const std::string& var2) {
  dynamicModels->addConnect("Disconnect_" + elementId, "event_state1", elementId, var2);
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
DydEvent::buildNetworkStateDisconnection(const std::string& elementId, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel("Disconnect_" + elementId);
  model->setLib("EventSetPointReal");
  model->setParFile(basename + ".par");
  model->setParId("Disconnect_" + elementId);
  return model;
}

void
DydEvent::addNetworkStateDisconnectionConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModels, const std::string& elementId) {
  dynamicModels->addConnect("Disconnect_" + elementId, "event_state1", constants::networkModelName, elementId + "_state");
}

void
DydEvent::addNetworkState12DisconnectionConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModels, const std::string& elementId) {
  dynamicModels->addConnect("Disconnect_" + elementId, "event_state1", constants::networkModelName, elementId + "_state1");
  dynamicModels->addConnect("Disconnect_" + elementId, "event_state1", constants::networkModelName, elementId + "_state2");
}

}  // namespace outputs
}  // namespace dfl
