# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

add_test(
  # Use some per-module/project prefix so that it is easier to run only tests for this module
  NAME MAIN.launch
  COMMAND DynaFlowLauncher --iidm=res/IEEE14.iidm --config=res/config.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_property(TEST MAIN.launch PROPERTY ENVIRONMENT IIDM_XML_XSD_PATH=${DYNAWO_HOME}/share/iidm/xsd/)
set_property(TEST MAIN.launch APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
set_property(TEST MAIN.launch APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_INSTALL=${CMAKE_SOURCE_DIR})
