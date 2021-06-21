#!/bin/bash
#
# Copyright (c) 2021, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

# This file aimes to encapsulate processing for users to compiler and use DFL

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

error_exit() {
    echo "${1:-"Unknown Error"}" 1>&2
    exit 1
}

export_var_env() {
    local var="$@"
    local name=${var%%=*}
    local value="${var#*=}"

    if ! `expr $name : "DYNAFLOW_LAUNCHER_.*" > /dev/null`; then
      error_exit "You must export variables with DYNAFLOW_LAUNCHER_ prefix for $name."
    fi

    if eval "[ \"\$$name\" ]"; then
      eval "value=\${$name}"
      ##echo "Environment variable for $name already set : $value"
      return
    fi

    if [ "$value" = UNDEFINED ]; then
      error_exit "You must define the value of $name"
    fi
    export $name="$value"
}

export_var_env_force() {
  local var="$@"
  local name=${var%%=*}
  local value="${var#*=}"

  if ! `expr $name : "DYNAFLOW_LAUNCHER_.*" > /dev/null`; then
    error_exit "You must export variables with DYNAFLOW_LAUNCHER prefix for $name."
  fi

  if eval "[ \"\$$name\" ]"; then
    unset $name
    export $name="$value"
    return
  fi

  if [ "$value" = UNDEFINED ]; then
    error_exit "You must define the value of $name"
  fi
  export $name="$value"
}

ld_library_path_remove() {
  export LD_LIBRARY_PATH=`echo -n $LD_LIBRARY_PATH | awk -v RS=: -v ORS=: '$0 != "'$1'"' | sed 's/:$//'`;
}

ld_library_path_prepend() {
  if [ ! -z "$LD_LIBRARY_PATH" ]; then
    ld_library_path_remove $1
    export LD_LIBRARY_PATH="$1:$LD_LIBRARY_PATH"
  else
    export LD_LIBRARY_PATH="$1"
  fi
}

pythonpath_remove() {
  export PYTHONPATH=`echo -n $PYTHONPATH | awk -v RS=: -v ORS=: '$0 != "'$1'"' | sed 's/:$//'`;
}

pythonpath_prepend() {
  if [ ! -z "$PYTHONPATH" ]; then
    pythonpath_remove $1
    export PYTHONPATH="$1:$PYTHONPATH"
  else
    export PYTHONPATH="$1"
  fi
}

define_options() {
    export_var_env DYNAFLOW_LAUNCHER_USAGE="Usage: `basename $0` [option] -- program to deal with DynaFlow Launcher

where [option] can be:
        =========== Build
        build-user                              build DynaFlow Launcher
        clean-build-all                         Clean and rebuild DynaFlow Launcher
        build-tests-coverage                    build DynaFlow Launcher and run coverage

        =========== Tests
        tests                                   launch DynaFlow Launcher unit tests
        update-references                       update MAIN tests references

        =========== Launch
        launch [network] [config]               launch DynaFlow Launcher:
                                                - network: filepath (only IIDM is supported)
                                                - config: filepath (JSON configuration file)

        =========== Others
        help                                    show all available options
        version                                 show DynaFlow Launcher version
        reset-environment                       reset all environment variables set by DynaFlow Launcher
"
}

help() {
    define_options
    echo "$DYNAFLOW_LAUNCHER_USAGE"
}

set_commit_hook() {
    $HERE/set_commit_hook.sh
}

set_environment() {
    # global vars
    ld_library_path_prepend $DYNAWO_INSTALL_DIR/lib         # For Dynawo library
    ld_library_path_prepend $DYNAFLOW_LAUNCHER_HOME/lib64   # For local DFL libraries, used only at runtime in case we compile in shared

    # dynawo vars
    export DYNAWO_INSTALL_DIR=$DYNAWO_HOME
    export IIDM_XML_XSD_PATH=$DYNAWO_INSTALL_DIR/share/iidm/xsd
    export DYNAWO_RESOURCES_DIR=$DYNAWO_INSTALL_DIR/share:$DYNAWO_INSTALL_DIR/share/xsd
    export DYNAWO_DDB_DIR=$DYNAWO_INSTALL_DIR/ddb
    export DYNAWO_IIDM_EXTENSION=$DYNAWO_INSTALL_DIR/lib/libdynawo_DataInterfaceIIDMExtension.so
    export DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_INSTALL_DIR/lib

    # build
    export_var_env_force DYNAFLOW_LAUNCHER_BUILD_DIR=$DYNAFLOW_LAUNCHER_HOME/buildLinux
    export_var_env_force DYNAFLOW_LAUNCHER_INSTALL_DIR=$DYNAFLOW_LAUNCHER_HOME/installLinux
    export_var_env DYNAFLOW_LAUNCHER_SHARED_LIB=OFF # same default value as cmakelist
    export_var_env DYNAFLOW_LAUNCHER_USE_DOXYGEN=ON # same default value as cmakelist
    export_var_env DYNAFLOW_LAUNCHER_BUILD_TESTS=ON # same default value as cmakelist
    export_var_env DYNAFLOW_LAUNCHER_CMAKE_GENERATOR="Unix Makefiles"
    export_var_env DYNAFLOW_LAUNCHER_PROCESSORS_USED=1

    # Run
    export_var_env_force DYNAFLOW_LAUNCHER_INSTALL=$DYNAFLOW_LAUNCHER_INSTALL_DIR
    export_var_env DYNAFLOW_LAUNCHER_LOG_LEVEL=INFO # INFO by default

    # python
    pythonpath_prepend $DYNAWO_HOME/sbin/nrt/nrt_diff

    # hooks
    set_commit_hook
}

