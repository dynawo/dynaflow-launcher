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
 * @file Dico.h
 * @brief Dictionary manager header file
 */

#pragma once

#include "DicoKeys.h"

#include <unordered_map>

namespace std {
/**
 * @brief specialization hash for generated keys
 *
 * For old compilers, hashs are not implemented for unordered_map when key is an enum class.
 * see https://en.cppreference.com/w/cpp/utility/hash for template definition
 */
template<>
struct hash<dfl::common::generated::DicoKeys::Key> {
  /// @brief Constructor
  hash() {}
  /**
   * @brief Action operator
   *
   * Performs hash by relying on integer cast for enum class
   *
   * @param key the key to hash
   * @returns the hash value
   */
  size_t operator()(const dfl::common::generated::DicoKeys::Key& key) const {
    return hash<unsigned int>{}(static_cast<unsigned int>(key));
  }
};
}  // namespace std

namespace dfl {
namespace common {

/**
 * @brief Dictionary manager
 *
 * Singleton managing the dictionnary of messages
 */
class Dico {
 public:
  /**
  * @brief Retrieves dictionnary instance
  *
  * @returns the single instance of Dico
  */
  static Dico& instance();

  /**
   * @brief Configure the dictionnary with the filepath to use
   *
   * MUST be called before the first call of instance()
   *
   * @param filepath the file path of the dictionnary definition
   */
  static void configure(const std::string& filepath);

  /**
   * @brief Retrieves the message associated with a key
   *
   * @param key the message key
   * @returns the message associated with the key, or an empty string if not defined
   */
  std::string message(generated::DicoKeys::Key key) const;

 private:
  static std::string filepath_;  ///< The configured filepath

 private:
  /**
   * @brief Constructor
   *
   * constructor is private to implement a singleton pattern
   *
   * @param filepath file path to the dictionnary
   */
  explicit Dico(const std::string& filepath);

 private:
  std::unordered_map<generated::DicoKeys::Key, std::string> messages_;  ///< mapping of the messages
};

/**
 * @brief Retrieves the constant Dico instance
 *
 * @returns the constant instance of Dico
 */
inline const Dico&
dico() {
  return Dico::instance();
}

}  // namespace common
}  // namespace dfl
