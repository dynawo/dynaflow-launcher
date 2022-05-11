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
 * @file AssemblingXmlDocument.h
 * @brief Assembling xml document handler header
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
 * @brief Assembling xml document handler
 *
 * XML document handler for the assembling file for dynamic models
 */
class AssemblingXmlDocument : public xml::sax::parser::ComposableDocumentHandler {
 public:
  /**
   * @brief Connection XML element
   */
  struct Connection {
    std::string var1;  ///< first variable to connect
    std::string var2;  ///< second variable to connect
  };

  /**
   * @brief Bus XML element
   */
  struct Bus {
    std::string voltageLevel;  ///< voltage level of the bus
  };

  /**
   * @brief Shunt XML element
   */
  struct Shunt {
    std::string voltageLevel;  ///< id of the voltage level containing the shunts
  };

  /**
   * @brief Tfo XML element
   */
  struct Tfo {
    std::string name;  ///< name of the transformer
  };

  /**
   * @brief Line XML element
   */
  struct Line {
    std::string name;  ///< name of the line
  };

  /**
   * @brief Model XML element
   */
  struct Model {
    std::string id;  ///< id of the model
    std::string lib;  ///< lib of the model
  };

  /**
   * @brief Macro connect XML element defining a macro connection of a dynamic model
   */
  struct MacroConnect {
    std::string macroConnection;  ///< macro connection id
    std::string id;               ///< association id to use
  };

  /**
   * @brief Macro connection XML element defining a macro connector
   */

  struct MacroConnection {
    std::string id;                       ///< macro connection id
    std::vector<Connection> connections;  ///< list of connections of the macro connections
  };

  /**
   * @brief Single association XML element
   */
  struct SingleAssociation {
    std::string id;              ///< unique association id
    boost::optional<Bus> bus;    ///< bus of the association
    boost::optional<Tfo> tfo;    ///< transformer of the association
    boost::optional<Line> line;  ///< line of the association
  };

  /**
   * @brief Multiple association XML element
   */
  struct MultipleAssociation {
    std::string id;  ///< unique association id
    Shunt shunt;     ///< Shunt of the association
  };

  /**
   * @brief Model association XML element
   */
  struct ModelAssociation {
    std::string id;   ///< model connection id
    Model model;      ///< model of the association
  };

  /**
   * @brief Dynamic automaton XML element
   */
  struct DynamicAutomaton {
    std::string id;                           ///< automaton id
    std::string lib;                          ///< automaton library name
    std::vector<MacroConnect> macroConnects;  ///< list of macro connections to use
  };

 public:
  /// @brief Default constructor
  AssemblingXmlDocument();

  /**
   * @brief Retrieve the list of macro connections
   * @returns the list of macro connections elements
   */
  const std::vector<MacroConnection>& macroConnections() const {
    return macroConnections_;
  }

  /**
   * @brief Retrieve the list of single associations
   * @returns the list of single associations elements
   */
  const std::vector<SingleAssociation>& singleAssociations() const {
    return singleAssociations_;
  }

  /**
   * @brief Retrieve the list of multiple associations
   * @returns the list of multiple associations elements
   */
  const std::vector<MultipleAssociation>& multipleAssociations() const {
    return multipleAssociations_;
  }

  /**
   * @brief Retrieve the list of model associations
   * @returns the list of model associations elements
   */
  const std::vector<ModelAssociation>& modelAssociations() const {
    return modelAssociations_;
  }

  /**
   * @brief Retrieve the list of dynamic dynamic models
   * @returns the list of dynamic dynamic models elements
   */
  const std::vector<DynamicAutomaton>& dynamicAutomatons() const {
    return dynamicAutomatons_;
  }

 private:
  /**
   * @brief Connection element handler
   */
  struct ConnectionHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit ConnectionHandler(const elementName_type& root);

    boost::optional<Connection> currentConnection;  ///< the current conection element
  };

  /**
   * @brief Bus element handler
   */

  struct BusHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit BusHandler(const elementName_type& root);

    boost::optional<Bus> currentBus;  ///< current bus element
  };

  /**
   * @brief Shunt element handler
   */
  struct ShuntHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit ShuntHandler(const elementName_type& root);

    boost::optional<Shunt> currentShunt;  ///< current shunt element
  };

  /**
   * @brief Transformer element handler
   */
  struct TfoHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit TfoHandler(const elementName_type& root);

    boost::optional<Tfo> currentTfo;  ///< current transformer element
  };

  /**
   * @brief Line element handler
   */
  struct LineHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit LineHandler(const elementName_type& root);

    boost::optional<Line> currentLine;  ///< current line element
  };

  /**
   * @brief Model element handler
   */
  struct ModelHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit ModelHandler(const elementName_type& root);

    boost::optional<Model> currentModel;  ///< current model element
  };

  /**
   * @brief Macro connect element handler
   */
  struct MacroConnectHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit MacroConnectHandler(const elementName_type& root);

    boost::optional<MacroConnect> currentMacroConnect;  ///< current macro connect element
  };

  /**
   * @brief Macro connection handler
   */
  struct MacroConnectionHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit MacroConnectionHandler(const elementName_type& root);

    boost::optional<MacroConnection> currentMacroConnection;  ///< current macro connection element

    ConnectionHandler connectionHandler;  ///< connection handler
  };

  /**
   * @brief Single association handler
   */
  struct SingleAssociationHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit SingleAssociationHandler(const elementName_type& root);

    boost::optional<SingleAssociation> currentSingleAssociation;  ///< current single association

    BusHandler busHandler;    ///< bus element handler
    LineHandler lineHandler;  ///< line element handler
    TfoHandler tfoHandler;    ///< transformer element handler
  };

  /**
   * @brief Multi association element handler
   */
  struct MultipleAssociationHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit MultipleAssociationHandler(const elementName_type& root);

    boost::optional<MultipleAssociation> currentMultipleAssociation;  ///< current multiple association element

    ShuntHandler shuntHandler;  ///< shunt element handler
  };

  /**
   * @brief Model association element handler
   */
  struct ModelAssociationHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit ModelAssociationHandler(const elementName_type& root);

    boost::optional<ModelAssociation> currentModelAssociation;  ///< current model association element

    ModelHandler modelHandler;  ///< model element handler
  };

  /**
   * @brief Automaton element handler
   */
  struct DynamicAutomatonHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit DynamicAutomatonHandler(const elementName_type& root);

    boost::optional<DynamicAutomaton> currentDynamicAutomaton;  ///< current dynamic model element

    MacroConnectHandler macroConnectHandler;  ///< macro connect handler
  };

 private:
  MacroConnectionHandler macroConnectionHandler_;          ///< Macro connection handler
  SingleAssociationHandler singleAssociationHandler_;      ///< Single association handler
  MultipleAssociationHandler multipleAssociationHandler_;  ///< Multi association handler
  ModelAssociationHandler modelAssociationHandler_;        ///< Model association handler
  DynamicAutomatonHandler dynamicAutomatonHandler_;        ///< Automaton handler

  std::vector<MacroConnection> macroConnections_;          ///< list of macro connections
  std::vector<SingleAssociation> singleAssociations_;      ///< list of single associations
  std::vector<MultipleAssociation> multipleAssociations_;  ///< list of multiple associations
  std::vector<ModelAssociation> modelAssociations_;        ///< list of model associations
  std::vector<DynamicAutomaton> dynamicAutomatons_;        ///< list of dynamic models
};
}  // namespace inputs
}  // namespace dfl
