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

#include "Constants.h"
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
static file::path computeXsdPath(const file::path &filepath) {
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

AssemblingDataBase::AssemblingDataBase(const boost::filesystem::path &assemblingFilePath) : containsSVC_(false) {
  parser::ParserFactory factory;
  AssemblingXmlDocument assemblingXml(*this);
  auto parser = factory.createParser();
  bool xsdValidation = true;

  std::ifstream in(assemblingFilePath.c_str());
  if (!in) {
    // only a warning here because not providing an assembling or setting file is an expected behaviour for some simulations
    if (!assemblingFilePath.empty())
      LOG(warn, DynModelFileNotFound, assemblingFilePath.generic_string());
    return;
  }

  auto xsd = computeXsdPath(assemblingFilePath);
  if (xsd.empty()) {
    LOG(warn, DynModelFileXSDNotFound, assemblingFilePath.generic_string());
    xsdValidation = false;
  } else {
    parser->addXmlSchema(xsd.generic_string());
  }

  try {
    parser->parse(in, assemblingXml, xsdValidation);
  } catch (const xml::sax::parser::ParserException &e) {
    throw Error(DynModelFileReadError, assemblingFilePath.generic_string(), e.what());
  }
}

const AssemblingDataBase::MacroConnection &AssemblingDataBase::getMacroConnection(const std::string &id, bool network) const {
  if (network) {
    const auto it = networkMacroConnections_.find(id);
    if (it != networkMacroConnections_.end())
      return it->second;
  }
  const auto it = macroConnections_.find(id);
  if (it != macroConnections_.end())
    return it->second;
  throw Error(UnknownMacroConnection, id);
}

bool AssemblingDataBase::hasNetworkMacroConnection(const std::string &id) const { return networkMacroConnections_.find(id) != networkMacroConnections_.end(); }

const AssemblingDataBase::SingleAssociation &AssemblingDataBase::getSingleAssociation(const std::string &id) const {
  const auto it = singleAssociations_.find(id);
  if (it != singleAssociations_.end())
    return it->second;
  throw Error(UnknownSingleAssoc, id);
}

bool AssemblingDataBase::isSingleAssociation(const std::string &id) const { return singleAssociations_.find(id) != singleAssociations_.end(); }

const AssemblingDataBase::MultipleAssociation &AssemblingDataBase::getMultipleAssociation(const std::string &id) const {
  const auto it = multipleAssociations_.find(id);
  if (it != multipleAssociations_.end())
    return it->second;
  throw Error(UnknownMultiAssoc, id);
}

std::string AssemblingDataBase::getSingleAssociationFromGenerator(const std::string &name) const {
  const auto it = generatorIdToSingleAssociationsId_.find(name);
  if (it != generatorIdToSingleAssociationsId_.end()) {
    return it->second;
  }
  return "";
}

std::string AssemblingDataBase::getSingleAssociationFromHvdcLine(const std::string &name) const {
  const auto it = HvdcIdToSingleAssociationsId_.find(name);
  if (it != HvdcIdToSingleAssociationsId_.end()) {
    return it->second;
  }
  return "";
}

bool AssemblingDataBase::isMultipleAssociation(const std::string &id) const { return multipleAssociations_.find(id) != multipleAssociations_.end(); }

const AssemblingDataBase::Property &AssemblingDataBase::getProperty(const std::string &id) const {
  const auto it = properties_.find(id);
  if (it != properties_.end())
    return it->second;
  throw Error(UnknownProperty, id);
}

bool AssemblingDataBase::isProperty(const std::string &id) const { return properties_.find(id) != properties_.end(); }

/**
 * @return namespace used to read xml file
 */
static parser::namespace_uri ns("");

AssemblingDataBase::AssemblingXmlDocument::AssemblingXmlDocument(AssemblingDataBase &db)
    : macroConnectionHandler_(parser::ElementName(ns, "macroConnection")), singleAssociationHandler_(parser::ElementName(ns, "singleAssociation")),
      multipleAssociationHandler_(parser::ElementName(ns, "multipleAssociation")), dynamicAutomatonHandler_(parser::ElementName(ns, "dynamicAutomaton")),
      propertyHandler_(parser::ElementName(ns, "property")) {
  onElement(ns("assembling/macroConnection"), macroConnectionHandler_);
  onElement(ns("assembling/singleAssociation"), singleAssociationHandler_);
  onElement(ns("assembling/multipleAssociation"), multipleAssociationHandler_);
  onElement(ns("assembling/dynamicAutomaton"), dynamicAutomatonHandler_);
  onElement(ns("assembling/property"), propertyHandler_);

  macroConnectionHandler_.onStart([this]() { macroConnectionHandler_.currentMacroConnection = AssemblingDataBase::MacroConnection(); });
  macroConnectionHandler_.onEnd([this, &db]() {
    if (macroConnectionHandler_.currentMacroConnection->network) {
      db.networkMacroConnections_[macroConnectionHandler_.currentMacroConnection->id] = *macroConnectionHandler_.currentMacroConnection;
    } else {
      db.macroConnections_[macroConnectionHandler_.currentMacroConnection->id] = *macroConnectionHandler_.currentMacroConnection;
    }
    macroConnectionHandler_.currentMacroConnection.reset();
  });

  singleAssociationHandler_.onStart([this]() { singleAssociationHandler_.currentSingleAssociation = AssemblingDataBase::SingleAssociation(); });
  singleAssociationHandler_.onEnd([this, &db]() {
    db.singleAssociations_[singleAssociationHandler_.currentSingleAssociation->id] = *singleAssociationHandler_.currentSingleAssociation;
    for (const auto &gen : singleAssociationHandler_.currentSingleAssociation->generators) {
      db.generatorIdToSingleAssociationsId_[gen.name] = singleAssociationHandler_.currentSingleAssociation->id;
    }
    if (singleAssociationHandler_.currentSingleAssociation->hvdcLine)
      db.HvdcIdToSingleAssociationsId_[singleAssociationHandler_.currentSingleAssociation->hvdcLine->name] =
          singleAssociationHandler_.currentSingleAssociation->id;
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
    if ((*dynamicAutomatonHandler_.currentDynamicAutomaton).lib == dfl::common::constants::svcModelName) {
      db.containsSVC_ = true;
    }
    dynamicAutomatonHandler_.currentDynamicAutomaton.reset();
  });

  propertyHandler_.onStart([this]() { propertyHandler_.currentProperty = AssemblingDataBase::Property(); });
  propertyHandler_.onEnd([this, &db]() {
    db.properties_[propertyHandler_.currentProperty->id] = *propertyHandler_.currentProperty;
    propertyHandler_.currentProperty.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::ConnectionHandler::ConnectionHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) {
    currentConnection->var1 = attributes["var1"].as_string();
    currentConnection->var2 = attributes["var2"].as_string();
  });
}

AssemblingDataBase::AssemblingXmlDocument::BusHandler::BusHandler(const elementName_type &root) {
  onStartElement(root,
                 [this](const parser::ElementName &, const attributes_type &attributes) { currentBus->voltageLevel = attributes["voltageLevel"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::MultipleShuntsHandler::MultipleShuntsHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) {
    currentMultipleShunts->voltageLevel = attributes["voltageLevel"].as_string();
  });
}

AssemblingDataBase::AssemblingXmlDocument::GeneratorHandler::GeneratorHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentGenerator->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::LoadHandler::LoadHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentLoad->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::TfoHandler::TfoHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentTfo->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::HvdcLineHandler::HvdcLineHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentHvdcLine->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::SingleShuntHandler::SingleShuntHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentSingleShunt->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::LineHandler::LineHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentLine->name = attributes["name"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::MacroConnectHandler::MacroConnectHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) {
    currentMacroConnect->id = attributes["id"].as_string();
    currentMacroConnect->macroConnection = attributes["macroConnection"].as_string();
    if (attributes.has("mandatory"))
      currentMacroConnect->mandatory = attributes["mandatory"].as<bool>();
  });
}

