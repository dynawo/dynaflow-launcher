# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

function(check_file _file _expected_file)
  execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol ${_file} ${_expected_file} RESULT_VARIABLE COMPARE_RESULT)
  if(COMPARE_RESULT)
      message(FATAL_ERROR "${_file} is different from expected ${_expected_file} are different")
  endif()
endfunction(check_file)

execute_process(COMMAND ${EXE}  --network=res/IEEE14_${TEST_NAME}.iidm --config=res/config_${TEST_NAME}.json RESULT_VARIABLE EXE_RESULT)
if(EXE_RESULT)
    message(FATAL_ERROR "Execution failed: ${EXE} --network=res/IEEE14.iidm --config=res/config_${TEST_NAME}.json")
endif()
check_file(results/${TEST_NAME}/outputs/finalState/outputIIDM.xml reference/${TEST_NAME}/outputIIDM.xml)
