# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(varEnv MAIN_NSA COMMAND $<TARGET_FILE:DynaFlowLauncher> --network=res/TestIIDM_launch.iidm --config=res/config_varEnv.json --contingencies=res/contingencies_launch.json --nsa)
# Test will fail because environment variables are not set
set_property(TEST MAIN_NSA.varEnv APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
set_tests_properties(MAIN_NSA.varEnv PROPERTIES WILL_FAIL TRUE)

macro(DEFINE_LAUNCH_TEST_NSA _name)
  DEFINE_TEST_FULLENV(${_name} MAIN_NSA
                      COMMAND ${CMAKE_COMMAND}
                              -DPYTHON_COMMAND=${PYTHON_EXECUTABLE}
                              -DEXE=$<TARGET_FILE:DynaFlowLauncher>
                              -DUSE_MPI=${USE_MPI}
                              -DMPI_RUN=${MPI_RUN}
                              -DTEST_NAME=${_name}
                              -DN_DIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diff.py
                              -DSA_DIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diffContingencies.py
                              -DSA_CHECK_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/checkContingencies.py
                              -P ${CMAKE_SOURCE_DIR}/cmake/TestMainNSA.cmake
  )
endmacro()

DEFINE_LAUNCH_TEST_NSA(launch)
DEFINE_LAUNCH_TEST_NSA(noContingencies)
