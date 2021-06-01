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
 * @file  AutomatonConfigurationManager.cpp
 *
 * @brief Automaton manager implementation file
 *
 */

#include "AutomatonConfigurationManager.h"

#include "Log.h"
#include "Message.hpp"

#include <fstream>
#include <iostream>
#include <xml/sax/parser/ComposableDocumentHandler.h>
#include <xml/sax/parser/Parser.h>
#include <xml/sax/parser/ParserException.h>
#include <xml/sax/parser/ParserFactory.h>

namespace parser = xml::sax::parser;

namespace dfl {
namespace inputs {

namespace helper {
template<class T>
static void
parserFile(const std::string& filepath, const parser::ParserFactory& factory, T& element) {
  auto parser = factory.createParser();
  constexpr bool xsdValidation = false;
  // TODO(lecourtoisflo) add xsd validation
  // parser->addXmlSchema();

  std::ifstream in(filepath.c_str());
  if (!in) {
    LOG(warn) << MESS(FileNotFound, filepath) << LOG_ENDL;
    return;
  }
  parser->parse(in, element, xsdValidation);
}
}  // namespace helper

AutomatonConfigurationManager::AutomatonConfigurationManager(const std::string& settingsFilePath, const std::string& assemblyFilePath) {
  parser::ParserFactory factory;
  helper::parserFile(settingsFilePath, factory, settingsDoc_);
  helper::parserFile(assemblyFilePath, factory, assemblyDoc_);
}
}  // namespace inputs

}  // namespace dfl
