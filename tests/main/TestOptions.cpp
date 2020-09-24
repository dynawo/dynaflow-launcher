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

TEST(Options, help) {
  common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--help"};
  char* argv[] = {argv0, argv1};
  ASSERT_FALSE(options.parse(2, argv));
}

TEST(Options, missingIIDM) {
  common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--config=test.json"};
  char* argv[] = {argv0, argv1};
  ASSERT_FALSE(options.parse(2, argv));
}

TEST(Options, missingCONFIG) {
  common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--iidm=test.iidm"};
  char* argv[] = {argv0, argv1};
  ASSERT_FALSE(options.parse(2, argv));
}

TEST(Options, nominal) {
  common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--iidm=test.iidm "};
  char argv2[] = {"--config=test.json"};
  char* argv[] = {argv0, argv1, argv2};
  ASSERT_TRUE(options.parse(3, argv));
}

TEST(Options, nominalLogLevel) {
  common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--iidm=test.iidm "};
  char argv2[] = {"--config=test.json "};
  char argv3[] = {"--log-level=DEBUG"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  ASSERT_TRUE(options.parse(4, argv));
}

TEST(Options, wrongLogLevel) {
  common::Options options;

  char argv0[] = {"DynawoLauncher"};
  char argv1[] = {"--iidm=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--log-level=NO_LEVEL"};  // this level is not defined
  char* argv[] = {argv0, argv1, argv2, argv3};
  ASSERT_FALSE(options.parse(4, argv));
}
