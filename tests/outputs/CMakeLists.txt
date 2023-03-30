# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(TestJob OUTPUTS)
target_link_libraries(OUTPUTS.TestJob DynaFlowLauncher::outputs)
set_property(TEST OUTPUTS.TestJob APPEND PROPERTY ENVIRONMENT "DYNAWO_HOME=${DYNAWO_HOME}")

DEFINE_TEST_XML(TestSolverPar OUTPUTS)
target_link_libraries(OUTPUTS.TestSolverPar DynaFlowLauncher::outputs)

DEFINE_TEST_XML(TestDyd OUTPUTS)
target_link_libraries(OUTPUTS.TestDyd DynaFlowLauncher::outputs)
set_property(TEST OUTPUTS.TestDyd APPEND PROPERTY ENVIRONMENT "DYNAFLOW_LAUNCHER_XSD=")  # to avoid parse validation error

DEFINE_TEST_XML(TestDydEvent OUTPUTS)
target_link_libraries(OUTPUTS.TestDydEvent DynaFlowLauncher::outputs)

DEFINE_TEST_XML(TestPar OUTPUTS)
target_link_libraries(OUTPUTS.TestPar DynaFlowLauncher::outputs)
set_property(TEST OUTPUTS.TestPar APPEND PROPERTY ENVIRONMENT "DYNAFLOW_LAUNCHER_XSD=")  # to avoid parse validation error

DEFINE_TEST_XML(TestParEvent OUTPUTS)
target_link_libraries(OUTPUTS.TestParEvent DynaFlowLauncher::outputs)

DEFINE_TEST(TestDiagram OUTPUTS)
target_link_libraries(OUTPUTS.TestDiagram DynaFlowLauncher::outputs)
