# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

cmake_minimum_required(VERSION 3.12)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

set(USE_MPI "YES" CACHE STRING "Use MPI")
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel (CMake defaults)")

# Put the project early since modules might need to detect the compiler.
# More information https://cmake.org/cmake/help/latest/command/project.html
project(
    "DynaFlowLauncher" # This will exposed as the variable PROJECT_NAME.
    VERSION 1.7.1 # Used for installation and defines variables PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR, PROJECT_VERSION_PATCH, and PROJECT_VERSION_TWEAK.
    LANGUAGES C CXX # Used to determine the languages to use based on file extensions
)

############################
##  Modules and scripts   ##
############################

# Standard CMake modules

include(CTest)                          # Must be called before adding tests but after calling project(). This automatically calls enable_testing() and configures ctest targets when using Make/Ninja
include(CMakeDependentOption)           # This is a really useful scripts that creates options that depends on other options. It can even be used with generator expressions !
include(GNUInstallDirs)                 # This will define the default values for installation directories (all platforms even if named GNU)
if(CMAKE_BUILD_TYPE MATCHES "^Debug$")
  set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)       # Install the debug runtime libraries
  set(CMAKE_INSTALL_DEBUG_LIBRARIES_ONLY TRUE)  # Do not install the release runtime libraries
endif()
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)          # Install the Windows Universal CRT libraries
include(InstallRequiredSystemLibraries) # Tell CMake that the `install` target needs to install required system libraries (eg: Windows SDK)
include(CMakePackageConfigHelpers)      # Helper to create relocatable packages

# Custom modules and scripts

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake") # Make our cmake scripts available

include(Warnings)
include(Coverage)
include(Policy)
include(CheckVar)
include(BuildType)

###############
## POLICIES ###
###############

policy(CMP0054 NEW) # Only interpret if() arguments as variables or keywords when unquoted.
policy(CMP0074 NEW) # find_package uses PackageName_ROOT variables.
policy(CMP0075 NEW) # honor CMAKE_REQUIRED_LIBRARIES in the include file check macros

###############
##  OPTIONS  ##
###############

# You should try to give as much control over the project setup to the user.
# When modifying compile flags for example, if they are not mandatory, provide an option.

set(FORCE_CXX11_ABI OFF CACHE BOOL "Choose either ON or OFF.")

# Use your own option for tests, in case people use your library through add_subdirectory
cmake_dependent_option(DYNAFLOW_LAUNCHER_BUILD_TESTS
    "Enable ${PROJECT_NAME} project tests targets" ON # By default we want tests if CTest is enabled
    "BUILD_TESTING" OFF # Stay coherent with CTest variables
)

if(FORCE_CXX11_ABI)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=1")
endif()
set_optimization()


## External dependencies
## Dynawo and sub-dependencies
check_env_var(DYNAWO_HOME) # Correspond to deploy installation
get_filename_component(DYNAWO_HOME ${DYNAWO_HOME} ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}")
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}/cmake")
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}/share")
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_HOME}/share/cmake")
list(APPEND CMAKE_MODULE_PATH "${DYNAWO_HOME}/cmake")
list(APPEND CMAKE_MODULE_PATH "${DYNAWO_HOME}/share/cmake")
find_path(Dynawo_SBIN_DIR NAME buildChecker.py HINTS ${DYNAWO_HOME}/sbin NO_DEFAULT_PATH)
mark_as_advanced(Dynawo_SBIN_DIR)

# Dynawo algorithms
check_env_var(DYNAWO_ALGORITHMS_HOME) # Correspond to deploy installation
get_filename_component(DYNAWO_ALGORITHMS_HOME ${DYNAWO_ALGORITHMS_HOME} ABSOLUTE)
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_ALGORITHMS_HOME}")
list(APPEND CMAKE_PREFIX_PATH "${DYNAWO_ALGORITHMS_HOME}/share")
list(APPEND CMAKE_MODULE_PATH "${DYNAWO_ALGORITHMS_HOME}/share")

# ZLIB
find_package(ZLIB 1.2.3 REQUIRED)
set_property(TARGET ZLIB::ZLIB APPEND PROPERTY
  IMPORTED_LOCATION "${ZLIB_LIBRARY}")
