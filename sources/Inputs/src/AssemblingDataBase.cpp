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
 * @file AssemblingDataBase.cpp
 * @brief bases structures to represent the assembling
 */

#include "AssemblingDataBase.h"

#include "Log.h"

#include <xml/sax/parser/ParserFactory.h>

namespace parser = xml::sax::parser;
namespace file = boost::filesystem;

namespace dfl {
namespace inputs {

/**
 * @brief retrieve the xsd for a xml file
 *
 * @param filepath xml file path
 * @return the corresponding xsd filepath or an empty path if not found
 */
static file::path
computeXsdPath(const file::path& filepath) {
  auto basename = filepath.filename().replace_extension().generic_string();

  auto var = getenv("DYNAFLOW_LAUNCHER_XSD");
  file::path xsdFile;
  if (var != NULL) {
    xsdFile = file::path(var);
  } else {
    xsdFile = file::current_path();
  }

  xsdFile.append("assembling.xsd");

  if (!file::exists(xsdFile)) {
    return file::path("");
  }

  return xsdFile;
}

AssemblingDataBase::AssemblingDataBase(const boost::filesystem::path& assemblingFilePath) {
  parser::ParserFactory factory;
  AssemblingXmlDocument assemblingXml(*this);
  auto parser = factory.createParser();
  bool xsdValidation = true;

  std::ifstream in(assemblingFilePath.c_str());
  if (!in) {
    // only a warning here because not providing an assembling or setting file is an expected behaviour for some simulations
    LOG(warn, DynModelFileNotFound, assemblingFilePath.c_str());
    return;
  }

  auto xsd = computeXsdPath(assemblingFilePath);
  if (xsd.empty()) {
    LOG(warn, DynModelFileXSDNotFound, assemblingFilePath.c_str());
    xsdValidation = false;
  } else {
    parser->addXmlSchema(xsd.generic_string());
  }

  try {
    parser->parse(in, assemblingXml, xsdValidation);
  } catch (const xml::sax::parser::ParserException& e) {
    throw Error(DynModelFileReadError, assemblingFilePath.c_str(), e.what());
  }
}

/**
 * @brief Retrieve a macro connections with its id
 * @param id macro connection id
 * @returns the macro connection with the given id, throw if not found
 */
const AssemblingDataBase::MacroConnection&
AssemblingDataBase::getMacroConnection(const std::string& id) const {
  const auto it = macroConnections_.find(id);
  if (it != macroConnections_.end())
    return it->second;
  throw Error(UnknownMacroConnection, id);
}

/**
 * @brief Retrieve a single association with its id
 * @param id single association id
 * @returns the single associations element with the given id, throw if not found
 */
const AssemblingDataBase::SingleAssociation&
AssemblingDataBase::getSingleAssociation(const std::string& id) const {
  const auto it = singleAssociations_.find(id);
  if (it != singleAssociations_.end())
    return it->second;
  throw Error(UnknownSingleAssoc, id);
}

bool
AssemblingDataBase::isSingleAssociation(const std::string& id) const {
  return singleAssociations_.find(id) != singleAssociations_.end();
}

const AssemblingDataBase::MultipleAssociation&
AssemblingDataBase::getMultipleAssociation(const std::string& id) const {
  const auto it = multipleAssociations_.find(id);
  if (it != multipleAssociations_.end())
    return it->second;
  throw Error(UnknownMultiAssoc, id);
}

bool
AssemblingDataBase::isMultipleAssociation(const std::string& id) const {
  return multipleAssociations_.find(id) != multipleAssociations_.end();
}

/**
 * @return namespace used to read xml file
 */
static parser::namespace_uri ns("");

AssemblingDataBase::AssemblingXmlDocument::AssemblingXmlDocument(AssemblingDataBase& db) :
    macroConnectionHandler_(parser::ElementName(ns, "macroConnection")),
    singleAssociationHandler_(parser::ElementName(ns, "singleAssociation")),
    multipleAssociationHandler_(parser::ElementName(ns, "multipleAssociation")),
    dynamicAutomatonHandler_(parser::ElementName(ns, "dynamicAutomaton")) {
  onElement(ns("assembling/macroConnection"), macroConnectionHandler_);
  onElement(ns("assembling/singleAssociation"), singleAssociationHandler_);
  onElement(ns("assembling/multipleAssociation"), multipleAssociationHandler_);
  onElement(ns("assembling/dynamicAutomaton"), dynamicAutomatonHandler_);

  macroConnectionHandler_.onStart([this]() { macroConnectionHandler_.currentMacroConnection = AssemblingDataBase::MacroConnection(); });
  macroConnectionHandler_.onEnd([this, &db]() {
    db.macroConnections_[macroConnectionHandler_.currentMacroConnection->id] = *macroConnectionHandler_.currentMacroConnection;
    macroConnectionHandler_.currentMacroConnection.reset();
  });

  singleAssociationHandler_.onStart([this]() { singleAssociationHandler_.currentSingleAssociation = AssemblingDataBase::SingleAssociation(); });
  singleAssociationHandler_.onEnd([this, &db]() {
    db.singleAssociations_[singleAssociationHandler_.currentSingleAssociation->id] = *singleAssociationHandler_.currentSingleAssociation;
    singleAssociationHandler_.currentSingleAssociation.reset();
  });

  multipleAssociationHandler_.onStart([this]() { multipleAssociationHandler_.currentMultipleAssociation = AssemblingDataBase::MultipleAssociation(); });
  multipleAssociationHandler_.onEnd([this, &db]() {
    db.multipleAssociations_[multipleAssociationHandler_.currentMultipleAssociation->id] = *multipleAssociationHandler_.currentMultipleAssociation;
    multipleAssociationHandler_.currentMultipleAssociation.reset();
  });

  dynamicAutomatonHandler_.onStart([this]() { dynamicAutomatonHandler_.currentDynamicAutomaton = AssemblingDataBase::DynamicAutomaton(); });
  dynamicAutomatonHandler_.onEnd([this, &db]() {
    db.dynamicAutomatons_[dynamicAutomatonHandler_.currentDynamicAutomaton->id] = *dynamicAutomatonHandler_.currentDynamicAutomaton;
    dynamicAutomatonHandler_.currentDynamicAutomaton.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::ConnectionHandler::ConnectionHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentConnection->var1 = attributes["var1"].as_string();
    currentConnection->var2 = attributes["var2"].as_string();
  });
}

AssemblingDataBase::AssemblingXmlDocument::BusHandler::BusHandler(const elementName_type& root) {
  onStartElement(root,
                 [this](const parser::ElementName&, const attributes_type& attributes) { currentBus->voltageLevel = attributes["voltageLevel"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::ShuntHandler::ShuntHandler(const elementName_type& root) {
  onStartElement(
      root, [this](const parser::ElementName&, const attributes_type& attributes) { currentShunt->voltageLevel = attributes["voltageLevel"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::TfoHandler::TfoHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentTfo->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::LineHandler::LineHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentLine->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::MacroConnectHandler::MacroConnectHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentMacroConnect->id = attributes["id"].as_string();
    currentMacroConnect->macroConnection = attributes["macroConnection"].as_string();
  });
}

AssemblingDataBase::AssemblingXmlDocument::MacroConnectionHandler::MacroConnectionHandler(const elementName_type& root) :
    connectionHandler(parser::ElementName(ns, "connection")) {
  onElement(root + ns("connection"), connectionHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentMacroConnection->id = attributes["id"].as_string(); });

  connectionHandler.onStart([this]() { connectionHandler.currentConnection = AssemblingDataBase::Connection(); });
  connectionHandler.onEnd([this]() {
    currentMacroConnection->connections.push_back(*connectionHandler.currentConnection);
    connectionHandler.currentConnection.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::SingleAssociationHandler::SingleAssociationHandler(const elementName_type& root) :
    busHandler(parser::ElementName(ns, "bus")),
    lineHandler(parser::ElementName(ns, "line")),
    tfoHandler(parser::ElementName(ns, "tfo")) {
  onElement(root + ns("bus"), busHandler);
  onElement(root + ns("line"), lineHandler);
  onElement(root + ns("tfo"), tfoHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentSingleAssociation->id = attributes["id"].as_string(); });

  busHandler.onStart([this]() { busHandler.currentBus = AssemblingDataBase::Bus(); });
  busHandler.onEnd([this]() {
    currentSingleAssociation->bus = busHandler.currentBus;
    busHandler.currentBus.reset();
  });

  lineHandler.onStart([this]() { lineHandler.currentLine = AssemblingDataBase::Line(); });
  lineHandler.onEnd([this]() {
    currentSingleAssociation->line = lineHandler.currentLine;
    lineHandler.currentLine.reset();
  });

  tfoHandler.onStart([this]() { tfoHandler.currentTfo = AssemblingDataBase::Tfo(); });
  tfoHandler.onEnd([this]() {
    currentSingleAssociation->tfo = tfoHandler.currentTfo;
    tfoHandler.currentTfo.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::MultipleAssociationHandler::MultipleAssociationHandler(const elementName_type& root) :
    shuntHandler(parser::ElementName(ns, "shunt")) {
  onElement(root + ns("shunt"), shuntHandler);

  onStartElement(root,
                 [this](const parser::ElementName&, const attributes_type& attributes) { currentMultipleAssociation->id = attributes["id"].as_string(); });

  shuntHandler.onStart([this]() { shuntHandler.currentShunt = AssemblingDataBase::Shunt(); });
  shuntHandler.onEnd([this]() {
    currentMultipleAssociation->shunt = *shuntHandler.currentShunt;
    shuntHandler.currentShunt.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::DynamicAutomatonHandler::DynamicAutomatonHandler(const elementName_type& root) :
    macroConnectHandler(parser::ElementName(ns, "macroConnect")) {
  onElement(root + ns("macroConnect"), macroConnectHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentDynamicAutomaton->id = attributes["id"].as_string();
    currentDynamicAutomaton->lib = attributes["lib"].as_string();
  });

  macroConnectHandler.onStart([this]() { macroConnectHandler.currentMacroConnect = MacroConnect(); });
  macroConnectHandler.onEnd([this]() {
    currentDynamicAutomaton->macroConnects.push_back(*macroConnectHandler.currentMacroConnect);
    macroConnectHandler.currentMacroConnect.reset();
  });
}
}  // namespace inputs
}  // namespace dfl