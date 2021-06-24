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
#include "Message.hpp"

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

template<class T>
static void
parserFile(const boost::filesystem::path& filepath, const parser::ParserFactory& factory, T& element) {
  auto parser = factory.createParser();
  bool xsdValidation = true;

  std::ifstream in(filepath.c_str());
  if (!in) {
    // only a warning here because not providing an assembling or setting file is an expected behaviour for some simulations
    LOG(warn) << MESS(AutomatonFileNotFound, filepath.c_str()) << LOG_ENDL;
    return;
  }

  auto xsd = computeXsdPath<T>(filepath);
  if (xsd.empty()) {
    LOG(warn) << MESS(AutomatonFileXSDNotFound, filepath.c_str()) << LOG_ENDL;
    xsdValidation = false;
  } else {
    parser->addXmlSchema(xsd.generic_string());
  }

  try {
    parser->parse(in, element, xsdValidation);
  } catch (const xml::sax::parser::ParserException& e) {
    LOG(error) << MESS(AutomatonFileReadError, filepath, e.what()) << LOG_ENDL;
    std::exit(EXIT_FAILURE);
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
