//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "gtest_dynawo.h"

#include <libxml/parser.h>
#include <xercesc/util/PlatformUtils.hpp>

class XmlEnvironment : public testing::Environment {
 public:
  ~XmlEnvironment() {}

  // Override this to define how to set up the environment.
  void SetUp() {
    xercesc::XMLPlatformUtils::Initialize();
    xmlInitParser();
  }

  // Override this to define how to tear down the environment.
  void TearDown() {
    xercesc::XMLPlatformUtils::Terminate();
    xmlCleanupParser();
  }
};

testing::Environment*
initXmlEnvironment() {
  return testing::AddGlobalTestEnvironment(new XmlEnvironment);
}
