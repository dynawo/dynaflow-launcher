# Copyright (c) 2022, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0

set(SRC_DIR ${PROJECT_SOURCE_DIR})
set(DOC_DIR ${docdir})

# add a target to generate API documentation with Doxygen
find_package(Doxygen 1.8)
if(DOXYGEN_FOUND)
    configure_file(doxygen.cfg.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
    set(DOXYGEN_FILE_PATTERNS *.h *.cpp *.hpp)
    set(DOXYGEN_EXTRACT_PRIVATE YES)
    add_custom_target(doc
                      ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                      COMMENT "Generating Dynaflow Launcher documentation with Doxygen" VERBATIM
                      )
else()
    add_custom_target(doc)
endif(DOXYGEN_FOUND)
