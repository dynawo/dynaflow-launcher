# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(varEnv MAIN COMMAND $<TARGET_FILE:DynaFlowLauncher> --network=res/TestIIDM_launch_infinite.iidm --config=res/config_launch_infinite.json)
# Test will fail because environment variables are not set
set_property(TEST MAIN.varEnv APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
set_tests_properties(MAIN.varEnv PROPERTIES WILL_FAIL TRUE)

DEFINE_TEST(help MAIN COMMAND $<TARGET_FILE:DynaFlowLauncher> --help)

DEFINE_TEST(version MAIN COMMAND $<TARGET_FILE:DynaFlowLauncher> --version)

macro(DEFINE_LAUNCH_TEST _name)
  DEFINE_TEST_FULLENV(${_name} MAIN
                      COMMAND ${CMAKE_COMMAND}
                              -DPYTHON_COMMAND=${PYTHON_EXECUTABLE}
                              -DEXE=$<TARGET_FILE:DynaFlowLauncher>
                              -DTEST_NAME=${_name}
                              -DDIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diff.py
                              -P ${CMAKE_SOURCE_DIR}/cmake/TestMain.cmake
  )
endmacro()

DEFINE_LAUNCH_TEST(launch)
DEFINE_LAUNCH_TEST(launch_slack)
DEFINE_LAUNCH_TEST(launch_diagram)
DEFINE_LAUNCH_TEST(launch_diagram_tfo)

DEFINE_LAUNCH_TEST(launch_P)

DEFINE_LAUNCH_TEST(launch_infinite)

DEFINE_LAUNCH_TEST(node_breaker)

DEFINE_LAUNCH_TEST(hvdc_line_normal)
DEFINE_LAUNCH_TEST(hvdc_dangling)
DEFINE_LAUNCH_TEST(hvdc)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQProp)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQProp_multiple_bus)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQPropSwitch)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQPropDangling)
DEFINE_LAUNCH_TEST(hvdc_HvdcPV_HvdcPTanPhi)
DEFINE_LAUNCH_TEST(hvdc_HvdcPVDangling_HvdcPTanPhiDangling)
DEFINE_LAUNCH_TEST(hvdc_diagrams)
DEFINE_LAUNCH_TEST(hvdc_diagrams_flat_start)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQProp_diagrams)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQPropDangling_diagrams)
DEFINE_LAUNCH_TEST(hvdc_HvdcPV_HvdcPTanPhi_diagrams)
DEFINE_LAUNCH_TEST(hvdc_HvdcPVDangling_HvdcPTanPhiDangling_diagrams)
DEFINE_LAUNCH_TEST(hvdc_svc)
DEFINE_LAUNCH_TEST(hvdc_svc_side2)

DEFINE_LAUNCH_TEST(distant_regulation)


DEFINE_LAUNCH_TEST(special_characters)

DEFINE_LAUNCH_TEST(phase_shifter)

DEFINE_LAUNCH_TEST(launch_fail_generators)
## Expected to fail because no generator regulates the voltage in the main connected component
set_tests_properties(MAIN.launch_fail_generators PROPERTIES WILL_FAIL TRUE)

DEFINE_LAUNCH_TEST(no_SVarC_regulation)

DEFINE_LAUNCH_TEST(launch_svc)
DEFINE_LAUNCH_TEST(launch_svc_network)
DEFINE_LAUNCH_TEST(launch_svc_tfo)
DEFINE_LAUNCH_TEST(launch_svc_infinite)
DEFINE_LAUNCH_TEST(launch_svc_tfo_infinite)
DEFINE_LAUNCH_TEST(launch_svc_regulation)
