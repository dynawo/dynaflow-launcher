# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

set(SOURCES
  src/AssemblingDataBase.cpp
  src/NetworkManager.cpp
  src/Node.cpp
  src/Configuration.cpp
  src/HvdcLine.cpp
  src/Contingencies.cpp
  src/ContingenciesManager.cpp
  src/DynamicDataBaseManager.cpp
  src/SettingDataBase.cpp
)

add_library(dfl_Inputs SHARED ${SOURCES})

target_include_directories(dfl_Inputs
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_set_warnings(dfl_Inputs ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_link_libraries(dfl_Inputs
PUBLIC
DynaFlowLauncher::common
Boost::filesystem
XMLSAXParser${LibXML_LINK_SUFFIX}
LibXml2::LibXml2

PRIVATE
Dynawo::dynawo_DataInterface
Dynawo::dynawo_DataInterfaceFactory
)
add_library(DynaFlowLauncher::inputs ALIAS dfl_Inputs)
install(TARGETS dfl_Inputs
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
