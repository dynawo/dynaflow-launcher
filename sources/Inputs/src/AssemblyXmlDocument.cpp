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
 * @file AssemblyXmlDocument.cpp
 * @brief Assembly xml document handler implementation
 */

#include "AssemblyXmlDocument.h"

namespace parser = xml::sax::parser;

namespace dfl {
namespace inputs {

static parser::namespace_uri ns("");

AssemblyXmlDocument::AssemblyXmlDocument() :
    macroConnectionHandler_(parser::ElementName(ns, "macroConnection")),
    singleAssociationHandler_(parser::ElementName(ns, "singleAssociation")),
    multipleAssociationHandler_(parser::ElementName(ns, "multipleAssociation")),
    dynamicAutomatonHandler_(parser::ElementName(ns, "dynamicAutomaton")) {
  onElement(ns("assembling/macroConnection"), macroConnectionHandler_);
  onElement(ns("assembling/singleAssociation"), singleAssociationHandler_);
  onElement(ns("assembling/multipleAssociation"), multipleAssociationHandler_);
  onElement(ns("assembling/dynamicAutomaton"), dynamicAutomatonHandler_);

  macroConnectionHandler_.onStart([this]() { macroConnectionHandler_.currentMacroConnection = MacroConnection(); });
  macroConnectionHandler_.onEnd([this]() {
    macroConnections_.push_back(*macroConnectionHandler_.currentMacroConnection);
    macroConnectionHandler_.currentMacroConnection.reset();
  });

  singleAssociationHandler_.onStart([this]() { singleAssociationHandler_.currentSingleAssociation = SingleAssociation(); });
  singleAssociationHandler_.onEnd([this]() {
    singleAssociations_.push_back(*singleAssociationHandler_.currentSingleAssociation);
    singleAssociationHandler_.currentSingleAssociation.reset();
  });

  multipleAssociationHandler_.onStart([this]() { multipleAssociationHandler_.currentMultipleAssociation = MultipleAssociation(); });
  multipleAssociationHandler_.onEnd([this]() {
    multipleAssociations_.push_back(*multipleAssociationHandler_.currentMultipleAssociation);
    multipleAssociationHandler_.currentMultipleAssociation.reset();
  });

  dynamicAutomatonHandler_.onStart([this]() { dynamicAutomatonHandler_.currentDynamicAutomaton = DynamicAutomaton(); });
  dynamicAutomatonHandler_.onEnd([this]() {
    dynamicAutomatons_.push_back(*dynamicAutomatonHandler_.currentDynamicAutomaton);
    dynamicAutomatonHandler_.currentDynamicAutomaton.reset();
  });
}

AssemblyXmlDocument::ConnectionHandler::ConnectionHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentConnection->var1 = attributes["var1"].as_string();
    currentConnection->var2 = attributes["var2"].as_string();
    currentConnection->required = attributes["required"].as<bool>();
    currentConnection->internal = attributes["internal"].as<bool>();
  });
}

AssemblyXmlDocument::BusHandler::BusHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentBus->name = attributes["name"].as_string();
    currentBus->voltageLevel = attributes["voltageLevel"].as_string();
  });
}

AssemblyXmlDocument::BusBarSectionHandler::BusBarSectionHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentBusBarSection->id = attributes["id"].as_string(); });
}

AssemblyXmlDocument::ShuntHandler::ShuntHandler(const elementName_type& root) {
  onStartElement(
      root, [this](const parser::ElementName&, const attributes_type& attributes) { currentShunt->voltageLevel = attributes["voltageLevel"].as_string(); });
}

AssemblyXmlDocument::TfoHandler::TfoHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentTfo->name = attributes["name"].as_string(); });
}

AssemblyXmlDocument::LineHandler::LineHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentLine->name = attributes["name"].as_string(); });
}

AssemblyXmlDocument::MacroConnectHandler::MacroConnectHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentMacroConnect->id = attributes["id"].as_string();
    currentMacroConnect->macroConnection = attributes["macroConnection"].as_string();
  });
}

