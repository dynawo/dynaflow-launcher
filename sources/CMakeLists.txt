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
add_subdirectory(Outputs)
add_subdirectory(Algo)

# Always list the source files explicitly, including headers so that they are listed in the IDE
# If you need to use files based on a variable value, use target_sources
set(SOURCES
  main.cpp
  Context.cpp
)

set(HEADERS
  Context.h
  gitversion_dfl.h
)

add_executable(DynaFlowLauncher ${SOURCES})

target_link_libraries(DynaFlowLauncher
  PRIVATE
  DynaFlowLauncher::outputs
  DynaFlowLauncher::algo
  DynaFlowLauncher::inputs
  DynaFlowLauncher::common

  Dynawo::dynawo_Simulation
  Dynawo::dynawo_SimulationCommon

  DynawoAlgorithms::dynawo_algorithms_Common
  DynawoAlgorithms::dynawo_algorithms_API_MultipleJobs
  DynawoAlgorithms::dynawo_algorithms_Launcher

  LibXml2::LibXml2

  Boost::serialization
)

# explicitly say that the executable depends on custom target
add_dependencies(DynaFlowLauncher revisiontag)

# obtain git revision
# a custom target that is always built
add_custom_target(revisiontag ALL)

# creates gitversion_dfl.h using cmake script
add_custom_command(TARGET revisiontag
  COMMAND ${CMAKE_COMMAND}
    -DOUTPUT_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -DGIT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/getgitheadrevision.cmake
  )

install(DIRECTORY ${CMAKE_SOURCE_DIR}/etc DESTINATION "${CMAKE_INSTALL_PREFIX}")

# The following will export the targets under the name ${PROJECT_NAME}_Targets, not install them yet
# It will then need a call to `install(EXPORT)`
install(
    TARGETS
        DynaFlowLauncher # We can install executables
    EXPORT ${PROJECT_NAME}_Targets
# Following is only needed pre-cmake3.14
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
# If you want to split between runtime and dev for examples, take a look at COMPONENT, NAMELINK_COMPONENT etc
# More info in Craig Scott's talk "Deep CMake for library authors" https://www.youtube.com/watch?v=m0DwB4OvDXk
    INCLUDES
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
