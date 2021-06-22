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
 * @file SettingsXmlDocument.h
 * @brief setting xml document handler header
 */

#pragma once

#include <boost/optional.hpp>
#include <string>
#include <vector>
#include <xml/sax/parser/Attributes.h>
#include <xml/sax/parser/ComposableDocumentHandler.h>
#include <xml/sax/parser/ComposableElementHandler.h>

namespace dfl {
namespace inputs {

/**
 * @brief Setting document handler
 *
 * XML document handler for the setting file for automatons
 */
class SettingsXmlDocument : public xml::sax::parser::ComposableDocumentHandler {
 public:
  /**
   * @brief Parameter element
   */
  template<class T>
  struct Parameter {
    std::string name;  ///< name of the parameter
    T value;           ///< value of the parameter
  };

  /**
   * @brief Ref element
   */
  struct Ref {
    std::string id;    ///< id of the element
    std::string name;  ///< name of the ref
    std::string tag;   ///< tag of the ref
  };

  /**
   * @brief Reference element
   */
  struct Reference {
    /// @brief Type of data in the reference
    enum class DataType {
      DOUBLE = 0,  ///< Double type
      INT,         ///< Interger type
      BOOL,        ///< Boolean type
      STRING       ///< String type
    };
    // Data origin is always IIDM
    boost::optional<std::string> componentId;  ///< referenced component id
    std::string name;                          ///< name of the reference
    std::string origName;                      ///< origin of the name
    DataType dataType = DataType::DOUBLE;      ///< the data type of the reference

    /**
     * @brief Convert data type into its string representation
     * @param type the type to convert
     * @returns the string representation of the type
     */
    static std::string toString(DataType type);
  };

  /**
   * @brief Count element
   */
  struct Count {
    std::string name;  ///< name of the count
    std::string id;    ///< id
  };

  /**
   * @brief Set element
   */
  struct Set {
    std::string id;                                        ///< id of the set
    std::vector<Count> count;                              ///< list of the counts
    std::vector<Ref> refs;                                 ///< list of the refs
    std::vector<Reference> references;                     ///< list of the references
    std::vector<Parameter<double>> doubleParameters;       ///< list of the double parameters
    std::vector<Parameter<bool>> boolParameters;           ///< list of the boolean parameters
    std::vector<Parameter<int>> integerParameters;         ///< list of the integer parameters
    std::vector<Parameter<std::string>> stringParameters;  ///< list of the string parameters
  };

 public:
  /// @brief Default constructor
  SettingsXmlDocument();

  /**
   * @brief Retrieve the list of the sets
   * @returns the sets defined by the xml file
   */
  const std::vector<Set>& sets() const {
    return sets_;
  }

 private:
  /**
   * @brief Parameter element handler
   *
   * Boolean, integer and double parameters are exclusive for a same element
   */
  struct ParameterHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root root element to parse
     */
    explicit ParameterHandler(const elementName_type& root);

    boost::optional<Parameter<bool>> currentBoolParameter;           ///< current boolean parameters
    boost::optional<Parameter<double>> currentDoubleParameter;       ///< current double parameter
    boost::optional<Parameter<int>> currentIntegerParameter;         ///< current integer parameter
    boost::optional<Parameter<std::string>> currentStringParameter;  ///< current string parameter
  };

  /// @brief Ref element handler
  struct RefHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root root element to parse
     */
    explicit RefHandler(const elementName_type& root);

    boost::optional<Ref> currentRef;  ///< current ref element
  };

  /// @brief Reference element handler
  struct ReferenceHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root root element to parse
     */
    explicit ReferenceHandler(const elementName_type& root);

    boost::optional<Reference> currentReference;  ///< current reference handler
  };

  /// @brief Count element handler
  class CountHandler : public xml::sax::parser::ComposableElementHandler {
   public:
    /**
     * @brief Constructor
     * @param root root element to parse
     */
    explicit CountHandler(const elementName_type& root);

    boost::optional<Count> currentCount;  ///< Current count

   private:
    /**
     * @brief Checks that the count name is supported
     * @param name the name to check
     * @returns true if the name is supported, false if not
     */
    static bool check(const std::string& name);
  };

  /// @brief Set element handler
  struct SetHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root root element to parse
     */
    explicit SetHandler(const elementName_type& root);

    boost::optional<Set> currentSet;  ///< current set element

    CountHandler countHandler;          ///< count element handler
    RefHandler refHandler;              ///< ref element handler
    ReferenceHandler referenceHandler;  ///< reference handler
    ParameterHandler parameterHandler;  ///< parameter handler
  };

 private:
  /**
   * @brief Create optional parameter
   * @param param the parameter to update
   * @param attributes the xml attributes to use to create the parameter
   */
  template<class T>
  static void createOptionalParameter(boost::optional<Parameter<T>>& param, const attributes_type& attributes) {
    param = Parameter<T>();
    param->value = attributes["value"].as<T>();
    param->name = attributes["name"].as_string();
  }

 private:
  SetHandler setHandler_;  ///< Set handler
  std::vector<Set> sets_;  ///< list of the sets
};

}  // namespace inputs

}  // namespace dfl
