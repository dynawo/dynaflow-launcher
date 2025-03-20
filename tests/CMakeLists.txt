# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

cmake_minimum_required(VERSION 3.14)

if(DYNAFLOW_LAUNCHER_BUILD_TESTS)
  include(FetchContent)

  ## Googletest dependency
  set(GTEST_ROOT ${DYNAWO_HOME} CACHE PATH "Path where an installation of GoogleTest may already exist")
  set(GMOCK_HOME ${DYNAWO_HOME} CACHE PATH "Path where an installation of GoogleTest may already exist")
  find_package(GTest)
  find_package(GMock)
  if (NOT GTEST_FOUND OR NOT GMOCK_FOUND)
    message(STATUS "GTest or GMock not found : they will be downloaded")

    set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build shared libraries")
    set(gtest_force_shared_crt ON CACHE INTERNAL "Force shared crt")
    set(googletest_version   1.16.0)
    set(googletest_name      googletest-release-${googletest_version}.tar.gz)
    if(DEFINED ENV{DYNAFLOW_LAUNCHER_GOOGLETEST_DOWNLOAD_URL})
      set(googletest_prefix_url $ENV{DYNAFLOW_LAUNCHER_GOOGLETEST_DOWNLOAD_URL})
    else()
      set(googletest_prefix_url https://github.com/google/googletest/archive)
    endif()
    set(googletest_url       ${googletest_prefix_url}/v${googletest_version}.tar.gz)
    set(googletest_md5       9a75eb2ac97300cdb8b65b1a5833f411)
    FetchContent_Declare(
      googletest

      URL            ${googletest_url}
      URL_HASH       MD5=${googletest_md5}
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
    add_library(GTest::Main ALIAS gtest_main)
  endif()

  if(NOT TARGET GTest::gtest AND TARGET GTest::GTest)
    add_library(GTest::gtest UNKNOWN IMPORTED)
    if(EXISTS "${GTEST_LIBRARY_DEBUG}")
      set_target_properties(GTest::gtest PROPERTIES
              IMPORTED_LOCATION "${GTEST_LIBRARY_DEBUG}")
    elseif(EXISTS "${GTEST_LIBRARY}")
      set_target_properties(GTest::gtest PROPERTIES
        IMPORTED_LOCATION "${GTEST_LIBRARY}")
    endif()
  endif()
  if(NOT TARGET GTest::gtest_main AND TARGET GTest::Main)
    add_library(GTest::gtest_main UNKNOWN IMPORTED)
    if(EXISTS "${GTEST_MAIN_LIBRARY_DEBUG}")
      set_target_properties(GTest::gtest_main PROPERTIES
              IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY_DEBUG}")
    elseif(EXISTS "${GTEST_MAIN_LIBRARY}")
      set_target_properties(GTest::gtest_main PROPERTIES
        IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}")
    endif()
  endif()

  add_library(DynaFlowLauncherTest INTERFACE)
  target_link_libraries(DynaFlowLauncherTest INTERFACE
    GTest::gmock_main
    GTest::gtest_main)

  add_custom_target(tests)

  # In order to incude tests.h in all tests and compile Tests.cpp in all test executables
  set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR})

  if(MSVC)
    set(_lib_path "bin")
  else()
    set(_lib_path "lib")
  endif()
  set(_runtime_path
    $<TARGET_FILE_DIR:DynaFlowLauncher::common>
    $<TARGET_FILE_DIR:DynaFlowLauncher::inputs>
    $<TARGET_FILE_DIR:DynaFlowLauncher::algo>
    $<TARGET_FILE_DIR:DynaFlowLauncher::outputs>
    $<$<TARGET_EXISTS:GTest::gmock_main>:$<TARGET_FILE_DIR:GTest::gmock_main>>
    $<$<TARGET_EXISTS:GTest::Main>:$<$<NOT:$<STREQUAL:$<TARGET_PROPERTY:GTest::Main,TYPE>,INTERFACE_LIBRARY>>:$<TARGET_FILE_DIR:GTest::Main>>>
    ${GTEST_ROOT}/${_lib_path}
    ${GMOCK_HOME}/${_lib_path}
    ${DYNAWO_HOME}/${_lib_path}
    ${DYNAWO_ALGORITHMS_HOME}/${_lib_path}
  )
  if(DEFINED ENV{GTEST_ROOT})
    list(APPEND _runtime_path "$ENV{GTEST_ROOT}/${_lib_path}")
  endif()
  if(DEFINED ENV{GMOCK_HOME})
    list(APPEND _runtime_path "$ENV{GMOCK_HOME}/${_lib_path}")
  endif()
  list(REMOVE_DUPLICATES _runtime_path)
  unset(_lib_path)

  unset(runtime_tests_ENV)
  if(MSVC)
    list(APPEND _runtime_path $ENV{PATH})
    string(REPLACE ";" "\\;" _runtime_path "${_runtime_path}")
    set(runtime_tests_ENV "PATH=${_runtime_path}")
  else()
    string(REPLACE ";" ":" _runtime_path "${_runtime_path}")
    set(runtime_tests_ENV "LD_LIBRARY_PATH=${_runtime_path}:$ENV{LD_LIBRARY_PATH}")
  endif()
  unset(_runtime_path)
