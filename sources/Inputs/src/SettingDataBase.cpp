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

#include "SettingDataBase.h"

#include "Log.h"

#include <boost/optional.hpp>
#include <xml/sax/parser/ParserFactory.h>

namespace parser = xml::sax::parser;
namespace file = boost::filesystem;

namespace dfl {
namespace inputs {

/**
 * @return namespace used to read xml file
 */
static parser::namespace_uri ns("");

const std::string SettingDataBase::SettingXmlDocument::origData_("IIDM");

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

  xsdFile.append("setting.xsd");

  if (!file::exists(xsdFile)) {
    return file::path("");
  }

  return xsdFile;
}

SettingDataBase::SettingDataBase(const boost::filesystem::path& settingFilePath) {
  parser::ParserFactory factory;
  SettingXmlDocument settingXml(*this);
  auto parser = factory.createParser();
  bool xsdValidation = true;

  std::ifstream in(settingFilePath.c_str());
  if (!in) {
    // only a warning here because not providing an assembling or setting file is an expected behaviour for some simulations
    if (!settingFilePath.empty())
      LOG(warn, DynModelFileNotFound, settingFilePath.c_str());
    return;
  }

  auto xsd = computeXsdPath(settingFilePath);
  if (xsd.empty()) {
    LOG(warn, DynModelFileXSDNotFound, settingFilePath.c_str());
    xsdValidation = false;
  } else {
    parser->addXmlSchema(xsd.generic_string());
  }

  try {
    parser->parse(in, settingXml, xsdValidation);
  } catch (const xml::sax::parser::ParserException& e) {
    throw Error(DynModelFileReadError, settingFilePath.c_str(), e.what());
  }
}

const SettingDataBase::Set&
SettingDataBase::getSet(const std::string& id) const {
  const auto it = sets_.find(id);
  if (it != sets_.end())
    return it->second;
  throw Error(UnknownParamSet, id);
}

/**
 * @brief Specialization for string of @a createOptionalParameter
 * @param param the parameter to update
 * @param attributes the xml attributes to use to create the parameter
 */
template<>
void
SettingDataBase::SettingXmlDocument::createOptionalParameter<std::string>(boost::optional<Parameter<std::string>>& param, const attributes_type& attributes) {
  param = Parameter<std::string>();
  param->value = attributes["value"].as_string();
  param->name = attributes["name"].as_string();
}

SettingDataBase::SettingXmlDocument::SettingXmlDocument(SettingDataBase& db) : setHandler_(parser::ElementName(ns, "set")) {
  onElement(ns("setting/set"), setHandler_);

  setHandler_.onStart([this]() { setHandler_.currentSet = Set(); });
  setHandler_.onEnd([this, &db]() {
    db.sets_[setHandler_.currentSet->id] = *setHandler_.currentSet;
    setHandler_.currentSet.reset();
  });
}

SettingDataBase::SettingXmlDocument::SetHandler::SetHandler(const elementName_type& root) :
    countHandler(parser::ElementName(ns, "count")),
    refHandler(parser::ElementName(ns, "ref")),
    referenceHandler(parser::ElementName(ns, "reference")),
    parameterHandler(parser::ElementName(ns, "par")) {
  onElement(root + ns("count"), countHandler);
  onElement(root + ns("ref"), refHandler);
  onElement(root + ns("reference"), referenceHandler);
  onElement(root + ns("par"), parameterHandler);

  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) { currentSet->id = attributes["id"].as_string(); });

  countHandler.onStart([this]() { countHandler.currentCount = Count(); });
  countHandler.onEnd([this]() {
    currentSet->counts.push_back(*countHandler.currentCount);
    countHandler.currentCount.reset();
  });

  refHandler.onStart([this]() { refHandler.currentRef = Ref(); });
  refHandler.onEnd([this]() {
    currentSet->refs.push_back(*refHandler.currentRef);
    refHandler.currentRef.reset();
  });

  referenceHandler.onStart([this]() { referenceHandler.currentReference = Reference(); });
  referenceHandler.onEnd([this]() {
    currentSet->references.push_back(*referenceHandler.currentReference);
    referenceHandler.currentReference.reset();
  });

  // parameter optional will be initialized later on according to the type of parameter : no onStart here
  parameterHandler.onEnd([this]() {
    // by construction, one and only one is set
    if (parameterHandler.currentBoolParameter) {
      currentSet->boolParameters.push_back(*parameterHandler.currentBoolParameter);
    } else if (parameterHandler.currentDoubleParameter) {
      currentSet->doubleParameters.push_back(*parameterHandler.currentDoubleParameter);
    } else if (parameterHandler.currentIntegerParameter) {
      currentSet->integerParameters.push_back(*parameterHandler.currentIntegerParameter);
    } else if (parameterHandler.currentStringParameter) {
      currentSet->stringParameters.push_back(*parameterHandler.currentStringParameter);
    } else {
      // Shouldn't happen, an exception should be thrown earlier in the sequence
#if _DEBUG_
      assert(false);
#endif
    }
    parameterHandler.currentBoolParameter.reset();
    parameterHandler.currentDoubleParameter.reset();
    parameterHandler.currentIntegerParameter.reset();
    parameterHandler.currentStringParameter.reset();
  });
}

