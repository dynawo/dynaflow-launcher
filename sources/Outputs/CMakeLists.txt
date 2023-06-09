# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(SOURCES
src/OutputsConstants.cpp
src/Diagram.cpp
src/Dyd.cpp
src/DydDynModel.cpp
src/DydEvent.cpp
src/DydGenerator.cpp
src/DydHvdc.cpp
src/DydLoads.cpp
src/DydSVarC.cpp
src/DydVRRemote.cpp
src/Job.cpp
src/Network.cpp
src/Par.cpp
src/ParDynModel.cpp
src/ParEvent.cpp
src/ParGenerator.cpp
src/ParHvdc.cpp
src/ParLoads.cpp
src/ParSVarC.cpp
src/ParVRRemote.cpp
src/Solver.cpp
)

add_library(dfl_Outputs SHARED ${SOURCES})

target_include_directories(dfl_Outputs
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_definitions(dfl_Outputs
PUBLIC
  $<$<BOOL:${MSVC}>:_USE_MATH_DEFINES>  # expose macro definitions for common math constants (e.g. M_PI)
)

target_set_warnings(dfl_Outputs ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_link_libraries(dfl_Outputs
PUBLIC
DynaFlowLauncher::algo

Dynawo::dynawo_API_JOB
Dynawo::dynawo_API_DYD
Dynawo::dynawo_API_PAR

PRIVATE
Boost::filesystem

)
add_library(DynaFlowLauncher::outputs ALIAS dfl_Outputs)
install(TARGETS dfl_Outputs
RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
