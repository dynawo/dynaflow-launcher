# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

add_test(
    NAME MAIN.varEnv
    COMMAND DynaFlowLauncher --network=res/TestIIDM_launch_infinite.iidm --config=res/launch_infinite.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
# Test will fail because environement variables are not set
set_tests_properties(MAIN.varEnv PROPERTIES WILL_FAIL TRUE)

add_test(
    NAME MAIN.help
    COMMAND DynaFlowLauncher --help
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  add_test(
    NAME MAIN.version
    COMMAND DynaFlowLauncher --version
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

macro(DEFINE_LAUNCH_TEST _name)
  add_test(
    # Use some per-module/project prefix so that it is easier to run only tests for this module
    NAME MAIN.${_name}
    COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:DynaFlowLauncher> -DTEST_NAME=${_name} -DDIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diff.py -P ${CMAKE_SOURCE_DIR}/cmake/TestMain.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_property(TEST MAIN.${_name} PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH=${DYNAWO_HOME}/share/iidm/xsd/)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH=${DYNAWO_HOME}/lib:${LD_LIBRARY_PATH})
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_INSTALL=${CMAKE_SOURCE_DIR})
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_RESOURCES_DIR=${DYNAWO_HOME}/share:${DYNAWO_HOME}/share/xsd)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_DDB_DIR=${DYNAWO_HOME}/ddb)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_INSTALL_DIR=${DYNAWO_HOME})
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_IIDM_EXTENSION=${DYNAWO_HOME}/lib/libdynawo_DataInterfaceIIDMExtension.so)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_LIBIIDM_EXTENSIONS=${DYNAWO_HOME}/lib)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT PYTHONPATH=${DYNAWO_HOME}/sbin/nrt/nrt_diff)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LIBRARIES=${DYNAWO_HOME}/ddb)
endmacro()

DEFINE_LAUNCH_TEST(launch)

DEFINE_LAUNCH_TEST(launch_P)

DEFINE_LAUNCH_TEST(launch_infinite)

DEFINE_LAUNCH_TEST(node_breaker)
## Expected to fail because node breaker is not supported
set_tests_properties(MAIN.node_breaker PROPERTIES WILL_FAIL TRUE)

DEFINE_LAUNCH_TEST(hvdc_line_normal)

DEFINE_LAUNCH_TEST(hvdc_dangling)

DEFINE_LAUNCH_TEST(hvdc)
DEFINE_LAUNCH_TEST(distant_regulation)


DEFINE_LAUNCH_TEST(special_characters)

DEFINE_LAUNCH_TEST(phase_shifter)
