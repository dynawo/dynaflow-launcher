# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(TestAlgo ALGO)
target_link_libraries(TestAlgo DynaFlowLauncher::algo)

# Dummy Library for algo test
add_library(dummyLib SHARED res/DummyLibFile.cpp)

DEFINE_TEST_XML(TestAlgoDynModel ALGO)
target_link_libraries(TestAlgoDynModel DynaFlowLauncher::algo)
add_dependencies(TestAlgoDynModel dummyLib)
set_property(TEST ALGO.TestAlgoDynModel PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LIBRARIES=${CMAKE_CURRENT_BINARY_DIR})
set_property(TEST ALGO.TestAlgoDynModel APPEND PROPERTY ENVIRONMENT DYNAWO_INSTALL_DIR=${DYNAWO_HOME})