AssemblingDataBase::AssemblingXmlDocument::MacroConnectionHandler::MacroConnectionHandler(const elementName_type &root)
    : connectionHandler(parser::ElementName(ns, "connection")) {
  onElement(root + ns("connection"), connectionHandler);

  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) {
    currentMacroConnection->id = attributes["id"].as_string();
    if (attributes.has("network"))
      currentMacroConnection->network = attributes["network"].as<bool>();
    if (attributes.has("indexId"))
      currentMacroConnection->indexId = attributes["indexId"].as_string();
  });

  connectionHandler.onStart([this]() { connectionHandler.currentConnection = AssemblingDataBase::Connection(); });
  connectionHandler.onEnd([this]() {
    currentMacroConnection->connections.push_back(*connectionHandler.currentConnection);
    connectionHandler.currentConnection.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::SingleAssociationHandler::SingleAssociationHandler(const elementName_type &root)
    : busHandler(parser::ElementName(ns, "bus")), lineHandler(parser::ElementName(ns, "line")), hvdcLineHandler(parser::ElementName(ns, "hvdcLine")),
      tfoHandler(parser::ElementName(ns, "tfo")), singleShuntHandler(parser::ElementName(ns, "shunt")), generatorHandler(parser::ElementName(ns, "generator")),
      loadHandler(parser::ElementName(ns, "load")) {
  onElement(root + ns("bus"), busHandler);
  onElement(root + ns("line"), lineHandler);
  onElement(root + ns("hvdcLine"), hvdcLineHandler);
  onElement(root + ns("tfo"), tfoHandler);
  onElement(root + ns("shunt"), singleShuntHandler);
  onElement(root + ns("generator"), generatorHandler);
  onElement(root + ns("load"), loadHandler);

  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentSingleAssociation->id = attributes["id"].as_string(); });

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

  hvdcLineHandler.onStart([this]() { hvdcLineHandler.currentHvdcLine = AssemblingDataBase::HvdcLine(); });
  hvdcLineHandler.onEnd([this]() {
    currentSingleAssociation->hvdcLine = hvdcLineHandler.currentHvdcLine;
    hvdcLineHandler.currentHvdcLine.reset();
  });

  tfoHandler.onStart([this]() { tfoHandler.currentTfo = AssemblingDataBase::Tfo(); });
  tfoHandler.onEnd([this]() {
    currentSingleAssociation->tfo = tfoHandler.currentTfo;
    tfoHandler.currentTfo.reset();
  });

  singleShuntHandler.onStart([this]() { singleShuntHandler.currentSingleShunt = AssemblingDataBase::SingleShunt(); });
  singleShuntHandler.onEnd([this]() {
    currentSingleAssociation->shunt = singleShuntHandler.currentSingleShunt;
    singleShuntHandler.currentSingleShunt.reset();
  });

  generatorHandler.onStart([this]() { generatorHandler.currentGenerator = AssemblingDataBase::Generator(); });
  generatorHandler.onEnd([this]() {
    currentSingleAssociation->generators.push_back(*generatorHandler.currentGenerator);
    generatorHandler.currentGenerator.reset();
  });

  loadHandler.onStart([this]() { loadHandler.currentLoad = AssemblingDataBase::Load(); });
  loadHandler.onEnd([this]() {
    currentSingleAssociation->loads.push_back(*loadHandler.currentLoad);
    loadHandler.currentLoad.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::MultipleAssociationHandler::MultipleAssociationHandler(const elementName_type &root)
    : multipleShuntsHandler(parser::ElementName(ns, "shunt")) {
  onElement(root + ns("shunt"), multipleShuntsHandler);

  onStartElement(root,
                 [this](const parser::ElementName &, const attributes_type &attributes) { currentMultipleAssociation->id = attributes["id"].as_string(); });

  multipleShuntsHandler.onStart([this]() { multipleShuntsHandler.currentMultipleShunts = AssemblingDataBase::MultipleShunts(); });
  multipleShuntsHandler.onEnd([this]() {
    currentMultipleAssociation->shunt = *multipleShuntsHandler.currentMultipleShunts;
    multipleShuntsHandler.currentMultipleShunts.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::DynamicAutomatonHandler::DynamicAutomatonHandler(const elementName_type &root)
    : macroConnectHandler(parser::ElementName(ns, "macroConnect")) {
  onElement(root + ns("macroConnect"), macroConnectHandler);

  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) {
    currentDynamicAutomaton->id = attributes["id"].as_string();
    currentDynamicAutomaton->lib = attributes["lib"].as_string();
  });

  macroConnectHandler.onStart([this]() { macroConnectHandler.currentMacroConnect = MacroConnect(); });
  macroConnectHandler.onEnd([this]() {
    currentDynamicAutomaton->macroConnects.push_back(*macroConnectHandler.currentMacroConnect);
    macroConnectHandler.currentMacroConnect.reset();
  });
}

AssemblingDataBase::AssemblingXmlDocument::DeviceHandler::DeviceHandler(const elementName_type &root) {
  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentDevice->id = attributes["id"].as_string(); });
}

AssemblingDataBase::AssemblingXmlDocument::PropertyHandler::PropertyHandler(const elementName_type &root) : deviceHandler(parser::ElementName(ns, "device")) {
  onElement(root + ns("device"), deviceHandler);

  onStartElement(root, [this](const parser::ElementName &, const attributes_type &attributes) { currentProperty->id = attributes["id"].as_string(); });

  deviceHandler.onStart([this]() { deviceHandler.currentDevice = Device(); });
  deviceHandler.onEnd([this]() {
    currentProperty->devices.push_back(*deviceHandler.currentDevice);
    deviceHandler.currentDevice.reset();
  });
}

}  // namespace inputs
}  // namespace dfl
