# Copyright (c) 2022, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#
# This file is part of Dynawo, an hybrid C++/Modelica open source suite
# of simulation tools for power systems.

# Minimum required (for ExternalProject_Add)
cmake_minimum_required(VERSION 3.9.6 FATAL_ERROR)

project("dynaflow-launcher-3rd"
LANGUAGES C CXX
)

include(ExternalProject)

set(DOWNLOAD_DIR ${CMAKE_INSTALL_PREFIX}/src CACHE PATH "Directory where 3rd parties are downloaded.")
set(TMP_DIR ${CMAKE_INSTALL_PREFIX}/tmp CACHE PATH "Directory where 3rd parties temporary files are created.")

set(mpich_version "3.4.2")
if(DEFINED ENV{DYNAWO_ALGORITHMS_MPICH_DOWNLOAD_URL})
  set(mpich_prefix_url $ENV{DYNAWO_ALGORITHMS_MPICH_DOWNLOAD_URL})
else()
  set(mpich_prefix_url http://www.mpich.org/static/downloads)
endif()
set(mpich_url ${mpich_prefix_url}/${mpich_version}/mpich-${mpich_version}.tar.gz)
set(mpich_md5 6ee1cfff98728e5160c6e78bdb1986ca)

find_package(MPI QUIET)
if(MPI_FOUND)
  add_custom_target(mpich)
else()
  ExternalProject_Add(mpich
      INSTALL_DIR         ${CMAKE_INSTALL_PREFIX}/mpich
      DOWNLOAD_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/mpich
      TMP_DIR             ${TMP_DIR}
      STAMP_DIR           ${DOWNLOAD_DIR}/mpich-stamp
      SOURCE_DIR          ${DOWNLOAD_DIR}/mpich
      URL                 ${mpich_url}
      URL_MD5             ${mpich_md5}
      BUILD_IN_SOURCE     1

      CONFIGURE_COMMAND   <SOURCE_DIR>/configure
                          --prefix=<INSTALL_DIR>
                          "CC=${CMAKE_C_COMPILER}"
                          "CXX=${CMAKE_CXX_COMPILER}"
                          "CFLAGS=-m64 -O3 -DNDEBUG"
                          "CXXFLAGS=-m64 -O3 -DNDEBUG"
                          "FCFLAGS=-m64 -O3 -DNDEBUG"
                          "LDFLAGS=-m64 -O3 -DNDEBUG"
                          --with-device=ch3:nemesis
                          --disable-fortran
                          --enable-fast=all,O3,ndebug
  )
  ExternalProject_Get_Property(mpich install_dir)
  set(MPI_HOME ${install_dir})
endif()
