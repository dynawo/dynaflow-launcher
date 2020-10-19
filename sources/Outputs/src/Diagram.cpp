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

#include <fstream>

namespace dfl {
namespace outputs {

Diagram::Diagram(DiagramDefinition&& def) : def_{std::forward<DiagramDefinition>(def)} {}

void
Diagram::write() {
  //We don't want to create the file if there are no generators diagrams to write
  //maybe use stdbuf for using << and not .append(..()))
  std::stringstream buffer;
  buffer << "#1";
  //Modelica requires this file to start with "#1", if it is not present, problems occurs
  bool empty = true;

  for (algo::GeneratorDefinition& generator : def_.generators) {
    if (generator.model != algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN &&
        generator.model != algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN)
      continue;
    empty = false;

    std::sort(generator.points.begin(), generator.points.end(),
              [](const DYN::GeneratorInterface::ReactiveCurvePoint& gen1, const DYN::GeneratorInterface::ReactiveCurvePoint& gen2) -> bool {
                return gen1.p < gen2.p;
              });
    writeTable(generator, buffer, Tables::TABLE_QMIN);
    writeTable(generator, buffer, Tables::TABLE_QMAX);
  }

  if (!empty) {
    //Put buffer into file
    std::ofstream ofs(def_.filename, std::ofstream::out);
    ofs << buffer.str();
    ofs.close();
  }
}

void
Diagram::writeTable(const algo::GeneratorDefinition& generator, std::stringstream& buffer, Tables table) {
  buffer << "\ndouble ";
  std::size_t hash = constants::hash(generator.id);
  buffer << hash;
  if (table == Tables::TABLE_QMIN)
    buffer << constants::diagramMinTableSuffix << '(';
  else
    buffer << constants::diagramMaxTableSuffix << '(';
  std::size_t numberLines = generator.points.empty() ? 2 : generator.points.size();
  //The number of lines is 2 when there are no points
  buffer << numberLines;
  buffer << ",2)";

  const int divisorFactor = 100;
  if (generator.points.empty()) {
    double qValue = table == Tables::TABLE_QMIN ? generator.qmin : generator.qmax;
    buffer << '\n' << generator.pmin / divisorFactor << " " << qValue / divisorFactor << '\n';
    buffer << generator.pmax / divisorFactor << " " << qValue / divisorFactor;
  } else {
    for (const DYN::GeneratorInterface::ReactiveCurvePoint& point : generator.points) {
      double qValue = table == Tables::TABLE_QMIN ? point.qmin : point.qmax;
      buffer << '\n' << point.p / divisorFactor << " " << qValue / divisorFactor;
    }
  }
}

}  // namespace outputs
}  // namespace dfl
