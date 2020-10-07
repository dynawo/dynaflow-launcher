# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

macro(DEFINE_LAUNCH_TEST _name)
  add_test(
    # Use some per-module/project prefix so that it is easier to run only tests for this module
    NAME MAIN.${_name}
    COMMAND DynaFlowLauncher --iidm=res/IEEE14.iidm --config=res/config_${_name}.json
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_property(TEST MAIN.${_name} PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH=${DYNAWO_HOME}/share/iidm/xsd/)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_INSTALL=${CMAKE_SOURCE_DIR})
  set_property(TEST MAIN.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_RESOURCES_DIR=${DYNAWO_HOME}/share:${DYNAWO_HOME}/share/xsd)
endmacro()

DEFINE_LAUNCH_TEST(launch)
DEFINE_LAUNCH_TEST(launch_infinite)
