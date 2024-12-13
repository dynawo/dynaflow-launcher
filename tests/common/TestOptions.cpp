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
#include "DFLError_keys.h"

#include <gtest_dynawo.h>

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

TEST(Options, noInputProvided) {
  dfl::common::Options options;

  char programName[] = {"DynaFlowLauncher"};
  char* argv[] = {programName};
  ASSERT_EQ(dfl::common::Options::Request::HELP, options.parse(1, argv));
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

TEST(Options, nominalArchive) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/nominalArchive/nominalArchive.zip"};
  char* argv[] = {argv0, argv1};
  auto status = options.parse(2, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_N, status);
}

TEST(Options, nominalArchiveLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/nominalArchiveLogLevel/nominalArchiveLogLevel.zip"};
  char argv2[] = {"--log-level=DEBUG"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_EQ(dfl::common::Options::Request::RUN_SIMULATION_N, status);
}

TEST(Options, nominalArchiveWrongLogLevel) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/nominalArchiveWrongLogLevel/nominalArchiveWrongLogLevel.zip"};
  char argv2[] = {"--log-level=NO_LEVEL"};  // this level is not defined
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

TEST(Options, InvalidCombination_NetworkFileAndInputArchive) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--network=test.iidm"};
  char argv2[] = {"--input-archive=res/test.zip"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
  ASSERT_EQ(dfl::common::Options::Request::ERROR, status);
}

TEST(Options, InvalidCombination_ConfigFileAndInputArchive) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--config=test.json"};
  char argv2[] = {"--input-archive=res/test.zip"};
  char* argv[] = {argv0, argv1, argv2};
  auto status = options.parse(3, argv);
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

TEST(Options, AlreadyInitializedNetworkFile) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/AlreadyInitializedNetworkFile/2NetworksArchive.zip"};
  char* argv[] = {argv0, argv1};
  ASSERT_THROW_DYNAWO(options.parse(2, argv), DYN::Error::GENERAL, dfl::KeyError_t::AlreadyInitializedNetworkFileInput);
}

TEST(Options, AlreadyInitializedConfigFileInput) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/AlreadyInitializedConfigFile/2ConfigFilesArchive.zip"};
  char* argv[] = {argv0, argv1};
  ASSERT_THROW_DYNAWO(options.parse(2, argv), DYN::Error::GENERAL, dfl::KeyError_t::AlreadyInitializedConfigFileInput);
}

TEST(Options, AlreadyInitializedContingenciesInput) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/AlreadyInitializedContingencies/2ContingenciesFilesArchive.zip"};
  char* argv[] = {argv0, argv1};
  ASSERT_THROW_DYNAWO(options.parse(2, argv), DYN::Error::GENERAL, dfl::KeyError_t::AlreadyInitializedContingenciesInput);
}

TEST(Options, NoConfigFileFound) {
  dfl::common::Options options;

  char argv0[] = {"DynaFlowLauncher"};
  char argv1[] = {"--input-archive=res/NoConfigFileFound/FakeConfig.zip"};
  char* argv[] = {argv0, argv1};
  ASSERT_THROW_DYNAWO(options.parse(2, argv), DYN::Error::GENERAL, dfl::KeyError_t::NoConfigFileFound);
}
