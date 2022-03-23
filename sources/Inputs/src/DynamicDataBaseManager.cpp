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
 * @file  DynamicDataBaseManager.cpp
 *
 * @brief Automaton manager implementation file
 *
 */

#include "DynamicDataBaseManager.h"

#include "Log.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <xml/sax/parser/ComposableDocumentHandler.h>
#include <xml/sax/parser/Parser.h>
#include <xml/sax/parser/ParserException.h>
#include <xml/sax/parser/ParserFactory.h>

namespace parser = xml::sax::parser;
namespace file = boost::filesystem;

namespace dfl {
namespace inputs {

namespace helper {

/**
 * @brief Trait class for xsd filename by type
 */
template<class T>
struct XsdTrait {
  static const std::string filename;  ///< filename trait definition, to specialize
};

template<>
const std::string XsdTrait<AssemblingXmlDocument>::filename("assembling.xsd");  ///< XSD filename for assembling xml document
template<>
const std::string XsdTrait<SettingXmlDocument>::filename("setting.xsd");  ///< XSD filename for setting xml document

/**
 * @brief retrieve the xsd for a xml file
 *
 * @param filepath xml file path
 * @return the corresponding xsd filepath or an empty path if not found
 */
template<class T>
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

  xsdFile.append(XsdTrait<T>::filename);

  if (!file::exists(xsdFile)) {
    return file::path("");
  }

  return xsdFile;
}

/**
 * @brief Parse an xml file and fill the associated element
 *
 * @param filepath xml file path
 * @param factory Parser factory
 * @param element Element associated to the target file
 */
template<class T>
static void
parserFile(const boost::filesystem::path& filepath, const parser::ParserFactory& factory, T& element) {
  auto parser = factory.createParser();
  bool xsdValidation = true;

  std::ifstream in(filepath.c_str());
  if (!in) {
    // only a warning here because not providing an assembling or setting file is an expected behaviour for some simulations
    LOG(warn, DynModelFileNotFound, filepath.c_str());
    return;
  }

  auto xsd = computeXsdPath<T>(filepath);
  if (xsd.empty()) {
    LOG(warn, DynModelFileXSDNotFound, filepath.c_str());
    xsdValidation = false;
  } else {
    parser->addXmlSchema(xsd.generic_string());
  }

  try {
    parser->parse(in, element, xsdValidation);
  } catch (const xml::sax::parser::ParserException& e) {
    throw Error(DynModelFileReadError, filepath.c_str(), e.what());
  }
}
}  // namespace helper

DynamicDataBaseManager::DynamicDataBaseManager(const boost::filesystem::path& settingFilePath, const boost::filesystem::path& assemblingFilePath) {
  parser::ParserFactory factory;
  helper::parserFile(settingFilePath, factory, settingDoc_);
  helper::parserFile(assemblingFilePath, factory, assemblingDoc_);
}

}  // namespace inputs

}  // namespace dfl
