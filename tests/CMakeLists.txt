# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

cmake_minimum_required(VERSION 3.14)

include(FetchContent)

## Googletest dependency
set(GTEST_ROOT ${DYNAWO_HOME} CACHE PATH "Path where a GoogleTest installation already exists")
find_package(GTest)
find_package(GMock)
if (NOT GTEST_FOUND OR NOT GMOCK_FOUND)
  message(STATUS "GTest or GMock not found : they will be downloaded")

  set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")
  set(gtest_force_shared_crt ON CACHE INTERNAL "Force shared crt")
  FetchContent_Declare(
    googletest

    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.8.1
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
  )

  FetchContent_GetProperties(googletest)
  if(NOT googletest_POPULATED)
    FetchContent_Populate(googletest)
    # we do not want googletest to be installed with the rest of DFL as googletest is used only for unit tests
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()
  add_library(GTest::gtest_main ALIAS gtest_main)
  add_library(GTest::gmock_main ALIAS gmock_main)
endif()

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
add_library(DynaFlowLauncherTest INTERFACE)
target_link_libraries(DynaFlowLauncherTest INTERFACE
  GTest::gmock_main
  GTest::gtest_main)

# In order to incude tests.h in all tests and compile Tests.cpp in all test executables
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# macro to define tests
macro(DEFINE_TEST _name _module_name)
  add_executable(${_name}
  ${_name}.cpp
  ${TEST_DIR}/Tests.cpp)

  target_link_libraries(${_name}
    DynaFlowLauncherTest)
  target_include_directories(${_name} PRIVATE ${TEST_DIR})
  add_test(
    NAME ${_module_name}.${_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${_name}
  )
endmacro(DEFINE_TEST)

macro(DEFINE_TEST_XML _name _module_name)
  add_executable(${_name}
  ${_name}.cpp
  ${TEST_DIR}/Tests.cpp
  ${TEST_DIR}/initXml.cpp)

  target_link_libraries(${_name}
    DynaFlowLauncherTest
    XMLSAXParser${LibXML_LINK_SUFFIX}
    LibXml2::LibXml2)
  target_include_directories(${_name} PRIVATE ${TEST_DIR})
  add_test(
    NAME ${_module_name}.${_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${_name}
  )
endmacro(DEFINE_TEST_XML)

add_subdirectory(common)
add_subdirectory(inputs)
add_subdirectory(algo)
add_subdirectory(outputs)
add_subdirectory(main)
