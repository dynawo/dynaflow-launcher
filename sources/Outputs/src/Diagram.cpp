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
  bool empty = true;

  for (algo::GeneratorDefinition& generator : def_.generators) {
    if (generator.model != algo::GeneratorDefinition::ModelType::DIAGRAM_PQ_SIGNALN and
        generator.model != algo::GeneratorDefinition::ModelType::WITH_IMPEDANCE_DIAGRAM_PQ_SIGNALN)
      continue;
    empty = false;

    std::sort(generator.points.begin(), generator.points.end(),
              [](const DYN::GeneratorInterface::ReactiveCurvePoint& gen1, const DYN::GeneratorInterface::ReactiveCurvePoint& gen2) -> bool {
                return gen1.p < gen2.p;
              });

    write_table(generator, buffer, true);
    write_table(generator, buffer, false);
  }

  if (not empty) {
    //Put buffer into file
    std::ofstream ofs(def_.filename, std::ofstream::out);
    ofs << buffer.str();
    ofs.close();
  }
}

void
Diagram::write_table(const algo::GeneratorDefinition& generator, std::stringstream& buffer, bool is_table_qmin) {
  buffer << "\ndouble ";
  auto hash = constants::hash(generator.id);
  buffer << hash;
  if (is_table_qmin)
    buffer << constants::diagramMinTableSuffix << '(';
  else
    buffer << constants::diagramMaxTableSuffix << '(';
  std::size_t number_lines = generator.points.empty() ? 2 : generator.points.size();
  //The number of lines is 2 when there are no points
  buffer << number_lines;
  buffer << ",2)";

  if (generator.points.empty()) {
    double q_value = is_table_qmin ? generator.qmin : generator.qmax;
    buffer << '\n' << generator.pmin / 100 << " " << q_value / 100 << '\n';
    buffer << generator.pmax / 100 << " " << q_value / 100;
  } else {
    for (const DYN::GeneratorInterface::ReactiveCurvePoint& point : generator.points) {
      double q_value = is_table_qmin ? point.qmin : point.qmax;
      buffer << '\n' << point.p / 100 << " " << q_value / 100;
    }
  }
}

}  // namespace outputs
}  // namespace dfl