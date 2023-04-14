# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

DEFINE_TEST(varEnv MAIN_SA COMMAND $<TARGET_FILE:DynaFlowLauncher> --network=res/TestIIDM_launch.iidm --config=res/config_varEnv.json --contingencies=res/contingencies_launch.json)
# Test will fail because environment variables are not set
set_property(TEST MAIN_SA.varEnv APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
set_tests_properties(MAIN_SA.varEnv PROPERTIES WILL_FAIL TRUE)

macro(DEFINE_LAUNCH_TEST_SA _name)
  DEFINE_TEST_FULLENV(${_name} MAIN_SA
                      COMMAND ${CMAKE_COMMAND}
                              -DPYTHON_COMMAND=${PYTHON_EXECUTABLE}
                              -DEXE=$<TARGET_FILE:DynaFlowLauncher>
                              -DMPI_RUN=${MPI_RUN}
                              -DTEST_NAME=${_name}
                              -DDIFF_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/diffContingencies.py
                              -DCHECK_SCRIPT=${CMAKE_SOURCE_DIR}/scripts/checkContingencies.py
                              -P ${CMAKE_SOURCE_DIR}/cmake/TestMainSA.cmake
  )
endmacro()

DEFINE_LAUNCH_TEST_SA(launch)
DEFINE_LAUNCH_TEST_SA(noContingencies)
