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
)

add_library(common STATIC ${SOURCES})
target_include_directories(common
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(common
  PUBLIC
    Boost::program_options
    Boost::filesystem
    Dynawo::dynawo_Common
)
add_library(DynaFlowLauncher::common ALIAS common)
