# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

if(${USE_ZIP} STREQUAL "YES")
  execute_process(COMMAND zip -j res/${TEST_NAME}.zip res/TestIIDM_${TEST_NAME}.iidm res/config_${TEST_NAME}.json)
  set(_command ${EXE} --network=TestIIDM_${TEST_NAME}.iidm --config=config_${TEST_NAME}.json --input-archive=res/${TEST_NAME}.zip)
else()
  set(_command ${EXE} --network=res/TestIIDM_${TEST_NAME}.iidm --config=res/config_${TEST_NAME}.json)
endif()
message(STATUS "Execute process: ${_command}")
execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
if(_result)
  message(FATAL_ERROR "Execution failed: ${_command}")
endif()

set(_command ${PYTHON_COMMAND} ${DIFF_SCRIPT} . ${TEST_NAME} res/config_${TEST_NAME}.json)
if(${USE_ZIP} STREQUAL "YES")
  list(APPEND _command --output-zip output.zip)
endif()
if(DEFINED DIFF_SCRIPT)
  message(STATUS "Execute process: ${_command}")
  execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
  if(_result)
    message(FATAL_ERROR "resultsTestsTmp/${TEST_NAME} files are different from expected references reference/${TEST_NAME}")
  endif()
endif()
