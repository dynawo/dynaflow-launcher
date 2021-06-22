# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(TestNode INPUTS)
target_link_libraries(TestNode DynaFlowLauncher::inputs)

DEFINE_TEST_XML(TestSettingsXmlDocument INPUTS)
target_link_libraries(TestSettingsXmlDocument
DynaFlowLauncher::inputs
)

DEFINE_TEST_XML(TestAssemblyXmlDocument INPUTS)
target_link_libraries(TestAssemblyXmlDocument
DynaFlowLauncher::inputs
)

DEFINE_TEST_XML(TestAutomatonConfigurationManager INPUTS)
target_link_libraries(TestAutomatonConfigurationManager
DynaFlowLauncher::inputs
)
set_property(TEST INPUTS.TestAutomatonConfigurationManager PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_XSD=${CMAKE_SOURCE_DIR}/etc/xsd)

DEFINE_TEST(TestNetworkManager INPUTS)
target_link_libraries(TestNetworkManager DynaFlowLauncher::inputs)
set_property(TEST INPUTS.TestNetworkManager PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH="${DYNAWO_HOME}/share/iidm/xsd/")
set_property(TEST INPUTS.TestNetworkManager APPEND PROPERTY ENVIRONMENT DYNAWO_IIDM_EXTENSION=${DYNAWO_HOME}/lib/libdynawo_DataInterfaceIIDMExtension.so)
set_property(TEST INPUTS.TestNetworkManager APPEND PROPERTY ENVIRONMENT DYNAWO_LIBIIDM_EXTENSIONS=${DYNAWO_HOME}/lib)

DEFINE_TEST(TestConfig INPUTS)
target_link_libraries(TestConfig DynaFlowLauncher::inputs)
set_property(TEST INPUTS.TestConfig PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH="${DYNAWO_HOME}/share/iidm/xsd/")
set_property(TEST INPUTS.TestConfig APPEND PROPERTY ENVIRONMENT DYNAWO_IIDM_EXTENSION=${DYNAWO_HOME}/lib/libdynawo_DataInterfaceIIDMExtension.so)
set_property(TEST INPUTS.TestConfig APPEND PROPERTY ENVIRONMENT DYNAWO_LIBIIDM_EXTENSIONS=${DYNAWO_HOME}/lib)
