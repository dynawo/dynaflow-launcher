# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

configure_file(${CMAKE_SOURCE_DIR}/cmake/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/version.h)

set(SOURCES
src/Options.cpp
src/Log.cpp
src/ZipErrorMessage.cpp
src/DFLLog_keys.cpp
src/DFLError_keys.cpp
)

set_source_files_properties(src/DFLLog_keys.cpp PROPERTIES GENERATED 1)
set_source_files_properties(include/DFLLog_keys.h PROPERTIES GENERATED 1)
set_source_files_properties(src/DFLError_keys.cpp PROPERTIES GENERATED 1)
set_source_files_properties(include/DFLError_keys.h PROPERTIES GENERATED 1)

add_library(dfl_Common SHARED ${SOURCES})
target_include_directories(dfl_Common
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_set_warnings(dfl_Common ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you

# We use a target to ensure it will be executed at each build
add_custom_target(dicokeys
BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/src/DFLLog_keys.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/DFLLog_keys.h ${CMAKE_CURRENT_SOURCE_DIR}/src/DFLError_keys.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/DFLError_keys.h
COMMAND ${Python_EXECUTABLE} ${Dynawo_SBIN_DIR}/validateDictionaries.py
        --inputDir=${CMAKE_SOURCE_DIR}/etc/Dictionaries --outputDir=${CMAKE_CURRENT_SOURCE_DIR} --namespace dfl
COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/DFLLog_keys.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/DFLLog_keys.cpp
COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/DFLLog_keys.cpp
COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/DFLLog_keys.h ${CMAKE_CURRENT_SOURCE_DIR}/include/DFLLog_keys.h
COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/DFLLog_keys.h
COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/DFLError_keys.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/DFLError_keys.cpp
COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/DFLError_keys.cpp
COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/DFLError_keys.h ${CMAKE_CURRENT_SOURCE_DIR}/include/DFLError_keys.h
COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_CURRENT_SOURCE_DIR}/DFLError_keys.h
COMMENT "Generate key files for dictionaries")

add_dependencies(dfl_Common dicokeys)

target_link_libraries(dfl_Common
  PUBLIC
    Boost::program_options
    Dynawo::dynawo_Common
    DynawoAlgorithms::dynawo_algorithms_Common

  PRIVATE
    Boost::filesystem
    libZIP::libZIP
)
add_library(DynaFlowLauncher::common ALIAS dfl_Common)
install(FILES ${CMAKE_SOURCE_DIR}/etc/Dictionaries/DFLLog_en_GB.dic ${CMAKE_SOURCE_DIR}/etc/Dictionaries/DFLError_en_GB.dic DESTINATION share)
install(TARGETS dfl_Common
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
