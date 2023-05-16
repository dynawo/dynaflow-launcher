//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file AssemblingDataBase.h
 * @brief bases structures to represent the assembling
 */

#pragma once

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <xml/sax/parser/Attributes.h>
#include <xml/sax/parser/ComposableDocumentHandler.h>
#include <xml/sax/parser/ComposableElementHandler.h>

namespace dfl {
namespace inputs {

/**
 * @brief bases structures to represent the assembling
 */
class AssemblingDataBase {
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
   * @brief Multiple Shunts XML element
   */
  struct MultipleShunts {
    std::string voltageLevel;  ///< id of the voltage level containing the shunts
  };

  /**
   * @brief Generator XML element
   */
  struct Generator {
    std::string name;  ///< name of the generator
  };

  /**
   * @brief Tfo XML element
   */
  struct Tfo {
    std::string name;  ///< name of the transformer
  };

  /**
   * @brief Single shunt XML element
   */
  struct SingleShunt {
    std::string name;  ///< name of the shunt
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
    std::string id;                     ///< unique association id
    boost::optional<Bus> bus;           ///< bus of the association
    boost::optional<Tfo> tfo;           ///< transformer of the association
    boost::optional<Line> line;         ///< line of the association
    boost::optional<SingleShunt> shunt;  ///< single shunt of the association
    std::vector<Generator> generators;  ///< List of generators
  };

  /**
   * @brief Multiple association XML element
   */
  struct MultipleAssociation {
    std::string id;                ///< unique association id
    boost::optional<MultipleShunts> shunt;     ///< Id of the voltage level containing the shunts
  };

  /**
   * @brief Dynamic automaton XML element
   */
  struct DynamicAutomaton {
    std::string id;                           ///< automaton id
    std::string lib;                          ///< automaton library name
    std::vector<MacroConnect> macroConnects;  ///< list of macro connections to use
  };

  /**
   * @brief Device XML element
   */
  struct Device {
    std::string id;  ///< device id
  };

  /**
   * @brief Property XML element
   */
  struct Property {
    std::string id;               ///< property id
    std::vector<Device> devices;  ///< list of devices
  };

 private:
  /**
   * @brief Assembling xml document handler
   *
   * XML document handler for the assembling file for dynamic models
   */
  class AssemblingXmlDocument : public xml::sax::parser::ComposableDocumentHandler {
   public:
    /**
     * @brief Constructor
     * @param db assembling data base to update
     */
    explicit AssemblingXmlDocument(AssemblingDataBase& db);

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

      boost::optional<AssemblingDataBase::Connection> currentConnection;  ///< the current conection element
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

      boost::optional<AssemblingDataBase::Bus> currentBus;  ///< current bus element
    };

    /**
     * @brief Multiple Shunts element handler
     */
    struct MultipleShuntsHandler : public xml::sax::parser::ComposableElementHandler {
      /**
       * @brief Constructor
       * @param root the root element to parse
       */
      explicit MultipleShuntsHandler(const elementName_type& root);

      boost::optional<AssemblingDataBase::MultipleShunts> currentMultipleShunts;  ///< current multiple shunts element
    };

    /**
     * @brief Generator element handler
     */
    struct GeneratorHandler : public xml::sax::parser::ComposableElementHandler {
      /**
       * @brief Constructor
       * @param root the root element to parse
       */
      explicit GeneratorHandler(const elementName_type& root);

      boost::optional<AssemblingDataBase::Generator> currentGenerator;  ///< current generator element
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

      boost::optional<AssemblingDataBase::Tfo> currentTfo;  ///< current transformer element
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

      boost::optional<AssemblingDataBase::Line> currentLine;  ///< current line element
    };

  /**
   * @brief Single shunt element handler
   */
  struct SingleShuntHandler : public xml::sax::parser::ComposableElementHandler {
    /**
     * @brief Constructor
     * @param root the root element to parse
     */
    explicit SingleShuntHandler(const elementName_type& root);

    boost::optional<AssemblingDataBase::SingleShunt> currentSingleShunt;  ///< current shunt element
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

      boost::optional<AssemblingDataBase::MacroConnect> currentMacroConnect;  ///< current macro connect element
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

      boost::optional<AssemblingDataBase::MacroConnection> currentMacroConnection;  ///< current macro connection element

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

      boost::optional<AssemblingDataBase::SingleAssociation> currentSingleAssociation;  ///< current single association

      BusHandler busHandler;              ///< bus element handler
      LineHandler lineHandler;            ///< line element handler
      TfoHandler tfoHandler;              ///< transformer element handler
      SingleShuntHandler singleShuntHandler;    ///< single shunt element handler
      GeneratorHandler generatorHandler;  ///< generator element handler
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

