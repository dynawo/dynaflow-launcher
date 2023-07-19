# Copyright (c) 2020-2022, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(SOURCES
src/ContingencyValidationAlgorithm.cpp
src/DynModelDefinitionAlgorithm.cpp
src/DynModelFilterAlgorithm.cpp
src/GeneratorDefinitionAlgorithm.cpp
src/HVDCDefinitionAlgorithm.cpp
src/LineDefinitionAlgorithm.cpp
src/LoadDefinitionAlgorithm.cpp
src/MainConnexComponentAlgorithm.cpp
src/SVarCDefinitionAlgorithm.cpp
src/ShuntDefinitionAlgorithm.cpp
src/SlackNodeAlgorithm.cpp
src/TransfoDefinitionAlgorithm.cpp
)

add_library(dfl_Algo SHARED ${SOURCES})

target_include_directories(dfl_Algo
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_set_warnings(dfl_Algo ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_link_libraries(dfl_Algo
PUBLIC
DynaFlowLauncher::inputs
DynaFlowLauncher::common
Boost::system
)
add_library(DynaFlowLauncher::algo ALIAS dfl_Algo)
install(TARGETS dfl_Algo
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
