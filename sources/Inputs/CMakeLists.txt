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
  src/HvdcLine.cpp
  src/Contingencies.cpp
  src/DynamicDataBaseManager.cpp
  src/SettingXmlDocument.cpp
  src/AssemblingXmlDocument.cpp
  src/Contingencies.cpp
)

add_library(inputs ${LINK_TYPE} ${SOURCES})

target_include_directories(inputs
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_set_warnings(inputs ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you
target_link_libraries(inputs
PUBLIC
DynaFlowLauncher::common
Boost::filesystem
Dynawo::dynawo_DataInterface
XMLSAXParser${LibXML_LINK_SUFFIX}
LibXml2::LibXml2

PRIVATE
Dynawo::dynawo_DataInterfaceFactory
)
add_library(DynaFlowLauncher::inputs ALIAS inputs)
install_lib_shared(inputs)
