# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(SOURCES
src/Constants.cpp
src/Diagram.cpp
src/Dyd.cpp
src/DydDynModel.cpp
src/DydEvent.cpp
src/DydGenerator.cpp
src/DydHvdc.cpp
src/DydLoads.cpp
src/DydSVarC.cpp
src/Job.cpp
src/Network.cpp
src/Par.cpp
src/ParDynModel.cpp
src/ParEvent.cpp
src/ParGenerator.cpp
src/ParHvdc.cpp
src/ParLoads.cpp
src/ParSVarC.cpp
src/ParNetwork.cpp
src/Solver.cpp
)

add_library(outputs SHARED ${SOURCES})

target_include_directories(outputs
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_set_warnings(outputs ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_link_libraries(outputs
PUBLIC
DynaFlowLauncher::algo

Dynawo::dynawo_API_JOB
Dynawo::dynawo_API_DYD
Dynawo::dynawo_API_PAR

PRIVATE
Boost::filesystem

)
add_library(DynaFlowLauncher::outputs ALIAS outputs)
install(TARGETS outputs DESTINATION ${CMAKE_INSTALL_LIBDIR})
