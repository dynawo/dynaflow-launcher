//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydEvent.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestDydEvent, write) {
  using ContingencyElement = dfl::inputs::ContingencyElement;
  using ElementType = dfl::inputs::ContingencyElement::Type;

  std::string basename = "TestDydEvent";
  std::string dirname = "results";
  std::string filename = basename + ".dyd";

  boost::filesystem::path outputPath(dirname);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directory(outputPath);
  }

  auto contingency = dfl::inputs::Contingency("TestContingency");
  // We need one element per case handled in DydEvent
  contingency.elements.emplace_back("TestBranch", ElementType::BRANCH);                       // buildBranchDisconnection (branch case)
  contingency.elements.emplace_back("TestGenerator", ElementType::GENERATOR);                 // signal: "generator_switchOffSignal2"
  contingency.elements.emplace_back("TestLoad", ElementType::LOAD);                           // signal: "switchOff2"
  contingency.elements.emplace_back("TestHvdcLine", ElementType::HVDC_LINE);                  // signal: "hvdc_switchOffSignal2"
  contingency.elements.emplace_back("TestShuntCompensator", ElementType::SHUNT_COMPENSATOR);  // signal: "shunt_switchOffSignal2"
  contingency.elements.emplace_back("TestLine", ElementType::LINE);                           // general case

  outputPath.append(filename);
  dfl::outputs::DydEvent dyd(dfl::outputs::DydEvent::DydEventDefinition(basename, outputPath.generic_string(), contingency));
  dyd.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
