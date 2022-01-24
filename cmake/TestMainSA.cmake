# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#
execute_process(COMMAND ${EXE}  --network=res/TestIIDM_${TEST_NAME}.iidm --config=res/config_${TEST_NAME}.json --contingencies=res/contingencies_${TEST_NAME}.json RESULT_VARIABLE EXE_RESULT)
if(EXE_RESULT)
    message(FATAL_ERROR "Execution failed: ${EXE} --network=res/TestIIDM_${TEST_NAME}.iidm --config=res/config_${TEST_NAME}.json --contingencies=res/contingencies_${TEST_NAME}")
endif()
execute_process(COMMAND python ${DIFF_SCRIPT} -v . ${TEST_NAME} RESULT_VARIABLE COMPARE_RESULT)
if(COMPARE_RESULT)
    message(FATAL_ERROR "resultsTestsTmp/${TEST_NAME} has some different files from reference/${TEST_NAME}")
endif()
execute_process(COMMAND python ${CHECK_SCRIPT} . ${TEST_NAME} RESULT_VARIABLE CHECKS_RESULT)
if(CHECKS_RESULT)
    message(FATAL_ERROR "resultsTestsTmp/${TEST_NAME} has some input or output files that contain errors")
endif()
