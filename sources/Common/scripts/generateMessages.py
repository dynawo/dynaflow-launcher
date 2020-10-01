# -*- coding: utf-8;

# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

import sys
import re
import os

##
# @brief Dictionnary definition for message keys
#


class Dictionnary:

    ##
    # @brief the regex for dictionnary pair definition
    message_def_pattern = re.compile(r"\s*(?P<key>\w+)\s*=\s*(?P<message>.*)")

    ##
    # @brief Write common header to all files
    # @param file : the file to write into
    @staticmethod
    def _writeHeaderAll(file):
        file.write('''//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//
''')

    ##
    # @brief Write header file code
    # @param self : object pointer
    # @param file : the file to write into
    def _writeHeader(self, file):
        file.write("#pragma once\n\n")
        file.write("#include <unordered_map>\n")
        file.write("#include <string>\n\n")
        file.write("namespace dfl {\n")
        file.write("namespace common {\n")
        file.write("/// @brief namespace for generated code\n")
        file.write("namespace generated {\n\n")

        file.write("/// @brief Dictionnary keys\n")
        file.write("class DicoKeys {\n")
        file.write(" public:\n")

        file.write("  /// @brief Dictionnary keys definition\n")
        file.write("  enum class Key {\n")
        for mess in self.messages:
            file.write("    " + mess["key"] +
                       ",  ///< " + mess["message"] + "\n")
        file.write("  };\n")

        file.write("  /**\n")
        file.write(
            "   * @brief Determines if the string corresponds to a message key\n")
        file.write("   * @param str the string to test\n")
        file.write(
            "   * @returns whether the string corresponds to a message key\n")
        file.write("   */\n")
        file.write(
            "  static bool canConvertString(const std::string& str) { return keys_.count(str) > 0; }\n")
        file.write("  /**\n")
        file.write("   * @brief Convert a string into a message key\n")
        file.write("   *\n")
        file.write("   * precondition: the string can be converted into a key\n")
        file.write("   * @param str the string to convert\n")
        file.write("   * @returns the converted key\n")
        file.write("   */\n")
        file.write(
            "  static const Key& stringToKey(const std::string& str) { return keys_.at(str); }\n")

        file.write("  /**\n")
        file.write("   * @brief Convert a key into its string representation\n")
        file.write("   * @param key the key to convert\n")
        file.write("   * @returns the string representation of the key\n")
        file.write("   */\n")
        file.write(
            "  static const std::string& keyToString(const Key& key);\n\n")

        file.write(" private:\n")
        file.write(
            "  static const std::unordered_map<std::string, Key> keys_;  ///< The mapping of the keys\n")

        file.write("};\n")

        file.write("}  // namespace generated\n")
        file.write("}  // namespace common\n")
        file.write("}  // namespace dfl\n")

    ##
    # @brief Write implementation file code
    # @param self : object pointer
    # @param file : the file to write into
    def _writeSource(self, file):
        file.write('#include "DicoKeys.h"\n\n')
        file.write("#include <algorithm>\n\n")
        file.write("namespace dfl {\n")
        file.write("namespace common {\n")
        file.write("namespace generated {\n\n")

        file.write(
            "const std::unordered_map<std::string, DicoKeys::Key> DicoKeys::keys_{\n")
        for mess in self.messages:
            file.write(
                "    std::make_pair(" + '"' + mess["key"] + '"' + ", " + "Key::" + mess["key"] + "),\n")
        file.write("};\n\n")

        file.write(
            "const std::string& DicoKeys::keyToString(const DicoKeys::Key& key) {\n")
        file.write(
            "  auto found = std::find_if(keys_.begin(), keys_.end(), [&key](const std::pair<std::string, DicoKeys::Key>& pair){ return pair.second == key; });\n")
        file.write('  return found->first;\n')
        file.write("}\n\n")

        file.write("}  // namespace generated\n")
        file.write("}  // namespace common\n")
        file.write("}  // namespace dfl\n")

    # @brief Parse dictionnary definition file
    # @param self : object pointer
    # @param input : the input filepath of the file ot read
    def _parse(self, input):
        with open(input, 'r') as file:
            lines = file.readlines()
            for line in lines:
                match = Dictionnary.message_def_pattern.match(line)
                if match != None:
                    self.messages.append({
                        "key": match.group("key"),
                        "message": match.group("message")
                    })

    ##
    # @brief Constructor
    # @param self : object pointer
    # @param input : the input filepath of the file ot read
    def __init__(self, input):
        ##
        #  List of pairs defining a message
        self.messages = []
        self._parse(input)

    ##
    # @brief Write header file code
    # @param self : object pointer
    # @param outputFile : the filepath of the file to write into
    def generate_header(self, outputFile):
        with open(outputFile, 'w') as file:
            Dictionnary._writeHeaderAll(file)
            self._writeHeader(file)

    ##
    # @brief Write implementation file code
    # @param self : object pointer
    # @param outputFile : the filepath of the file to write into
    def generate_source(self, outputFile):
        with open(outputFile, 'w') as file:
            Dictionnary._writeHeaderAll(file)
            self._writeSource(file)

if __name__ == "__main__":
    inputDic = sys.argv[1]
    outputDir = sys.argv[2]

    dict = Dictionnary(inputDic)

    dict.generate_header(os.path.join(outputDir, "include/DicoKeys.h"))
    dict.generate_source(os.path.join(outputDir, "src/DicoKeys.cpp"))