reset_environment_variables() {
    for var in $(printenv | grep DYNAFLOW_LAUNCHER_ | cut -d '=' -f 1); do
        unset $var
    done

    ld_library_path_remove $DYNAFLOW_LAUNCHER_HOME/lib64
    ld_library_path_remove $DYNAFLOW_LAUNCHER_HOME/lib

    pythonpath_remove $DYNAWO_HOME/sbin/nrt/nrt_diff

    unset DYNAWO_HOME
    unset DYNAWO_INSTALL_DIR
    unset DYNAWO_RESOURCES_DIR
    unset DYNAWO_DDB_DIR
    unset DYNAWO_IIDM_EXTENSION
    unset DYNAWO_LIBIIDM_EXTENSIONS
    unset IIDM_XML_XSD_PATH
}

clean() {
    rm -rf $DYNAFLOW_LAUNCHER_BUILD_DIR
    rm -rf $DYNAFLOW_LAUNCHER_INSTALL_DIR
}

cmake_configure() {
    mkdir -p $DYNAFLOW_LAUNCHER_BUILD_DIR
    pushd $DYNAFLOW_LAUNCHER_BUILD_DIR > /dev/null
    cmake $DYNAFLOW_LAUNCHER_HOME \
        -G "$DYNAFLOW_LAUNCHER_CMAKE_GENERATOR" \
        -DCMAKE_BUILD_TYPE:STRING=$DYNAFLOW_LAUNCHER_BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX:STRING=$DYNAFLOW_LAUNCHER_INSTALL_DIR \
        -DDYNAWO_HOME:STRING=$DYNAWO_HOME \
        -DBOOST_ROOT:STRING=$DYNAWO_HOME \
        -DDYNAFLOW_LAUNCHER_LOCALE:STRING=$DYNAFLOW_LAUNCHER_LOCALE \
        -DDYNAFLOW_LAUNCHER_SHARED_LIB:BOOL=$DYNAFLOW_LAUNCHER_SHARED_LIB \
        -DDYNAFLOW_LAUNCHER_USE_DOXYGEN:BOOL=$DYNAFLOW_LAUNCHER_USE_DOXYGEN \
        -DDYNAFLOW_LAUNCHER_BUILD_TESTS:BOOL=$DYNAFLOW_LAUNCHER_BUILD_TESTS
    popd > /dev/null
}

cmake_build() {
    cmake --build $DYNAFLOW_LAUNCHER_BUILD_DIR --target install -j $DYNAFLOW_LAUNCHER_PROCESSORS_USED
}

cmake_tests() {
    pushd $DYNAFLOW_LAUNCHER_BUILD_DIR > /dev/null
    ctest -j $DYNAFLOW_LAUNCHER_PROCESSORS_USED --output-on-failure
    popd > /dev/null
}

cmake_coverage() {
    ctest \
        -S cmake/CTestScript.cmake \
        -DDYNAWO_HOME=$DYNAWO_HOME \
        -DBOOST_ROOT=$DYNAWO_HOME \
        -VV
}

clean_build_all() {
    clean
    build_user
}

build_user() {
    cmake_configure
    cmake_build
}

build_tests_coverage() {
    rm -rf $DYNAFLOW_LAUNCHER_HOME/buildCoverage
    cmake_coverage
}

launch() {
    if [ ! -f $1 ]; then
        error_exit "IIDM network file $network doesn't exist"
    fi
    if [ ! -f $2 ]; then
        error_exit "DFL configuration file $config doesn't exist"
    fi
    $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher \
    --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
    --network $1 \
    --config $2
}

version() {
    $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --version
}

update_references() {
    $HERE/updateMainReference.py
}

#################################
########### Main script #########
#################################

set_environment

case $1 in
    build-user)
        build_user || error_exit "Failed to build DFL"
        ;;
    build-tests-coverage)
        build_tests_coverage || error_exit "Failed to perform coverage"
        ;;
    clean-build-all)
        clean_build_all || error_exit "Failed to clean build DFL"
        ;;
    help)
        help
        ;;
    launch)
        launch $2 $3 || error_exit "Failed to perform launch with network=$2, config=$3"
        ;;
    reset-environment)
        reset_environment_variables || error_exit "Failed to reset environment variables"
        ;;
    tests)
        cmake_tests || error_exit "Failed to perform tests"
        ;;
    update-references)
        update_references || error_exit "Failed to update MAIN references"
        ;;
    version)
        version
        ;;
    *)
        echo "$1 is an invalid option"
        help
        exit 1
        ;;
esac
