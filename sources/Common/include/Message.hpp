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
 * @file Message.hpp
 * @brief Dictionary message file
 */

#pragma once

#include "Dico.h"
#include "DicoKeys.h"

#include <boost/format.hpp>
#include <sstream>
#include <vector>

namespace dfl {
namespace common {

/**
 * @brief Message formatter
 *
 * Message linking a dico key with its formatted string, filled with external data
 *
 */
class Message {
 public:
  /**
   * @brief Constructor
   *
   * Build a message from a list of argument. This will format the message string according to the arguments.
   * see boost::format documentation for the behaviour if the number of arguments doesn't match the format.
   *
   * In case the key is unknown (dictionnary not loaded or not compliant with current code), a default format is displayed:
   * KEY ARG1 ARG2 ...
   *
   * where KEY is the string repsentation of the key and Argx the raw arguments in the order
   *
   * @param key the message key
   * @param args pack of arguments
   */
  template<class... Args>
  Message(generated::DicoKeys::Key key, Args... args) : str_{} {
    std::string str = dico().message(key);

    if (str.empty()) {
      // case key is unknown: default message
      std::stringstream ss;
      ss << generated::DicoKeys::keyToString(key) + ' ';
      updateStr(ss, args...);
      str_ = ss.str();
    } else {
      // case key is known: we use the formatter
      boost::format fmter(str);
      updateFormatter(fmter, args...);
      str_ = fmter.str();
    }
  }

  /**
   * @brief Retrieves the formatted message
   *
   * @returns the formatted message
   */
  const std::string& str() const {
    return str_;
  }

 private:
  /**
   * @brief Formatter generic updater function
   *
   * This function implements a recursive template functions pattern to iterate on the arguments of multiple types
   *
   * @param fmter the formatter to update
   * @param arg the argument to process
   * @param args the rest of the arguments
   */
  template<class Arg, class... Args>
  static void updateFormatter(boost::format& fmter, Arg arg, Args... args) {
    fmter % arg;
    updateFormatter(fmter, args...);
  }

  /**
   * @brief Formatter single updater function
   *
   * Update the formatter in case of a single argument. Corresponds to the stop case of the recursive template algorithm.
   *
   * @param fmter the formatter to update
   * @param arg the argument to process
   */
  template<class Arg>
  static void updateFormatter(boost::format& fmter, Arg arg) {
    fmter % arg;
  }

  /**
   * @brief stream generic updater function
   *
   * This function implements a recursive template functions pattern to iterate on the arguments of multiple types
   *
   * @param os the stream to update
   * @param arg the argument to process
   * @param args the rest of the arguments
   */
  template<class Arg, class... Args>
  static void updateStr(std::ostream& os, Arg arg, Args... args) {
    os << arg << ' ';
    updateStr(os, args...);
  }

  /**
   * @brief stream single updater function
   *
   * Update the stream in case of a single argument. Corresponds to the stop case of the recursive template algorithm.
   *
   * @param os the stream to update
   * @param arg the argument to process
   */
  template<class Arg>
  static void updateStr(std::ostream& os, Arg arg) {
    os << arg;
  }

 private:
  std::string str_;  ///< the formatted message
};

}  // namespace common
}  // namespace dfl

/**
 * @brief Macro to easy build a message
 *
 * @param key the dico key (without namespaces and type)
 * @param ... Arguments required to format the corresponding message
 */
#define MESS(key, ...) dfl::common::Message(dfl::common::generated::DicoKeys::Key::key, __VA_ARGS__).str()
