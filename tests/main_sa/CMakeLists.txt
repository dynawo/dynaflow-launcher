# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

add_test(
    NAME MAIN_SA.varEnv
    COMMAND DynaFlowLauncher --network=res/TestIIDM_launch.iidm --config=res/config_launch.json --contingencies=res/contingencies_launch.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
# Test will fail because environment variables are not set
set_property(TEST MAIN_SA.varEnv APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
set_tests_properties(MAIN_SA.varEnv PROPERTIES WILL_FAIL TRUE)

# specific test for case with contingecies json file with no contingencies specified
add_test(
    NAME MAIN_SA.noContingencies
    COMMAND DynaFlowLauncher --network=res/TestIIDM_launch.iidm --config=res/config_launch.json --contingencies=res/no_contingencies.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
set_property(TEST MAIN_SA.noContingencies PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH=${DYNAWO_HOME}/share/iidm/xsd/)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH=${DYNAWO_HOME}/lib:${DYNAWO_ALGORITHMS_HOME}/lib:${LD_LIBRARY_PATH})
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAWO_ALGORITHMS_LOCALE=en_GB)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_INSTALL=${CMAKE_SOURCE_DIR})
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAWO_RESOURCES_DIR=${CMAKE_SOURCE_DIR}/etc/Dictionaries:${DYNAWO_HOME}/share:${DYNAWO_HOME}/share/xsd)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAWO_DDB_DIR=${DYNAWO_HOME}/ddb)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAWO_INSTALL_DIR=${DYNAWO_HOME})
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAWO_IIDM_EXTENSION=${DYNAWO_HOME}/lib/libdynawo_DataInterfaceIIDMExtension.so)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT DYNAWO_LIBIIDM_EXTENSIONS=${DYNAWO_HOME}/lib)
set_property(TEST MAIN_SA.noContingencies APPEND PROPERTY ENVIRONMENT PYTHONPATH=${DYNAWO_HOME}/sbin/nrt/nrt_diff)

macro(DEFINE_LAUNCH_TEST_SA _name)
  add_test(
    # Use some per-module/project prefix so that it is easier to run only tests for this module
    NAME MAIN_SA.${_name}
    COMMAND ${CMAKE_COMMAND} -DEXE=$<TARGET_FILE:DynaFlowLauncher> -DTEST_NAME=${_name} -DDIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diffContingencies.py -DCHECK_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/checkContingencies.py -P ${CMAKE_SOURCE_DIR}/cmake/TestMainSA.cmake
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_property(TEST MAIN_SA.${_name} PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH=${DYNAWO_HOME}/share/iidm/xsd/)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT LD_LIBRARY_PATH=${DYNAWO_HOME}/lib:${DYNAWO_ALGORITHMS_HOME}/lib:${LD_LIBRARY_PATH})
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_ALGORITHMS_LOCALE=en_GB)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_INSTALL=${CMAKE_SOURCE_DIR})
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_RESOURCES_DIR=${CMAKE_SOURCE_DIR}/etc/Dictionaries:${DYNAWO_HOME}/share:${DYNAWO_HOME}/share/xsd)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_DDB_DIR=${DYNAWO_HOME}/ddb)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_INSTALL_DIR=${DYNAWO_HOME})
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_IIDM_EXTENSION=${DYNAWO_HOME}/lib/libdynawo_DataInterfaceIIDMExtension.so)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_LIBIIDM_EXTENSIONS=${DYNAWO_HOME}/lib)
  set_property(TEST MAIN_SA.${_name} APPEND PROPERTY ENVIRONMENT PYTHONPATH=${DYNAWO_HOME}/sbin/nrt/nrt_diff)
endmacro()

DEFINE_LAUNCH_TEST_SA(launch)