bool
SettingDataBase::SettingXmlDocument::CountHandler::check(const std::string& name) {
  static const std::vector<std::string> keys = {"nbShunts", "nbShuntsLV", "nbShuntsHV"};

  return std::find(keys.begin(), keys.end(), name) != keys.end();
}

SettingDataBase::SettingXmlDocument::CountHandler::CountHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentCount->id = attributes["id"].as_string();
    const auto& name = attributes["name"].as_string();
    if (!check(name)) {
      throw Error(UnsupportedCountName, name);
    }
    currentCount->name = name;
  });
}

SettingDataBase::SettingXmlDocument::RefHandler::RefHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentRef->id = attributes["id"].as_string();
    currentRef->name = attributes["name"].as_string();
    currentRef->tag = attributes["tag"].as_string();
    // Type is not extracted because it is not needed but it is kept in xsd for future developments (see setting.xsd)
  });
}

SettingDataBase::SettingXmlDocument::ReferenceHandler::ReferenceHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    if (attributes.has("componentId")) {
      currentReference->componentId = attributes["componentId"].as_string();
    }
    currentReference->name = attributes["name"].as_string();
    currentReference->origName = attributes["origName"].as_string();

    if (attributes["origData"].as_string() != origData_) {
      throw Error(UnsupportedOrigDataReference, attributes["origData"].as_string(), currentReference->name);
    }

    auto type = attributes["type"].as_string();
    if (type == "DOUBLE") {
      currentReference->dataType = Reference::DataType::DOUBLE;
    } else if (type == "INT") {
      currentReference->dataType = Reference::DataType::INT;
    } else if (type == "BOOL") {
      currentReference->dataType = Reference::DataType::BOOL;
    } else if (type == "STRING") {
      currentReference->dataType = Reference::DataType::STRING;
    } else {
      throw Error(UnsupportedDataTypeReference, type, currentReference->name);
    }
  });
}

SettingDataBase::SettingXmlDocument::ParameterHandler::ParameterHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    auto type = attributes["type"].as_string();
    if (type == "INT") {
      createOptionalParameter(currentIntegerParameter, attributes);
    } else if (type == "DOUBLE") {
      createOptionalParameter(currentDoubleParameter, attributes);
    } else if (type == "BOOL") {
      createOptionalParameter(currentBoolParameter, attributes);
    } else if (type == "STRING") {
      createOptionalParameter(currentStringParameter, attributes);
    } else {
      throw Error(UnsupportedParameterDataType, type);
    }
  });
}

std::string
SettingDataBase::Reference::toString(DataType type) {
  switch (type) {
  case DataType::DOUBLE:
    return "DOUBLE";
  case DataType::INT:
    return "INT";
  case DataType::BOOL:
    return "BOOL";
  case DataType::STRING:
    return "STRING";
  default:
    //  Impossible case by construction of the enum
    return "";
  }
}
}  // namespace inputs
}  // namespace dfl
