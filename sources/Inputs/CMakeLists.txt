# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(SOURCES
  src/NetworkManager.cpp
  src/Node.cpp
  src/Configuration.cpp
)

add_library(inputs STATIC ${SOURCES})

target_include_directories(inputs
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(inputs
PUBLIC
DynaflowLauncher::common

Dynawo::dynawo_DataInterfaceFactory
)
add_library(DynaflowLauncher::inputs ALIAS inputs)
