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
 * @file  Diagram.cpp
 *
 * @brief Dynaflow launcher Diagram file writer implementation file
 *
 */

#include "Diagram.h"

#include "Constants.h"

#include <boost/filesystem.hpp>
#include <fstream>

namespace dfl {
namespace outputs {

Diagram::Diagram(DiagramDefinition&& def) : def_{std::forward<DiagramDefinition>(def)} {
  for (auto& generator : def_.generators) {
    std::sort(generator.points.begin(), generator.points.end(),
              [](const DYN::GeneratorInterface::ReactiveCurvePoint& lhs, const DYN::GeneratorInterface::ReactiveCurvePoint& rhs) { return lhs.p < rhs.p; });
  }
  for (auto& vscPair : def_.hvdcDefinitions.vscBusVSCDefinitionsMap) {
    auto& points = vscPair.second.points;
    std::sort(points.begin(), points.end(),
              [](const algo::VSCDefinition::ReactiveCurvePoint& lhs, const algo::VSCDefinition::ReactiveCurvePoint& rhs) { return lhs.p < rhs.p; });
  }
}

void
Diagram::write() const {
  writeGenerators();
  writeConverters();
}

void
Diagram::writeGenerators() const {
  for (const auto& generator : def_.generators) {
    if (!generator.isUsingDiagram())
      continue;
    if (!boost::filesystem::exists(def_.directoryPath)) {
      boost::filesystem::create_directories(def_.directoryPath);
    }

    std::stringstream buffer;
    //  Modelica requires this file to start with "#1", if it is not present, problems occurs
    buffer << "#1";

    writeTable(generator, buffer, Tables::TABLE_QMIN);
    writeTable(generator, buffer, Tables::TABLE_QMAX);
    boost::filesystem::path dir(def_.directoryPath);
    std::string filename = dir.append(outputs::constants::diagramFilename(generator.id)).generic_string();
    std::ofstream ofs(filename, std::ofstream::out);
    ofs << buffer.str();
    ofs.close();
  }
}

void
Diagram::writeVSC(const algo::VSCDefinition& vscDefinition) const {
  if (!boost::filesystem::exists(def_.directoryPath)) {
    boost::filesystem::create_directories(def_.directoryPath);
  }
  std::stringstream buffer;
  //  Modelica requires this file to start with "#1", if it is not present, problems occurs
  buffer << "#1";

  writeTable(vscDefinition, buffer, Tables::TABLE_QMIN);
  writeTable(vscDefinition, buffer, Tables::TABLE_QMAX);
  boost::filesystem::path dir(def_.directoryPath);
  std::string filename = dir.append(outputs::constants::diagramFilename(vscDefinition.id)).generic_string();
  std::ofstream ofs(filename, std::ofstream::out);
  ofs << buffer.str();
  ofs.close();
}

void
Diagram::writeLCC(const algo::HVDCDefinition::ConverterId& converterId, double powerFactor, double pMax) const {
  if (!boost::filesystem::exists(def_.directoryPath)) {
    boost::filesystem::create_directories(def_.directoryPath);
  }
  std::stringstream buffer;
  //  Modelica requires this file to start with "#1", if it is not present, problems occurs
  buffer << "#1";

  auto qMax = constants::computeQmax(powerFactor, pMax);
  LCCDefinition lccDefinition{converterId, {}, pMax, qMax, -pMax, -qMax};

  writeTable(lccDefinition, buffer, Tables::TABLE_QMIN);
  writeTable(lccDefinition, buffer, Tables::TABLE_QMAX);
  boost::filesystem::path dir(def_.directoryPath);
  std::string filename = dir.append(outputs::constants::diagramFilename(converterId)).generic_string();
  std::ofstream ofs(filename, std::ofstream::out);
  ofs << buffer.str();
  ofs.close();
}

void
Diagram::writeConverters() const {
  for (const auto& hvdcDefPair : def_.hvdcDefinitions.hvdcLines) {
    const auto& hvdcDef = hvdcDefPair.second;
    if (!hvdcDef.hasDiagramModel()) {
      continue;
    }

    // The presence of vscDefinitionx and the relevance of powerFactors array is garranteed according the type of HVDC line

    switch (hvdcDef.position) {
    case algo::HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT: {
      if (hvdcDef.vscDefinition1) {
        writeVSC(*hvdcDef.vscDefinition1);
      } else {
        writeLCC(hvdcDef.converter1Id, hvdcDef.powerFactors.at(0), hvdcDef.pMax);
      }
      break;
    }
    case algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT: {
      if (hvdcDef.vscDefinition2) {
        writeVSC(*hvdcDef.vscDefinition2);
      } else {
        writeLCC(hvdcDef.converter2Id, hvdcDef.powerFactors.at(1), hvdcDef.pMax);
      }
      break;
    }
    case algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT: {
      if (hvdcDef.vscDefinition1) {
        writeVSC(*hvdcDef.vscDefinition1);
      } else {
        writeLCC(hvdcDef.converter1Id, hvdcDef.powerFactors.at(0), hvdcDef.pMax);
      }
      if (hvdcDef.vscDefinition2) {
        writeVSC(*hvdcDef.vscDefinition2);
      } else {
        writeLCC(hvdcDef.converter2Id, hvdcDef.powerFactors.at(1), hvdcDef.pMax);
      }
      break;
    }
    default:  // impossible case by definition of the enum
      break;
    }
  }
}

template<class T>
void
Diagram::writeTable(const T& element, std::stringstream& buffer, Tables table) {
  buffer << "\ndouble ";
  std::size_t hash = constants::hash(element.id);
  buffer << hash;
  if (table == Tables::TABLE_QMIN)
    buffer << constants::diagramMinTableSuffix << '(';
  else
    buffer << constants::diagramMaxTableSuffix << '(';
  std::size_t numberLines = element.points.empty() ? 2 : element.points.size();
  //  The number of lines is 2 when there are no points
  buffer << numberLines;
  buffer << ",2)";

  const int divisorFactor = 100;
  if (element.points.empty()) {
    double qValue = table == Tables::TABLE_QMIN ? element.qmin : element.qmax;
    buffer << '\n' << element.pmin / divisorFactor << " " << qValue / divisorFactor << '\n';
    buffer << element.pmax / divisorFactor << " " << qValue / divisorFactor;
  } else {
    for (const auto& point : element.points) {
      double qValue = table == Tables::TABLE_QMIN ? point.qmin : point.qmax;
      buffer << '\n' << point.p / divisorFactor << " " << qValue / divisorFactor;
    }
  }
}
}  // namespace outputs
}  // namespace dfl
