//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file Dico.cpp
 * @brief Dictionary manager implementation file
 */

#include "Dico.h"

#include "Log.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace dfl {
namespace common {

std::string Dico::filepath_;

Dico&
Dico::instance() {
  static Dico dico(filepath_);
  return dico;
}

std::string
Dico::message(generated::DicoKeys::Key key) const {
  if (messages_.count(key) > 0) {
    return messages_.at(key);
  } else {
    LOG(debug) << "Unknown dictionnary key " << generated::DicoKeys::keyToString(key) << " : check input dictionnary" << LOG_ENDL;
    return "";
  }
}

void
Dico::configure(const std::string& filepath) {
  filepath_ = filepath;
}

Dico::Dico(const std::string& filepath) : messages_{} {
  std::ifstream is(filepath);
  std::string line;

  while (std::getline(is, line)) {
    size_t index = line.find_first_of('=');
    if (index != std::string::npos) {
      auto key = line.substr(0, index);
      key.erase(std::remove_if(key.begin(), key.end(), [](char c) { return std::isspace(c) != 0; }), key.end());

      size_t first_message_index = line.find_first_not_of(' ', index + 1);
      auto message = line.substr(first_message_index);

      if (generated::DicoKeys::canConvertString(key)) {
        messages_.insert(std::make_pair(generated::DicoKeys::stringToKey(key), message));
      } else {
        // We cannot use a log level higher than "debug" because log level higher than "debug" shall use dictionnaries
        LOG(debug) << "Key string " << key << " is not defined in list of generated keys: check your input file " << filepath << LOG_ENDL;
      }
    }
  }
}

}  // namespace common
}  // namespace dfl
