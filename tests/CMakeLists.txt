# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

cmake_minimum_required(VERSION 3.6)


## Googletest dependency
find_package(GTest REQUIRED)
if(NOT TARGET GTest::gtest AND TARGET GTest::GTest)
  add_library(GTest::gtest UNKNOWN IMPORTED)
  if(EXISTS "${GTEST_LIBRARY}")
    set_target_properties(GTest::gtest PROPERTIES
            IMPORTED_LOCATION "${GTEST_LIBRARY}")
  endif()
endif()
if(NOT TARGET GTest::gtest_main AND TARGET GTest::Main)
  add_library(GTest::gtest_main UNKNOWN IMPORTED)
  if(EXISTS "${GTEST_MAIN_LIBRARY}")
    set_target_properties(GTest::gtest_main PROPERTIES
            IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}")
  endif()
endif()
find_package(GMock REQUIRED)
add_library(DynaFlowLauncherTest INTERFACE)
  target_link_libraries(DynaFlowLauncherTest INTERFACE
    GTest::gmock_main
    GTest::gtest_main)

# In order to incude tests.h in all tests
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# macro to define tests
macro(DEFINE_TEST _name _module_name)
  add_executable(${_name} ${_name}.cpp)
  target_link_libraries(${_name}
    DynaFlowLauncherTest)
  target_include_directories(${_name} PRIVATE ${TEST_DIR})
  add_test(
    NAME ${_module_name}.${_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${_name}
  )
endmacro(DEFINE_TEST)


add_subdirectory(common)
add_subdirectory(inputs)
add_subdirectory(main)
