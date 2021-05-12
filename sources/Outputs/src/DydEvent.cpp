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

const std::string DydEvent::networkModelName_("NETWORK");

DydEvent::DydEvent(DydEventDefinition&& def) : def_{std::forward<DydEventDefinition>(def)} {}

void
DydEvent::write() {
  dynamicdata::XmlExporter exporter;

  auto dynamicModels = dynamicdata::DynamicModelsCollectionFactory::newCollection();

  // macro connectors
  auto macro_connectors = writeMacroConnectors();
  for (auto it = macro_connectors.begin(); it != macro_connectors.end(); ++it) {
    dynamicModels->addMacroConnector(*it);
  }

  // models and connections
  for (auto e = def_.contingency.elements.begin(); e != def_.contingency.elements.end(); ++e) {
    if (e->type == "BRANCH") {
      dynamicModels->addModel(writeBranchDisconnection(e->id, def_.basename));
      dynamicModels->addMacroConnect(writeBranchDisconnectionConnect(e->id));
    } else if (e->type == "GENERATOR") {
      dynamicModels->addModel(writeElementDisconnection(e->id, def_.basename));
      addElementDisconnectionConnect(dynamicModels, e->id, "generator_switchOffSignal2");
    } else if (e->type == "LOAD") {
      dynamicModels->addModel(writeElementDisconnection(e->id, def_.basename));
      addElementDisconnectionConnect(dynamicModels, e->id, "switchOff2");
    } else if (e->type == "SHUNT") {
      auto model = dynamicdata::BlackBoxModelFactory::newModel("Disconnect_" + e->id);
      model->setLib("EventSetPointReal");
      model->setParFile(def_.basename + ".par");
      model->setParId("Disconnect_" + e->id);
      dynamicModels->addModel(model);
      dynamicModels->addConnect("Disconnect_" + e->id, "event_state1", "NETWORK", e->id + "_state");
    } else {
      throw std::invalid_argument("Element type " + e->type + " not expected in contingency definition");
    }
  }

  exporter.exportToFile(dynamicModels, def_.filename, constants::xmlEncoding);
}

std::vector<boost::shared_ptr<dynamicdata::MacroConnector>>
DydEvent::writeMacroConnectors() {
  std::vector<boost::shared_ptr<dynamicdata::MacroConnector>> ret;

  auto connector = dynamicdata::MacroConnectorFactory::newMacroConnector("MC_EventQuadripoleDisconnection");
  connector->addConnect("event_state1_value", "@NAME@_state_value");
  ret.push_back(connector);

  return ret;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
DydEvent::writeBranchDisconnection(const std::string& branchId, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel("Disconnect_" + branchId);

  model->setLib("EventQuadripoleDisconnection");
  model->setParFile(basename + ".par");
  model->setParId("Disconnect_" + branchId);

  return model;
}

boost::shared_ptr<dynamicdata::MacroConnect>
DydEvent::writeBranchDisconnectionConnect(const std::string& branchId) {
  auto connect = dynamicdata::MacroConnectFactory::newMacroConnect("MC_EventQuadripoleDisconnection", "Disconnect_" + branchId, networkModelName_);
  connect->setName2(branchId);
  return connect;
}

boost::shared_ptr<dynamicdata::BlackBoxModel>
DydEvent::writeElementDisconnection(const std::string& elementId, const std::string& basename) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel("Disconnect_" + elementId);

  model->setLib("EventSetPointBoolean");
  model->setParFile(basename + ".par");
  model->setParId("Disconnect_" + elementId);

  return model;
}

void
DydEvent::addElementDisconnectionConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModels, const std::string& elementId, const std::string& var2) {
  dynamicModels->addConnect("Disconnect_" + elementId, "event_state1", elementId, var2);
}

}  // namespace outputs
}  // namespace dfl
