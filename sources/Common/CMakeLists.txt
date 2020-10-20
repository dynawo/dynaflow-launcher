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
src/DicoKeys.cpp
src/Dico.cpp
)

add_library(common STATIC ${SOURCES})
target_include_directories(common
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_set_warnings(common ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you

check_env_var(DYNAFLOW_LAUNCHER_LOCALE)

# We use a target to ensure it will be executed at each build
add_custom_target(dicokeys
BYPRODUCTS ${CMAKE_CURRENT_SOURCE_DIR}/src/DicoKeys.cpp ${CMAKE_CURRENT_SOURCE_DIR}/include/DicoKeys.h
COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generateMessages.py
        ${CMAKE_SOURCE_DIR}/etc/DFLMessages_${DYNAFLOW_LAUNCHER_LOCALE}.dic ${CMAKE_CURRENT_SOURCE_DIR})

add_dependencies(common dicokeys)

target_link_libraries(common
  PUBLIC
    Boost::program_options
    Dynawo::dynawo_Common

  PRIVATE
    Boost::filesystem
)
add_library(DynaFlowLauncher::common ALIAS common)
