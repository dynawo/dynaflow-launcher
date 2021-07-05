# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(SOURCES
src/Algo.cpp
)

add_library(algo ${LINK_TYPE} ${SOURCES})

target_include_directories(algo
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_set_warnings(algo ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_link_libraries(algo
PUBLIC
DynaFlowLauncher::inputs
DynaFlowLauncher::common
Boost::system
)
add_library(DynaFlowLauncher::algo ALIAS algo)
install_lib_shared(algo)
