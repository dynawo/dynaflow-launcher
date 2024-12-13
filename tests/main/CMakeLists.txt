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

macro(DEFINE_LAUNCH_TEST _name _use_zip)
  DEFINE_TEST_FULLENV(${_name} MAIN
                      COMMAND ${CMAKE_COMMAND}
                              -DPYTHON_COMMAND=${PYTHON_EXECUTABLE}
                              -DEXE=$<TARGET_FILE:DynaFlowLauncher>
                              -DTEST_NAME=${_name}
                              -DDIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diff.py
                              -DUSE_ZIP=${_use_zip}
                              -P ${CMAKE_SOURCE_DIR}/cmake/TestMain.cmake
  )
endmacro()

DEFINE_LAUNCH_TEST(launch NO)
DEFINE_LAUNCH_TEST(launch_archive YES)
DEFINE_LAUNCH_TEST(launch_slack NO)
DEFINE_LAUNCH_TEST(launch_diagram NO)
DEFINE_LAUNCH_TEST(launch_diagram_tfo NO)

DEFINE_LAUNCH_TEST(launch_P NO)
DEFINE_LAUNCH_TEST(launch_kGover NO)

DEFINE_LAUNCH_TEST(launch_infinite NO NO)

DEFINE_LAUNCH_TEST(node_breaker NO)

DEFINE_LAUNCH_TEST(hvdc_line_normal NO)
DEFINE_LAUNCH_TEST(hvdc_dangling NO)
DEFINE_LAUNCH_TEST(hvdc NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQProp NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQProp_multiple_bus NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQPropSwitch NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQPropDangling NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPV_HvdcPTanPhi NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPVDangling_HvdcPTanPhiDangling NO)
DEFINE_LAUNCH_TEST(hvdc_diagrams NO)
DEFINE_LAUNCH_TEST(hvdc_diagrams_flat_start NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQProp_diagrams NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPQPropDangling_diagrams NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPV_HvdcPTanPhi_diagrams NO)
DEFINE_LAUNCH_TEST(hvdc_HvdcPVDangling_HvdcPTanPhiDangling_diagrams NO)
DEFINE_LAUNCH_TEST(hvdc_svc NO)
DEFINE_LAUNCH_TEST(hvdc_svc_side2 NO)

DEFINE_LAUNCH_TEST(distant_regulation NO)


DEFINE_LAUNCH_TEST(special_characters NO)

DEFINE_LAUNCH_TEST(phase_shifter NO)

DEFINE_LAUNCH_TEST(launch_fail_generators NO)
## Expected to fail because no generator regulates the voltage in the main connected component
set_tests_properties(MAIN.launch_fail_generators PROPERTIES WILL_FAIL TRUE)

DEFINE_LAUNCH_TEST(no_SVarC_regulation NO)

DEFINE_LAUNCH_TEST(launch_svc NO)
DEFINE_LAUNCH_TEST(launch_svc_network NO)
DEFINE_LAUNCH_TEST(launch_svc_tfo NO)
DEFINE_LAUNCH_TEST(launch_svc_infinite NO)
DEFINE_LAUNCH_TEST(launch_svc_tfo_infinite NO)
DEFINE_LAUNCH_TEST(launch_svc_regulation NO)
