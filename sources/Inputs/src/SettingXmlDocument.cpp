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
 * @file SettingXmlDocument.cpp
 * @brief Setting xml document handler implementation
 */

#include "SettingXmlDocument.h"

#include "Log.h"
#include "Message.hpp"

#include <vector>

namespace parser = xml::sax::parser;

namespace dfl {
namespace inputs {

static parser::namespace_uri ns("");

const std::string SettingXmlDocument::origData_("IIDM");

/**
 * @brief Specialization for string of @a createOptionalParameter
 * @param param the parameter to update
 * @param attributes the xml attributes to use to create the parameter
 */
template<>
void
SettingXmlDocument::createOptionalParameter<std::string>(boost::optional<Parameter<std::string>>& param, const attributes_type& attributes) {
  param = Parameter<std::string>();
  param->value = attributes["value"].as_string();
  param->name = attributes["name"].as_string();
}

SettingXmlDocument::SettingXmlDocument() : setHandler_(parser::ElementName(ns, "set")) {
  onElement(ns("setting/set"), setHandler_);

  setHandler_.onStart([this]() { setHandler_.currentSet = Set(); });
  setHandler_.onEnd([this]() {
    sets_.push_back(*setHandler_.currentSet);
    setHandler_.currentSet.reset();
  });
}

SettingXmlDocument::SetHandler::SetHandler(const elementName_type& root) :
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
SettingXmlDocument::CountHandler::check(const std::string& name) {
  static const std::vector<std::string> keys = {"nbShunts", "nbShuntsLV", "nbShuntsHV"};

  return std::find(keys.begin(), keys.end(), name) != keys.end();
}

SettingXmlDocument::CountHandler::CountHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentCount->id = attributes["id"].as_string();
    const auto& name = attributes["name"].as_string();
    if (!check(name)) {
      throw std::runtime_error(MESS(UnsupportedCountName, name));
    }
    currentCount->name = name;
  });
}

SettingXmlDocument::RefHandler::RefHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    currentRef->id = attributes["id"].as_string();
    currentRef->name = attributes["name"].as_string();
    currentRef->tag = attributes["tag"].as_string();
    // Type is not extracted because it is not needed but it is kept in xsd for future developments (see setting.xsd)
  });
}

SettingXmlDocument::ReferenceHandler::ReferenceHandler(const elementName_type& root) {
  onStartElement(root, [this](const parser::ElementName&, const attributes_type& attributes) {
    if (attributes.has("componentId")) {
      currentReference->componentId = attributes["componentId"].as_string();
    }
    currentReference->name = attributes["name"].as_string();
    currentReference->origName = attributes["origName"].as_string();

    if (attributes["origData"].as_string() != origData_) {
      throw std::runtime_error(MESS(UnsupportedOrigDataReference, attributes["origData"].as_string(), currentReference->name));
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
      throw std::runtime_error(MESS(UnsupportedDataTypeReference, type, currentReference->name));
    }
  });
}

SettingXmlDocument::ParameterHandler::ParameterHandler(const elementName_type& root) {
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
      throw std::runtime_error(MESS(UnsupportedParameterDataType, type));
    }
  });
}

std::string
SettingXmlDocument::Reference::toString(DataType type) {
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