      boost::optional<AssemblingDataBase::MultipleAssociation> currentMultipleAssociation;  ///< current multiple association element

      MultipleShuntsHandler multipleShuntsHandler;  ///< multiple shunts element handler
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

      boost::optional<AssemblingDataBase::DynamicAutomaton> currentDynamicAutomaton;  ///< current dynamic model element

      MacroConnectHandler macroConnectHandler;  ///< macro connect handler
    };

    /**
     * @brief Device element handler
     */
    struct DeviceHandler : public xml::sax::parser::ComposableElementHandler {
      /**
       * @brief Constructor
       * @param root the root element to parse
       */
      explicit DeviceHandler(const elementName_type& root);

      boost::optional<AssemblingDataBase::Device> currentDevice;  ///< current device element
    };

    /**
     * @brief Property element handler
     */
    struct PropertyHandler : public xml::sax::parser::ComposableElementHandler {
      /**
       * @brief Constructor
       * @param root the root element to parse
       */
      explicit PropertyHandler(const elementName_type& root);

      boost::optional<AssemblingDataBase::Property> currentProperty;  ///< current property

      DeviceHandler deviceHandler;  ///< device handler
    };

   private:
    MacroConnectionHandler macroConnectionHandler_;          ///< Macro connection handler
    SingleAssociationHandler singleAssociationHandler_;      ///< Single association handler
    MultipleAssociationHandler multipleAssociationHandler_;  ///< Multi association handler
    DynamicAutomatonHandler dynamicAutomatonHandler_;        ///< Automaton handler
    PropertyHandler propertyHandler_;                        ///< Property handler
  };

 public:
  /**
   * @brief Constructor
   * @param assemblingFilePath the assembling document file path
   */
  explicit AssemblingDataBase(const boost::filesystem::path& assemblingFilePath);

  /**
   * @brief Retrieve a macro connection with its id
   * @param id macro connection id
   * @returns the macro connection with the given id, throw if not found
   */
  const MacroConnection& getMacroConnection(const std::string& id) const;


  /**
   * @brief Retrieve a single association with its id
   * @param id single association id
   * @returns the single associations element with the given id, throw if not found
   */
  const SingleAssociation& getSingleAssociation(const std::string& id) const;

  /**
   * @brief test if a single association with the given id exists
   * @param id single association id
   * @returns true if the single association exists, false otherwise
   */
  bool isSingleAssociation(const std::string& id) const;

  /**
   * @brief Retrieve a single association id that contains the generator with the given name
   * @param name generator id
   * @returns single association id that contains the generator with the given name, empty string if not found
   */
  std::string getSingleAssociationFromGenerator(const std::string& name) const;

  /**
   * @brief Retrieve a multiple association with its id
   * @param id multi association id
   * @returns multiple association element with the given id, throw if not found
   */
  const MultipleAssociation& getMultipleAssociation(const std::string& id) const;

  /**
   * @brief test if a multiple association with the given id exists
   * @param id multiple association id
   * @returns true if the multiple association exists, false otherwise
   */
  bool isMultipleAssociation(const std::string& id) const;

  /**
   * @brief Retrieve the list of dynamic dynamic models
   * @returns the list of dynamic dynamic models elements
   */
  const std::map<std::string, DynamicAutomaton>& dynamicAutomatons() const {
    return dynamicAutomatons_;
  }

  /**
   * @brief determines if the assembling data base contains one or more SVCs
   * @returns true if the assembling data base contains one or more SVCs, false otherwise
   */
  bool containsSVC() const {
    return containsSVC_;
  }

  /**
   * @brief Retrieve a property with its id
   * @param id property id
   * @returns property with the given id, empty property if not found
   */
  const Property& getProperty(const std::string& id) const;

  /**
   * @brief test if a property with the given id exists
   * @param id property id
   * @returns true if theproperty exists, false otherwise
   */
  bool isProperty(const std::string& id) const;

 private:
  std::unordered_map<std::string, MacroConnection> macroConnections_;          ///< list of macro connections
  std::unordered_map<std::string, SingleAssociation> singleAssociations_;      ///< list of single associations
  std::unordered_map<std::string, MultipleAssociation> multipleAssociations_;  ///< list of multiple associations
  std::unordered_map<std::string, std::string> generatorIdToSingleAssociationsId_;  ///< generator id associated to the single associations that contains it
  std::map<std::string, DynamicAutomaton> dynamicAutomatons_;                       ///< list of dynamic automatons
  bool containsSVC_;                                      ///< true if the assembling data base contains one or more SVC, false otherwise
  std::unordered_map<std::string, Property> properties_;  ///< list of properties
};

}  // namespace inputs

}  // namespace dfl