endif()

# macro to define tests
macro(DEFINE_TEST _name _module_name)
  # message(STATUS "DEFINE_TEST ${_module_name}.${_name} ${ARGN}")

  set(_with_xml)
  set(_with_iidm)
  set(_with_command)
  set(_command)
  foreach(_optional_arg ${ARGN})
    if (_with_command)
      list(APPEND _command ${_optional_arg})
    elseif("${_optional_arg}" STREQUAL XML)
      set(_with_xml ON)
    elseif("${_optional_arg}" STREQUAL IIDM)
      set(_with_iidm ON)
    elseif("${_optional_arg}" STREQUAL COMMAND)
      set(_with_command ON)
    endif()
  endforeach()

  if(DYNAFLOW_LAUNCHER_BUILD_TESTS)
    if (NOT _with_command)
      add_executable(${_module_name}.${_name}
        ${_name}.cpp
        ${TEST_DIR}/Tests.cpp
        $<$<BOOL:${_with_xml}>:${TEST_DIR}/initXml.cpp>
      )
      set_target_properties(${_module_name}.${_name} PROPERTIES OUTPUT_NAME ${_name})

      target_link_libraries(${_module_name}.${_name}
       PRIVATE
        DynaFlowLauncherTest
        "$<$<BOOL:${_with_xml}>:XMLSAXParser${LibXML_LINK_SUFFIX};LibXml2::LibXml2>"
      )
      target_include_directories(${_module_name}.${_name} PRIVATE ${TEST_DIR})

      set(_command $<TARGET_FILE:${_module_name}.${_name}>)
    endif()
  else()
    set(_command ${CMAKE_COMMAND} -E echo "${_module_name}.${_name} test is not built !")
  endif()
  if(NOT TARGET ${_module_name}.${_name})
    add_custom_target(${_module_name}.${_name})
    if(DYNAFLOW_LAUNCHER_BUILD_TESTS)
      add_dependencies(${_module_name}.${_name} DynaFlowLauncher)
    endif()
  endif()

  if(DYNAFLOW_LAUNCHER_BUILD_TESTS)
    if(${_module_name}.${_name} MATCHES "${DYNAFLOW_LAUNCHER_REGEX_TESTS}")
      add_dependencies(tests ${_module_name}.${_name})
    endif()
  endif()

  add_test(
    NAME ${_module_name}.${_name}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${_command}
  )

  if(DYNAFLOW_LAUNCHER_BUILD_TESTS)
    set_property(TEST ${_module_name}.${_name} PROPERTY ENVIRONMENT "${runtime_tests_ENV}")

    if(_with_iidm)
      set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT "DYNAWO_LIBIIDM_EXTENSIONS=${DYNAWO_HOME}/$<IF:$<BOOL:${MSVC}>,bin,lib>")
    endif()
  endif()
endmacro(DEFINE_TEST)

if(NOT DYNAFLOW_LAUNCHER_BUILD_TESTS)
  macro(add_library)
  endmacro()
  macro(set_target_properties)
  endmacro()
  macro(target_link_libraries)
  endmacro()
  macro(add_dependencies)
  endmacro()
endif()

macro(DEFINE_TEST_XML _name _module_name)
  DEFINE_TEST(${_name} ${_module_name} XML ${ARGN})
endmacro(DEFINE_TEST_XML)

macro(DEFINE_TEST_FULLENV _name _module_name)
  DEFINE_TEST(${_name} ${_module_name} IIDM ${ARGN})

  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT PYTHONPATH=${DYNAWO_HOME}/sbin/nrt/nrt_diff)
  if(MSVC)
    set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT "DYNAWO_RESOURCES_DIR=${CMAKE_SOURCE_DIR}/etc/Dictionaries\;${DYNAWO_HOME}/share\;${DYNAWO_HOME}/share/xsd")
  else()
    set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_RESOURCES_DIR=${CMAKE_SOURCE_DIR}/etc/Dictionaries:${DYNAWO_HOME}/share:${DYNAWO_HOME}/share/xsd)
  endif()
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_HOME=${DYNAFLOW_LAUNCHER_HOME})
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_DICTIONARIES=dictionaries_mapping)
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_DDB_DIR=${DYNAWO_HOME}/ddb)
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_INSTALL_DIR=${DYNAWO_HOME})
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAWO_ALGORITHMS_LOCALE=en_GB)
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LOCALE=en_GB)
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_INSTALL=${CMAKE_SOURCE_DIR})
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_BUILD_TYPE=${CMAKE_BUILD_TYPE})
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_LIBRARIES=${DYNAWO_HOME}/ddb)
  set_property(TEST ${_module_name}.${_name} APPEND PROPERTY ENVIRONMENT DYNAFLOW_LAUNCHER_XSD=${CMAKE_SOURCE_DIR}/etc/xsd)
endmacro()

add_subdirectory(common)
add_subdirectory(inputs)
add_subdirectory(algo)
add_subdirectory(outputs)
add_subdirectory(main)
add_subdirectory(main_sa)
add_subdirectory(main_n_sa)
