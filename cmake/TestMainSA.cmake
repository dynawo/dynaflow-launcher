# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

if(${USE_ZIP} STREQUAL "YES")
  # clean-up : delete the zip archive and the previous unzipped files
  execute_process(COMMAND rm -f res/${TEST_NAME}.zip res/TestIIDM_${TEST_NAME}.iidm res/config_${TEST_NAME}.json res/contingencies_${TEST_NAME}.json)

  execute_process(COMMAND zip -j res/${TEST_NAME}.zip res/res_to_zip/TestIIDM_${TEST_NAME}.iidm res/res_to_zip/config_${TEST_NAME}.json res/res_to_zip/contingencies_${TEST_NAME}.json)
  set(_dfl_cmd ${EXE} --network=TestIIDM_${TEST_NAME}.iidm --config=config_${TEST_NAME}.json --contingencies=contingencies_${TEST_NAME}.json --input-archive=res/${TEST_NAME}.zip)
else ()
  set(_dfl_cmd ${EXE} --network=res/TestIIDM_${TEST_NAME}.iidm --config=res/config_${TEST_NAME}.json --contingencies=res/contingencies_${TEST_NAME}.json)
endif()
if(NOT DEFINED USE_MPI OR USE_MPI STREQUAL "")
  message(FATAL_ERROR "USE_MPI is not defined")
endif()
if(${USE_MPI} STREQUAL "YES")
  if(NOT DEFINED MPI_RUN OR MPI_RUN STREQUAL "")
    message(FATAL_ERROR "mpirun is not found")
  endif()
  set(_command ${MPI_RUN} -np 4 ${_dfl_cmd})
else()
  set(_command ${_dfl_cmd})
endif()
message(STATUS "Execute process: ${_command}")
execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
if(_result)
  message(FATAL_ERROR "Execution failed: ${_command}")
endif()

set(_command ${PYTHON_COMMAND} ${DIFF_SCRIPT} . ${TEST_NAME} res/config_${TEST_NAME}.json)
if(${USE_ZIP} STREQUAL "YES")
  list(APPEND _command --output-zip outputs.zip)
endif()
if(DEFINED DIFF_SCRIPT)
  message(STATUS "Execute process: ${_command}")
  execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
  if(_result)
    message(FATAL_ERROR "resultsTestsTmp/${TEST_NAME} has some different files from reference/${TEST_NAME}")
  endif()
endif()

set(_command ${PYTHON_COMMAND} ${CHECK_SCRIPT} . ${TEST_NAME} "TestIIDM_${TEST_NAME}")
if(DEFINED CHECK_SCRIPT)
  message(STATUS "Execute process: ${_command}")
  execute_process(COMMAND ${_command} RESULT_VARIABLE _result)
  if(_result)
    message(FATAL_ERROR "resultsTestsTmp/${TEST_NAME} has some input or output files that contain errors")
  endif()
endif()
