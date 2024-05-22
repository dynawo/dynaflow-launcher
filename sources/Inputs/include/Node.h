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
 * @file  Node.h
 *
 * @brief Node structure header file
 *
 */

#pragma once

#include "Behaviours.h"

#include <DYNServiceManagerInterface.h>
#include <memory>
#include <string>
#include <vector>
#include <array>

namespace dfl {
/// @brief Namespace for inputs of Dynaflow launcher
namespace inputs {

class Node;

/**
 * @brief topological voltage level structure
 *
 * aggregate of nodes
 */
struct VoltageLevel {
  using VoltageLevelId = std::string;  ///< Voltage level id

  /**
   * @brief Constructor
   *
   * @param vlid voltage level id
   */
  explicit VoltageLevel(const VoltageLevelId &vlid);

  const VoltageLevelId id;                   ///< id
  std::vector<std::shared_ptr<Node>> nodes;  ///< nodes contained in the voltage level
};

/**
 * @brief Topological line between two nodes
 */
class Line {
 public:
  using LineId = std::string;  ///< Alias for line id

  /**
   * @brief Build a line
   *
   * This will update the neighbours and the line references in the input nodes
   * @param lineId the line id
   * @param node1 the origin of the line
   * @param node2 the extremity of the line
   * @param season active season of the line
   * @param isConnectedOnNode1 whether the line is initially connected on node 1
   * @param isConnectedOnNode2 whether the lien is initially connected on node 2
   * @returns the built line
   */
  static std::shared_ptr<Line> build(const LineId &lineId, const std::shared_ptr<Node> &node1, const std::shared_ptr<Node> &node2, const std::string &season,
                                     bool isConnectedOnNode1, bool isConnectedOnNode2);

  const LineId id;                                   ///< line id
  const std::string activeSeason;                    ///< active season associated with the line
  const std::array<std::shared_ptr<Node>, 2> nodes;  ///< nodes of the line

 private:
  /**
   * @brief Constructor
   *
   * @param lineId the line id
   * @param node1 the origin of the line
   * @param node2 the extremity of the line
   * @param season the active season of the line
   */
  Line(const LineId &lineId, const std::shared_ptr<Node> &node1, const std::shared_ptr<Node> &node2, const std::string &season);
};

/**
 * @brief topological Transformer
 */
class Tfo {
 public:
  using TfoId = std::string;  ///< alias for transformer id

  /**
   * @brief Build a two windings transformer
   *
   * This will update the references inside the input nodes
   * @param tfoId the transformer id
   * @param node1 the first node
   * @param node2 the second node
   * @param season the active season of the transformer
   * @param isConnectedOnNode1 whether the two windings transformer is initially connected on node 1
   * @param isConnectedOnNode2 whether the two windings transformer is initially connected on node 2
   * @returns the built transformer
   */
  static std::shared_ptr<Tfo> build(const TfoId &tfoId, const std::shared_ptr<Node> &node1, const std::shared_ptr<Node> &node2, const std::string &season,
                                    bool isConnectedOnNode1, bool isConnectedOnNode2);

  /**
   * @brief Build a three windings transformer
   *
   * This will update the references inside the input nodes
   * @param tfoId the transformer id
   * @param node1 the first node
   * @param node2 the second node
   * @param node3 the third node
   * @param season the active season of the transformer
   * @param isConnectedOnNode1 whether the three windings transformer is initially connected on node 1
   * @param isConnectedOnNode2 whether the three windings transformer is initially connected on node 2
   * @param isConnectedOnNode3 whether the three windings transformer is initially connected on node 3
   * @returns the built transformer
   */
  static std::shared_ptr<Tfo> build(const TfoId &tfoId, const std::shared_ptr<Node> &node1, const std::shared_ptr<Node> &node2,
                                    const std::shared_ptr<Node> &node3, const std::string &season, bool isConnectedOnNode1, bool isConnectedOnNode2,
                                    bool isConnectedOnNode3);

  const TfoId id;                                  ///< transformer id
  const std::vector<std::shared_ptr<Node>> nodes;  ///< list of nodes
  const std::string activeSeason;                  ///< active season associated with the transformer

 private:
  /**
   * @brief Constructor
   *
   * @param tfoId the transformer id
   * @param node1 the first node
   * @param node2 the second node
   * @param season the active season of the transformer
   */
  Tfo(const TfoId &tfoId, const std::shared_ptr<Node> &node1, const std::shared_ptr<Node> &node2, const std::string &season);

  /**
   * @brief Constructor
   *
   * @param tfoId the transformer id
   * @param node1 the first node
   * @param node2 the second node
   * @param node3 the third node
   * @param season the active season of the transformer
   */
  Tfo(const TfoId &tfoId, const std::shared_ptr<Node> &node1, const std::shared_ptr<Node> &node2, const std::shared_ptr<Node> &node3,
      const std::string &season);
};

/// @brief Topological shunt
struct Shunt {
  using ShuntId = std::string;  ///< alias for shunt id

  /**
   * @brief Constructor
   *
   * @param id the shunt id
   */
  explicit Shunt(const ShuntId &id) : id(id) {}

