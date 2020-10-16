//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

#include "Options.h"
#include "Tests.h"

#include <sstream>

TEST(Options, help) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--help"};
  char* argv[] = {argv0, argv1};
  ASSERT_TRUE(std::get<0>(options.parse(2, argv)));
  ASSERT_EQ(dfl::common::Options::Request::HELP, std::get<1>(options.parse(2, argv)));

  auto desc = options.desc();
  ASSERT_FALSE(desc.empty());
  std::stringstream ss;
  ss << options;
  ASSERT_FALSE(ss.str().empty());

  ASSERT_EQ(ss.str(), desc);
}

TEST(Options, version) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--version"};
  char* argv[] = {argv0, argv1};
  ASSERT_TRUE(std::get<0>(options.parse(2, argv)));
  ASSERT_EQ(dfl::common::Options::Request::VERSION, std::get<1>(options.parse(2, argv)));
}

TEST(Options, missingIIDM) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--config=test.json"};
  char* argv[] = {argv0, argv1};
  ASSERT_FALSE(std::get<0>(options.parse(2, argv)));
}

TEST(Options, missingCONFIG) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char* argv[] = {argv0, argv1};
  ASSERT_FALSE(std::get<0>(options.parse(2, argv)));
}

TEST(Options, nominal) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--network=test.iidm "};
  char argv2[] = {"--config=test.json"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_TRUE(std::get<0>(status));
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION, std::get<1>(status));
}

TEST(Options, nominalLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--network=test.iidm "};
  char argv2[] = {"--config=test.json "};
  char argv3[] = {"--log-level=DEBUG"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_TRUE(std::get<0>(status));
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION, std::get<1>(status));
}

TEST(Options, wrongLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--log-level=NO_LEVEL"};  // this level is not defined
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_FALSE(std::get<0>(status));
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION, std::get<1>(status));
}