# LibArchive
find_package(LibArchive 2.8.0 REQUIRED)
# KLU
find_package(SuiteSparse REQUIRED)
# Sundials
find_package(Sundials REQUIRED)
# libZIP
find_package(libZIP 1.3.0 REQUIRED)
# XercesC (required for libxml)
set(CXX_STDFLAG "-std=c++11") # Required to test Xerces compilation
find_package(XercesC 3.2.2 REQUIRED)
# libXML
find_package(libXML 0.2.4 REQUIRED)
find_package(LibXml2 "2.9" REQUIRED)
find_package(LibIIDM "1.5.1" REQUIRED)
# Boost
# Use only BOOST_ROOT by default to avoid conflicts between dynawo boost and default install
option(DYNAFLOW_LAUNCHER_USE_BOOST_DEFAULT "Use default system boost installation" OFF)
if(DYNAFLOW_LAUNCHER_USE_BOOST_DEFAULT)
  set(Boost_NO_SYSTEM_PATHS FALSE)
else()
  set(Boost_NO_SYSTEM_PATHS TRUE)
  # Use Boost_NO_BOOST_CMAKE to ensure that find_package uses MODULE mode and takes Boost_NO_SYSTEM_PATHS into account, instead of CONFIG mode
  set(Boost_NO_BOOST_CMAKE ON)
endif()
# Tell BOOST to link all libraries as DLL's rather than static libraries on Microsoft Windows
add_definitions(-DBOOST_ALL_DYN_LINK)
# Tell BOOST to disable auto linking
add_definitions(-DBOOST_ALL_NO_LIB)
find_package(Boost 1.70.0 REQUIRED COMPONENTS program_options filesystem system log serialization date_time)

# install target is deploy too !
option(DYNAFLOW_LAUNCHER_DEPLOY "Deploy dynawo and dynawo-algorithms while install dynaflow-launcher" ON)
if(DYNAFLOW_LAUNCHER_DEPLOY)
  install(DIRECTORY ${DYNAWO_HOME}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/ USE_SOURCE_PERMISSIONS)
  install(DIRECTORY ${DYNAWO_ALGORITHMS_HOME}/ DESTINATION ${CMAKE_INSTALL_PREFIX}/ USE_SOURCE_PERMISSIONS)
endif()

#concatenate dictionaries_mapping files
function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()
function(cat_filter IN_FILE OUT_FILE FILTER)
  file(STRINGS ${IN_FILE} CONTENTS)
  foreach(LINE ${CONTENTS})
    if (${LINE} MATCHES "${FILTER}")
      file(APPEND ${OUT_FILE} "${LINE}\n")
    endif()
  endforeach()
endfunction()
file(WRITE dictionaries_mapping.dic "")
cat(${DYNAWO_HOME}/share/dictionaries_mapping.dic dictionaries_mapping.dic)
cat_filter(${DYNAWO_ALGORITHMS_HOME}/share/dictionaries_mapping.dic dictionaries_mapping.dic "Algorithms")
file(APPEND dictionaries_mapping.dic "DFLLog                          = LOG\n")
file(APPEND dictionaries_mapping.dic "DFLError                        = ERROR\n")
install(FILES dictionaries_mapping.dic DESTINATION  ${CMAKE_INSTALL_PREFIX}/share/)

# MPI (required by algorithms)
if(USE_MPI STREQUAL "YES")
  set(MPI_HOME ${DYNAFLOW_LAUNCHER_THIRD_PARTY_DIR}/mpich)
  set(MPI_ROOT ${MPI_HOME})
  find_package(MPI REQUIRED)
  if(MPI_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MPI_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MPI_CXX_FLAGS}")
  endif()
  find_program(MPI_RUN NAMES mpirun mpiexec.exe PATHS ${DYNAFLOW_LAUNCHER_THIRD_PARTY_DIR}/mpich/bin REQUIRED NO_CMAKE_PATH)
  message(STATUS "mpirun found: " ${MPI_RUN})
  add_compile_definitions(_MPI_)
endif()

## Dynawo
find_package(Dynawo 1.7.1 REQUIRED)
find_package(DynawoAlgorithms 1.7.1 REQUIRED)

# Python Interpreter
find_package(PythonInterp REQUIRED)
if (DEFINED DYNAWO_PYTHON_COMMAND AND NOT DYNAWO_PYTHON_COMMAND STREQUAL "")
  set(PYTHON_EXECUTABLE ${DYNAWO_PYTHON_COMMAND})
  separate_arguments(PYTHON_EXECUTABLE)
endif()
message(STATUS "Python command used: ${PYTHON_EXECUTABLE}")

##

