# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

function(check_file _file _expected_file)
    if(NOT EXISTS ${_file})
        MESSAGE(FATAL_ERROR "Output file " ${_file} " doesn't exist")
    endif()

    configure_file(${_file} ${_file} NEWLINE_STYLE LF) # required for windows ctest
    execute_process( COMMAND ${CMAKE_COMMAND} -E compare_files ${_file} ${_expected_file}
        RESULT_VARIABLE compare_result)
    if(compare_result)
        MESSAGE(FATAL_ERROR "File " ${_file} " is different from expected file " ${_expected_file})
    endif()
endfunction()

execute_process(COMMAND ${EXE} WORKING_DIRECTORY ${WORKING_DIR} RESULT_VARIABLE cmd_result)
if(cmd_result)
    message(FATAL_ERROR "Error running: ${EXE} returns " ${cmd_result})
endif()

check_file("${TEST_OUTPUT_FILE}" "${EXPECTED_OUTPUT_FILE}")