  const ShuntId id;  ///< Shunt id
};

/// @brief Topological dangling line
struct DanglingLine {
  using DanglingLineId = std::string;  ///< alias for dangling line id

  /**
   * @brief Constructor
   *
   * @param id the dangling line id
   */
  explicit DanglingLine(const DanglingLineId &id) : id(id) {}

  const DanglingLineId id;  ///< Dangling line id
};

/// @brief Topological Bus bar section
struct BusBarSection {
  using BusBarSectionId = std::string;  ///< alias for bus bar section id

  /**
   * @brief Constructor
   *
   * @param id the bus bar section id
   */
  explicit BusBarSection(const BusBarSectionId &id) : id(id) {}

  const BusBarSectionId id;  ///< Bus bar section id
};

/**
 * @brief topological node structure
 *
 * This implement a graph node concept. It contains only the information required to perform the algorithms and not all information extractable from network
 * file
 */
class Node {
 public:
  using NodeId = std::string;  ///< node id definition

  /**
   * @brief Builder for node
   *
   * This builder will perform connections for voltage level element
   *
   * @param id the node id
   * @param vl the voltage level element containing the node
   * @param nominalVoltage the nominal voltage of the node
   * @param shunts the list of the shunts connectable to this node
   * @param fictitious the flag to mark the node is fictitious
   * @param serviceManagerNode the dynawo service manager to use
   *
   * @returns the built node
   */
  static std::shared_ptr<Node> build(const NodeId &id, const std::shared_ptr<VoltageLevel> &vl, double nominalVoltage, const std::vector<Shunt> &shunts,
                                     bool fictitious = false, boost::shared_ptr<DYN::ServiceManagerInterface> serviceManagerNode = nullptr);
  /**
   * @brief Retrieve the buses linked to this node through its voltage level
   * @returns the list of the bus ids linked to this node
   */
  const std::vector<std::string> &getBusesConnectedByVoltageLevel();
  /**
   * @brief Returns true if this node is connected to the network
   * @returns true if this node is connected to the network
   */
  bool isBusConnected();

  const NodeId id;                                                 ///< node id
  const std::weak_ptr<VoltageLevel> voltageLevel;                  ///< voltage level containing the node
  const double nominalVoltage;                                     ///< Nominal voltage of the node
  const std::vector<Shunt> shunts;                                 ///< Shunts connectable to the node
  const bool fictitious;                                           ///< Flag to mark the node is fictitious
  std::vector<std::weak_ptr<Line>> lines;                          ///< Lines connected to this node
  std::vector<std::weak_ptr<Tfo>> tfos;                            ///< Transformers connected to this node
  std::vector<std::shared_ptr<Node>> neighbours;                   ///< list of neighbours
  std::vector<Load> loads;                                         ///< list of loads associated to this node
  std::vector<Generator> generators;                               ///< list of generators associated to this node
  std::vector<std::weak_ptr<Converter>> converters;                ///< list of converter associated to this node
  std::vector<StaticVarCompensator> svarcs;                        ///< List of static var compensators
  std::vector<DanglingLine> danglingLines;                         ///< List of dangling lines
  std::vector<BusBarSection> busBarSections;                       ///< List of bus bar sections
  boost::shared_ptr<DYN::ServiceManagerInterface> serviceManager;  ///< Service manager to use

 private:
  /**
   * @brief Constructor
   *
   * Private constructor because we are supposed to use only the factory builder
   *
   * @param id the node id
   * @param vl the voltage level element containing the node
   * @param nominalVoltage the voltage level associated with the node
   * @param shunts the list of the shunts connectable to this node
   * @param fictitious the flag to mark if node is fictitious
   * @param serviceManagerNode the dynawo service manager to use
   */
  Node(const NodeId &id, const std::shared_ptr<VoltageLevel> vl, double nominalVoltage, const std::vector<Shunt> &shunts, bool fictitious,
       boost::shared_ptr<DYN::ServiceManagerInterface> serviceManagerNode);

 private:
  bool busesConnectedInitialized;           ///< true if busesConnected was filled with getBusesConnectedByVoltageLevel, false otherwise
  std::vector<std::string> busesConnected;  ///< List of buses connected by this switch
};

/**
 * @brief Determines if two nodes are equal
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns status of the comparaison
 */
bool operator==(const Node &lhs, const Node &rhs);

/**
 * @brief Determines if two nodes are different
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns status of the comparaison
 */
bool operator!=(const Node &lhs, const Node &rhs);

/**
 * @brief Determines if a node is inferior to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns status of the comparaison
 */
bool operator<(const Node &lhs, const Node &rhs);

/**
 * @brief Determines if a node is inferior or equal to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns status of the comparaison
 */
bool operator<=(const Node &lhs, const Node &rhs);

/**
 * @brief Determines if a node is superior to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns status of the comparaison
 */
bool operator>(const Node &lhs, const Node &rhs);

/**
 * @brief Determines if a node is superior or equal to another node
 *
 * Used criteria is node id
 *
 * @param lhs first node
 * @param rhs second node
 *
 * @returns status of the comparaison
 */
bool operator>=(const Node &lhs, const Node &rhs);

}  // namespace inputs
}  // namespace dfl