# It is always easier to navigate in an IDE when projects are organized in folders.
# set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Whe building a shared library, you do not want to export all symbols by default
# gcc (and hence clang) are wrong about this.
#
# For more information, see https://gcc.gnu.org/wiki/Visibility and https://www.youtube.com/embed/m0DwB4OvDXk
#
# Because shared libraries here are not too big, the code is simplified if we export all symbols
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)
# set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)    # export all symbols from dll on Windows
set(CMAKE_ENABLE_EXPORTS ON)                # export all symbols from exe on Windows

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

###############
##  Project  ##
###############

include(cpplint/cpplint.cmake)

#===============================#
#  DynaFlowLauncher executable  #
#===============================#

add_subdirectory(sources)

# CMake scripts extensions
target_set_warnings(DynaFlowLauncher ENABLE ALL AS_ERROR ALL DISABLE Annoying) # Helper that can set default warning flags for you

# Setup our project as the startup project for Visual so that people don't need to do it manually
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT DynaFlowLauncher)

#===========#
#   Tests   #
#===========#

# In a real project you most likely want to exclude test folders
list(APPEND CUSTOM_COVERAGE_EXCLUDE "/tests/")
add_subdirectory(tests)
# You can setup some custom variables and add them to the CTestCustom.cmake.in template to have custom ctest settings
# For example, you can exclude some directories from the coverage reports such as third-parties and tests
configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/cmake/CTestCustom.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake
    @ONLY
)

#############
## Doxygen ##
#############

set(DOCDIR_NAME doxygen)
get_filename_component(docdir ${CMAKE_INSTALL_PREFIX}/${DOCDIR_NAME} ABSOLUTE)
add_subdirectory(doxygen)


###############
## Packaging ##
###############

# Let users choose where to install the cmake package descriptions
# For that we make use of the CMake Cache
set(${PROJECT_NAME}_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "Path to install ${PROJECT_NAME} Config*.cmake files to.")
set(${PROJECT_NAME}_MODULE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake"               CACHE STRING "Path to install ${PROJECT_NAME}'s .cmake  module files to.")

# Use version checking helper provided by CMake so that users can safely use a version number in their find_package calls
write_basic_package_version_file(
    ${PROJECT_NAME}ConfigVersion.cmake # The name of the version file needed by find_package.
    VERSION ${PROJECT_VERSION}         # The version of the project, already set by the `project` command at the top of this file
    COMPATIBILITY SameMajorVersion     # We use semantic versioning, backward compatibity is only guaranteed for a same major version
)


# We will need our own file if we have our own dependencies or want some special behavior when the user calls find_package
# otherwise we could simply install the exports as the ${PROJECT_NAME}Config.cmake
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in # This is your template file
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake          # This is the resulting file
    INSTALL_DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}    # This is where the file will be installed
    # List of paths that needs to be relocated once installed
    # For example if the variable containing the path is named MY_PATH, all instances of @PACKAGE_MY_PATH@ in the template will be replaced by the relocated version of the path
    # This is mostly only needed when you want to install cmake modules or have an unusual layout that cmake is not aware of.
    PATH_VARS ${PROJECT_NAME}_MODULE_INSTALL_DIR # This will be exposed as @PACKAGE_DynawoLauncher_MODULE_INSTALL_DIR@ in the template file
    # Imported targets do not require the following macros
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# This time, install all the exported targets under the ${PROJECT_NAME}_Targets name.
install(
    EXPORT ${PROJECT_NAME}_Targets
    NAMESPACE ${PROJECT_NAME}::        # Always specify a namespace so that users can make sure they link targets with transitive properties and not only the library
    FILE ${PROJECT_NAME}Targets.cmake  # This is the file that needs to be included from your *Config.cmake. Otherwise, you could just make this your actual *Config.cmake file.
    DESTINATION ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
)

# So far we only installed the exported targets, now install the package config files.
# If you do not list headers in the PUBLIC_HEADER property, you will need to copy them using `install(FILES)` or `install(DIRECTORY)` too.
# In that case, you can use CMAKE_INSTALL_INCLUDEDIR as the base destination path.
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION
        ${${PROJECT_NAME}_INSTALL_CMAKEDIR}
)

if(MSVC)
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/dynaflow-launcher.cmd DESTINATION .)
else()  # Linux
  install(PROGRAMS ${CMAKE_SOURCE_DIR}/scripts/dynaflow-launcher.sh DESTINATION .)
endif()