AssemblyXmlDocument::MacroConnectionHandler::MacroConnectionHandler(const elementName_type& root) : connectionHandler(parser::ElementName(ns, "connection")) {
  onElement(root + ns("connection"), connectionHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentMacroConnection->id = attributes["id"].as_string();
    currentMacroConnection->name2 = attributes["name2"].as<bool>();
    if (attributes.has("index1")) {
      currentMacroConnection->index1 = attributes["index1"].as<bool>();
    }
  });

  connectionHandler.onStart([this]() { connectionHandler.currentConnection = Connection(); });
  connectionHandler.onEnd([this]() {
    currentMacroConnection->connections.push_back(*connectionHandler.currentConnection);
    connectionHandler.currentConnection.reset();
  });
}

AssemblyXmlDocument::SingleAssociationHandler::SingleAssociationHandler(const elementName_type& root) :
    busHandler(parser::ElementName(ns, "bus")),
    busBarSectionHandler(parser::ElementName(ns, "busbarSection")),
    lineHandler(parser::ElementName(ns, "line")),
    tfoHandler(parser::ElementName(ns, "tfo")) {
  onElement(root + ns("bus"), busHandler);
  onElement(root + ns("busbarSection"), busBarSectionHandler);
  onElement(root + ns("line"), lineHandler);
  onElement(root + ns("tfo"), tfoHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentSingleAssociation->id = attributes["id"].as_string(); });

  busHandler.onStart([this]() { busHandler.currentBus = Bus(); });
  busHandler.onEnd([this]() {
    currentSingleAssociation->buses.push_back(*busHandler.currentBus);
    busHandler.currentBus.reset();
  });

  busBarSectionHandler.onStart([this]() { busBarSectionHandler.currentBusBarSection = BusBarSection(); });
  busBarSectionHandler.onEnd([this]() {
    currentSingleAssociation->busebars.push_back(*busBarSectionHandler.currentBusBarSection);
    busBarSectionHandler.currentBusBarSection.reset();
  });

  lineHandler.onStart([this]() { lineHandler.currentLine = Line(); });
  lineHandler.onEnd([this]() {
    currentSingleAssociation->line = lineHandler.currentLine;
    lineHandler.currentLine.reset();
  });

  tfoHandler.onStart([this]() { tfoHandler.currentTfo = Tfo(); });
  tfoHandler.onEnd([this]() {
    currentSingleAssociation->tfo = tfoHandler.currentTfo;
    tfoHandler.currentTfo.reset();
  });
}

AssemblyXmlDocument::MultipleAssociationHandler::MultipleAssociationHandler(const elementName_type& root) : shuntHandler(parser::ElementName(ns, "shunt")) {
  onElement(root + ns("shunt"), shuntHandler);

  onStartElement(root,
                 [this](const parser::ElementName&, const attributes_type& attributes) { currentMultipleAssociation->id = attributes["id"].as_string(); });

  shuntHandler.onStart([this]() { shuntHandler.currentShunt = Shunt(); });
  shuntHandler.onEnd([this]() {
    currentMultipleAssociation->shunt = *shuntHandler.currentShunt;
    shuntHandler.currentShunt.reset();
  });
}

AssemblyXmlDocument::DynamicAutomatonHandler::DynamicAutomatonHandler(const elementName_type& root) :
    macroConnectHandler(parser::ElementName(ns, "macroConnect")) {
  onElement(root + ns("macroConnect"), macroConnectHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentDynamicAutomaton->id = attributes["id"].as_string();
    currentDynamicAutomaton->lib = attributes["lib"].as_string();
    currentDynamicAutomaton->type = attributes["type"].as_string();
    currentDynamicAutomaton->access = attributes["access"].as_string();
  });

  macroConnectHandler.onStart([this]() { macroConnectHandler.currentMacroConnect = MacroConnect(); });
  macroConnectHandler.onEnd([this]() {
    currentDynamicAutomaton->macroConnects.push_back(*macroConnectHandler.currentMacroConnect);
    macroConnectHandler.currentMacroConnect.reset();
  });
}

}  // namespace inputs
}  // namespace dfl
