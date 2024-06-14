# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(_command ${EXE} --network=res/TestIIDM_${TEST_NAME}.iidm --config=res/config_${TEST_NAME}.json)
message(STATUS "Execute process: ${_command}")
execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
if(_result)
  message(FATAL_ERROR "Execution failed: ${_command}")
endif()

set(_command ${PYTHON_COMMAND} ${DIFF_SCRIPT} . ${TEST_NAME} TestIIDM_${TEST_NAME} res/config_${TEST_NAME}.json)
if(DEFINED DIFF_SCRIPT)
  message(STATUS "Execute process: ${_command}")
  execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
  if(_result)
    message(FATAL_ERROR "resultsTestsTmp/${TEST_NAME}/outputs/finalState/outputIIDM.xml is different from expected reference/${TEST_NAME}/outputIIDM.xml")
  endif()
endif()
