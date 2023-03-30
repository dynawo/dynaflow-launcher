# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(TestNode INPUTS)
target_link_libraries(INPUTS.TestNode DynaFlowLauncher::inputs)

DEFINE_TEST_XML(TestSettingXmlDocument INPUTS)
target_link_libraries(INPUTS.TestSettingXmlDocument DynaFlowLauncher::inputs)

DEFINE_TEST_XML(TestAssemblingXmlDocument INPUTS)
target_link_libraries(INPUTS.TestAssemblingXmlDocument DynaFlowLauncher::inputs)

DEFINE_TEST_XML(TestDynamicDataBaseManager INPUTS)
target_link_libraries(INPUTS.TestDynamicDataBaseManager DynaFlowLauncher::inputs)
set_property(TEST INPUTS.TestDynamicDataBaseManager APPEND PROPERTY ENVIRONMENT "DYNAFLOW_LAUNCHER_XSD=${CMAKE_SOURCE_DIR}/etc/xsd")

DEFINE_TEST(TestNetworkManager INPUTS IIDM)
target_link_libraries(INPUTS.TestNetworkManager DynaFlowLauncher::inputs)

DEFINE_TEST(TestConfig INPUTS)
target_link_libraries(INPUTS.TestConfig DynaFlowLauncher::inputs)
