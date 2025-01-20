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

#include <boost/filesystem.hpp>

#include <sstream>

TEST(Options, help) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--help"};
  char* argv[] = {argv0, argv1};
  ASSERT_EQ(dfl::common::Options::Request::HELP, options.parse(2, argv));

  auto desc = options.desc();
  ASSERT_FALSE(desc.empty());
  std::stringstream ss;
  ss << options;
  ASSERT_FALSE(ss.str().empty());

  ASSERT_EQ(ss.str(), desc);
}

TEST(Options, version) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--version"};
  char* argv[] = {argv0, argv1};
  ASSERT_EQ(dfl::common::Options::Request::VERSION, options.parse(2, argv));
}

TEST(Options, missingIIDM) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--config=test.json"};
  char* argv[] = {argv0, argv1};
  ASSERT_EQ(dfl::common::Options::Request::ERROR, options.parse(2, argv));
}

TEST(Options, missingCONFIG) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char* argv[] = {argv0, argv1};
  ASSERT_EQ(dfl::common::Options::Request::ERROR, options.parse(2, argv));
}

TEST(Options, nominal) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm "};
  char argv2[] = {"--config=test.json"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_N, status);
}

TEST(Options, archive) {
  // Ensure the files are removed if they already exist before unzipping the archive
  const std::vector<boost::filesystem::path> archiveFiles = {"res/test.iidm", "res/test.json"};
  for (const boost::filesystem::path& archiveFile : archiveFiles) {
    if (boost::filesystem::exists(archiveFile)) {
      boost::filesystem::remove(archiveFile);
    }
  }

  dfl::common::Options options;
  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=res/test.iidm "};
  char argv2[] = {"--config=res/test.json"};
  char argv3[] = {"--input-archive=res/test.zip"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_N, status);
  ASSERT_TRUE(boost::filesystem::exists("res/test.iidm"));
  ASSERT_TRUE(boost::filesystem::exists("res/test.json"));
}

TEST(Options, nominalLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm "};
  char argv2[] = {"--config=test.json "};
  char argv3[] = {"--log-level=DEBUG"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_N, status);
}

TEST(Options, wrongLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--log-level=NO_LEVEL"};  // this level is not defined
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

/*  Systematic Analysis  */

TEST(Options, systematicAnalysis) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--contingencies=test.json"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_SA, status);
}

TEST(Options, systematicAnalysisLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--contingencies=test.json"};
  char argv4[] = {"--log-level=DEBUG"};
  char* argv[] = {argv0, argv1, argv2, argv3, argv4};
  auto status = options.parse(5, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_SA, status);
}

TEST(Options, systematicAnalysisWrongLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--contingencies=test.json"};
  char argv4[] = {"--log-level=NO_LEVEL"};  // this level is not defined
  char* argv[] = {argv0, argv1, argv2, argv3, argv4};
  auto status = options.parse(5, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

TEST(Options, systematicAnalysisMissingConfig) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--contingencies=test.json"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

TEST(Options, systematicAnalysisMissingIIDM) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--config=test.json"};
  char argv2[] = {"--contingencies=test.json"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

/*  N + SA  */

TEST(Options, NSA) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--contingencies=test.json"};
  char argv4[] = {"--nsa"};
  char* argv[] = {argv0, argv1, argv2, argv3, argv4};
  auto status = options.parse(5, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_NSA, status);
}

TEST(Options, NSAMissingIIDM) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--config=test.json"};
  char argv2[] = {"--contingencies=test.json"};
  char argv3[] = {"--nsa"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

TEST(Options, NSAMissingConfig) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--contingencies=test.json"};
  char argv3[] = {"--nsa"};
  char* argv[] = {argv0, argv1, argv2, argv3};
  auto status = options.parse(4, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

TEST(Options, NSALogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--contingencies=test.json"};
  char argv4[] = {"--log-level=DEBUG"};
  char argv5[] = {"--nsa"};
  char* argv[] = {argv0, argv1, argv2, argv3, argv4, argv5};
  auto status = options.parse(6, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_NSA, status);
}

TEST(Options, NSAWrongLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--config=test.json"};
  char argv3[] = {"--contingencies=test.json"};
  char argv4[] = {"--log-level=NO_LEVEL"};  // this level is not defined
  char argv5[] = {"--nsa"};
  char* argv[] = {argv0, argv1, argv2, argv3, argv4, argv5};
  auto status = options.parse(6, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}
