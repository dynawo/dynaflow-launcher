//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//
#include "Contingencies.h"
#include "ParEvent.h"
#include "Tests.h"

#include <boost/filesystem.hpp>

testing::Environment* initXmlEnvironment();

testing::Environment* const env = initXmlEnvironment();

TEST(TestParEvent, write) {
  using ContingencyElement = dfl::inputs::ContingencyElement;
  using ElementType = dfl::inputs::ContingencyElement::Type;

  std::string basename = "TestParEvent";
  std::string filename = basename + ".par";

  boost::filesystem::path outputPath(outputPathResults);
  outputPath.append(basename);

  if (!boost::filesystem::exists(outputPath)) {
    boost::filesystem::create_directories(outputPath);
  }

  auto contingency = dfl::inputs::Contingency("TestContingency");
  // We need the three of them to check the three cases that can be generated
  contingency.elements.emplace_back("TestBranch", ElementType::BRANCH);                       // buildBranchDisconnection (branch case)
  contingency.elements.emplace_back("TestGenerator", ElementType::GENERATOR);                 // buildEventSetPointBooleanDisconnection
  contingency.elements.emplace_back("TestShuntCompensator", ElementType::SHUNT_COMPENSATOR);  // buildEventSetPointRealDisconnection (general case)
  contingency.elements.emplace_back("TestGeneratorNetwork", ElementType::GENERATOR);          // network disconnection
  contingency.elements.emplace_back("TestLoadNetwork", ElementType::LOAD);                    // network disconnection
  contingency.elements.emplace_back("TestStaticVarCompensatorNetwork", ElementType::STATIC_VAR_COMPENSATOR);  // network disconnection

  std::unordered_set<std::string> networkElements;
  networkElements.insert("TestGeneratorNetwork");
  networkElements.insert("TestLoadNetwork");
  networkElements.insert("TestStaticVarCompensatorNetwork");

  outputPath.append(filename);
  dfl::outputs::ParEvent par(
      dfl::outputs::ParEvent::ParEventDefinition(basename, outputPath.generic_string(), contingency, networkElements, std::chrono::seconds(80)));
  par.write();

  boost::filesystem::path reference("reference");
  reference.append(basename);
  reference.append(filename);

  dfl::test::checkFilesEqual(outputPath.generic_string(), reference.generic_string());
}
