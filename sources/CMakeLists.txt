# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

add_subdirectory(Common)
add_subdirectory(Inputs)
add_subdirectory(Algo)

# Always list the source files explicitly, including headers so that they are listed in the IDE
# If you need to use files based on a variable value, use target_sources
set(SOURCES
  main.cpp
  Context.cpp
)

set(HEADERS
  Context.h
)

add_executable(DynaFlowLauncher ${SOURCES} ${HEADERS})

target_link_libraries(DynaFlowLauncher
  PRIVATE
  DynaFlowLauncher::common
  DynaFlowLauncher::inputs
  DynaFlowLauncher::algo

  Dynawo::dynawo_API_PAR
  Dynawo::dynawo_API_DYD
)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/etc DESTINATION ${CMAKE_INSTALL_PREFIX})
