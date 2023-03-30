# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(TestSlackNodeAlgo ALGO)
target_link_libraries(ALGO.TestSlackNodeAlgo DynaFlowLauncher::algo)
DEFINE_TEST(TestConnexityAlgo ALGO IIDM)
target_link_libraries(ALGO.TestConnexityAlgo DynaFlowLauncher::algo)
DEFINE_TEST_XML(TestGeneratorsAlgo ALGO)
target_link_libraries(ALGO.TestGeneratorsAlgo DynaFlowLauncher::algo)
add_dependencies(ALGO.TestGeneratorsAlgo SecondaryVoltageControlSimp)
set_property(TEST ALGO.TestGeneratorsAlgo APPEND PROPERTY ENVIRONMENT "DYNAFLOW_LAUNCHER_LIBRARIES=${CMAKE_CURRENT_BINARY_DIR}")
set_property(TEST ALGO.TestGeneratorsAlgo APPEND PROPERTY ENVIRONMENT "DYNAWO_INSTALL_DIR=${DYNAWO_HOME}")
DEFINE_TEST(TestLoadsAlgo ALGO)
target_link_libraries(ALGO.TestLoadsAlgo DynaFlowLauncher::algo)
DEFINE_TEST(TestHvdcLineAlgo ALGO)
target_link_libraries(ALGO.TestHvdcLineAlgo DynaFlowLauncher::algo)
DEFINE_TEST(TestShuntCounterAlgo ALGO)
target_link_libraries(ALGO.TestShuntCounterAlgo DynaFlowLauncher::algo)
DEFINE_TEST(TestLinesAlgo ALGO)
target_link_libraries(ALGO.TestLinesAlgo DynaFlowLauncher::algo)
DEFINE_TEST(TestStaticVarCompensatorAlgo ALGO)
target_link_libraries(ALGO.TestStaticVarCompensatorAlgo DynaFlowLauncher::algo)
DEFINE_TEST(TestContingencyAlgo ALGO)
target_link_libraries(ALGO.TestContingencyAlgo DynaFlowLauncher::algo)

# Dummy Library for algo test
add_library(dummyLib SHARED res/DummyLibFile.cpp)
set_target_properties(dummyLib PROPERTIES PREFIX "")
add_library(SecondaryVoltageControlSimp SHARED res/SecondaryVoltageControlSimp.cpp)
set_target_properties(SecondaryVoltageControlSimp PROPERTIES PREFIX "")

DEFINE_TEST_XML(TestAlgoDynModel ALGO)
target_link_libraries(ALGO.TestAlgoDynModel DynaFlowLauncher::algo)
add_dependencies(ALGO.TestAlgoDynModel dummyLib)
set_property(TEST ALGO.TestAlgoDynModel APPEND PROPERTY ENVIRONMENT "DYNAFLOW_LAUNCHER_LIBRARIES=${CMAKE_CURRENT_BINARY_DIR}")
set_property(TEST ALGO.TestAlgoDynModel APPEND PROPERTY ENVIRONMENT "DYNAWO_INSTALL_DIR=${DYNAWO_HOME}")
set_property(TEST ALGO.TestAlgoDynModel APPEND PROPERTY ENVIRONMENT "DYNAFLOW_LAUNCHER_XSD=")  # to avoid parse validation error
