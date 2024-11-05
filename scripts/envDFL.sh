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

# This file aims to encapsulate processing for users to compiler and use DFL

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
MPIRUN_PATH=$(which mpirun 2> /dev/null)
NBPROCS=1

error_exit() {
    echo "${1:-"Unknown Error"}" 1>&2
    exit 1
}

export_var_env() {
  local var="$@"
  local name=${var%%=*}
  local value="${var#*=}"

  if ! `expr $name : "DYNAFLOW_LAUNCHER_.*" > /dev/null`; then
    if [ "$name" != "DYNAWO_HOME" ] &&
        [ "$name" != "DYNAWO_ALGORITHMS_HOME" ] &&
        [ "$name" != "DYNAWO_DDB_DIR" ] &&
        [ "$name" != "DYNAWO_PYTHON_COMMAND" ] &&
        [ "$name" != "DYNAWO_DICTIONARIES" ] &&
        [ "$name" != "DYNAWO_USE_MPI" ]; then
      error_exit "You must export variables with DYNAFLOW_LAUNCHER_ prefix for $name."
    fi
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

export_var_env_force_dynawo() {
  local var="$@"
  local name=${var%%=*}
  local value="${var#*=}"

  if ! `expr $name : "DYNAWO_.*" > /dev/null`; then
    error_exit "You must export variables with DYNAWO prefix for $name."
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

export_var_env_dynawo_with_default() {
  # export_var_env_dynawo_with_default <VAR> <EXTERNAL_VAL> <DEFAULT_VAL>
  # case <EXTERNAL_VAL> non empty : <EXTERNAL_VAL>
  # case <EXTERNAL_VAL> empty : <DEFAULT_VAL>
  export_var_env_force_dynawo $1=$2
}

export_preload() {
  lib="tcmalloc"
  # uncomment to activate tcmalloc in debug when build is in debug
  # if [ $DYNAWO_BUILD_TYPE == "Debug" ]; then
  #   lib=$lib"_debug"
  # fi
  lib=$lib".so"

  externalTcMallocLib=$(find $DYNAWO_ALGORITHMS_HOME/lib -iname *$lib)
  if [ -n "$externalTcMallocLib" ]; then
    echo "Use downloaded tcmalloc library $externalTcMallocLib"
    export LD_PRELOAD=$externalTcMallocLib
    return
  fi

  nativeTcMallocLib=$(ldconfig -p | grep -e $lib$ | cut -d ' ' -f4)
  if [ -n "$nativeTcMallocLib" ]; then
    echo "Use native tcmalloc library $nativeTcMallocLib"
    export LD_PRELOAD=$nativeTcMallocLib
    return
  fi
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
        build-user                                build DynaFlow Launcher
        clean                                     clean DynaFlow Launcher
        clean-build-all                           Clean and rebuild DynaFlow Launcher
        build-tests-coverage                      build DynaFlow Launcher and run coverage

        =========== Tests
        tests                                     launch DynaFlow Launcher unit tests
        update-references                         update MAIN tests references

        =========== Launch
        launch [network] [config]                 launch DynaFlow Launcher:
                                                  - network: filepath (only IIDM is supported)
                                                  - config: filepath (JSON configuration file)

        launch-sa [network] [config] [contingencies] --nbThreads [nbprocs]
                                                  launch DynaFlow Launcher to run a Security Analysis:
                                                  - network: filepath (only IIDM is supported)
                                                  - config: filepath (JSON configuration file)
                                                  - contingencies: filepath (JSON file)
                                                  - nbprocs: number of MPI processes to use for SA (default 1)

        launch-gdb [network] [config]             launch DynaFlow Launcher with debugger:
                                                  - network: filepath (only IIDM is supported)
                                                  - config: filepath (JSON configuration file)

        launch-sa-gdb [network] [config] [contingencies] --nbThreads [nbprocs]
                                                  launch DynaFlow Launcher in gdb to run a Security Analysis:
                                                  - network: filepath (only IIDM is supported)
                                                  - config: filepath (JSON configuration file)
                                                  - contingencies: filepath (JSON file)
                                                  - nbprocs: number of MPI processes to use for SA (default 1)

        =========== Documentation
        doc                                       open DynaFlow Launcher documentation
        build-doc                                 build documentation
        clean-doc                                 clean documentation
        doxygen-doc                               open DynaFlow Launcher's Doxygen documentation into chosen browser
        build-doxygen-doc                         build all doxygen documentation

        =========== Others
        help                                      show all available options
        format                                    format modified git files using clang-format
        version                                   show DynaFlow Launcher version
        reset-environment                         reset all environment variables set by DynaFlow Launcher
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
    MODE=$1
    export_var_env DYNAFLOW_LAUNCHER_HOME=UNDEFINED
    export_var_env DYNAFLOW_LAUNCHER_BUILD_TYPE=UNDEFINED
    export_var_env DYNAFLOW_LAUNCHER_LOCALE=en_GB
    export_var_env DYNAFLOW_LAUNCHER_BROWSER=firefox
    export_var_env DYNAFLOW_LAUNCHER_BROWSER_SHOW=true
    export_var_env DYNAFLOW_LAUNCHER_PDFVIEWER=xdg-open
    export_var_env DYNAFLOW_LAUNCHER_USE_XSD_VALIDATION="true"

    # dynawo vars
    export_var_env DYNAWO_HOME=UNDEFINED
    export DYNAWO_INSTALL_DIR=$DYNAWO_HOME
    export IIDM_XML_XSD_PATH=$DYNAWO_INSTALL_DIR/share/iidm/xsd

    export_var_env DYNAWO_DDB_DIR=$DYNAWO_INSTALL_DIR/ddb

    # dynawo algorithms
    export_var_env DYNAWO_ALGORITHMS_HOME=UNDEFINED
    # Use same locale of dynaflow launcher
    export DYNAWO_ALGORITHMS_LOCALE=$DYNAFLOW_LAUNCHER_LOCALE

    # miscellaneous
    export_var_env DYNAWO_USE_MPI=YES
    export_var_env DYNAWO_PYTHON_COMMAND="python"
    if [ ! -x "$(command -v ${DYNAWO_PYTHON_COMMAND})" ]; then
      error_exit "Your python interpreter \"${DYNAWO_PYTHON_COMMAND}\" does not work. Use export DYNAWO_PYTHON_COMMAND=<Python Interpreter> in your myEnvDynawo.sh."
    fi
    export_var_env DYNAWO_DICTIONARIES=dictionaries_mapping

    # build
    export_var_env_force DYNAFLOW_LAUNCHER_BUILD_DIR=$DYNAFLOW_LAUNCHER_HOME/build/dynaflow-launcher
    export_var_env DYNAFLOW_LAUNCHER_INSTALL_DIR=$DYNAFLOW_LAUNCHER_HOME/install/dynaflow-launcher

    # 3rd party
    export_var_env DYNAFLOW_LAUNCHER_THIRD_PARTY_BUILD_DIR=$DYNAFLOW_LAUNCHER_HOME/build/3rdParty
    export_var_env DYNAFLOW_LAUNCHER_THIRD_PARTY_INSTALL_DIR=$DYNAFLOW_LAUNCHER_HOME/install/3rdParty
    export_var_env_force DYNAFLOW_LAUNCHER_THIRD_PARTY_SRC_DIR=$DYNAFLOW_LAUNCHER_HOME/3rdParty

    # global vars
    ld_library_path_prepend $DYNAWO_INSTALL_DIR/lib         # For Dynawo library
    ld_library_path_prepend $DYNAWO_ALGORITHMS_HOME/lib     # For Dynawo-algorithms library
    ld_library_path_prepend $DYNAFLOW_LAUNCHER_THIRD_PARTY_INSTALL_DIR/mpich/lib     # For mpich library

    # dynawo vars that can be extended by external
    export DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_INSTALL_DIR/lib
    export DYNAWO_RESOURCES_DIR=$DYNAFLOW_LAUNCHER_INSTALL_DIR/share:$DYNAWO_INSTALL_DIR/share/xsd

    if [ $MODE -ne 2 ]; then
        ld_library_path_prepend $DYNAFLOW_LAUNCHER_INSTALL_DIR/lib64 # For local DFL libraries, used only at runtime in case we compile in shared
        ld_library_path_prepend $DYNAFLOW_LAUNCHER_INSTALL_DIR/lib # For local DFL libraries, used only at runtime in case we compile in shared
    fi

    export_var_env DYNAFLOW_LAUNCHER_BUILD_TESTS=ON # same default value as cmakelist
    export_var_env DYNAFLOW_LAUNCHER_CMAKE_GENERATOR="Unix Makefiles"
    export_var_env DYNAFLOW_LAUNCHER_PROCESSORS_USED=1
    export DYNAWO_NB_PROCESSORS_USED=$DYNAFLOW_LAUNCHER_PROCESSORS_USED
    export_var_env DYNAFLOW_LAUNCHER_FORCE_CXX11_ABI="false"

    # Run
    if [ $MODE -eq 0 ]
    then
        # export runtime variables only if unit tests are not run
        export_var_env_force DYNAFLOW_LAUNCHER_INSTALL=$DYNAFLOW_LAUNCHER_INSTALL_DIR
        export_var_env_force DYNAFLOW_LAUNCHER_LIBRARIES=$DYNAWO_DDB_DIR # same as dynawo
        export_var_env_force DYNAFLOW_LAUNCHER_XSD=$DYNAFLOW_LAUNCHER_INSTALL_DIR/etc/xsd
        export_var_env DYNAFLOW_LAUNCHER_LOG_LEVEL=INFO # INFO by default
    fi

    if [ "${DYNAWO_USE_MPI}" == "YES" -a -z "${MPIRUN_PATH}" ]
    then
        MPIRUN_PATH="$DYNAWO_ALGORITHMS_HOME/bin/mpirun"
    fi

    # python
    pythonpath_prepend $DYNAWO_HOME/sbin/nrt/nrt_diff

    # hooks
    if [ -d "$DYNAFLOW_LAUNCHER_HOME/.git" ]; then
      set_commit_hook
    fi
}

reset_environment_variables() {
    for var in $(printenv | grep DYNAFLOW_LAUNCHER_ | cut -d '=' -f 1); do
        unset $var
    done

    ld_library_path_remove $DYNAFLOW_LAUNCHER_INSTALL_DIR/lib64
    ld_library_path_remove $DYNAFLOW_LAUNCHER_INSTALL_DIR/lib
    ld_library_path_remove $DYNAWO_INSTALL_DIR/lib
    ld_library_path_remove $DYNAWO_ALGORITHMS_HOME/lib
    ld_library_path_remove $DYNAFLOW_LAUNCHER_THIRD_PARTY_INSTALL_DIR/mpich/lib

    pythonpath_remove $DYNAWO_HOME/sbin/nrt/nrt_diff

    unset DYNAWO_HOME
    unset DYNAWO_INSTALL_DIR
    unset DYNAWO_RESOURCES_DIR
    unset DYNAWO_DDB_DIR
    unset DYNAWO_LIBIIDM_EXTENSIONS
    unset IIDM_XML_XSD_PATH
    unset DYNAWO_NB_PROCESSORS_USED

    unset DYNAWO_ALGORITHMS_HOME
}

clean() {
    rm -rf $DYNAFLOW_LAUNCHER_BUILD_DIR
    rm -rf $DYNAFLOW_LAUNCHER_INSTALL_DIR
    pushd $DYNAFLOW_LAUNCHER_HOME > /dev/null
    find tests -type d -name "resultsTestsTmp" -prune -exec rm -rf {} \;
    popd > /dev/null
}

cmake_configure() {
    CMAKE_OPTIONAL=""
    if [ $DYNAFLOW_LAUNCHER_FORCE_CXX11_ABI = true ]; then
        CMAKE_OPTIONAL="$CMAKE_OPTIONAL -DFORCE_CXX11_ABI=$DYNAFLOW_LAUNCHER_FORCE_CXX11_ABI"
    fi
    mkdir -p $DYNAFLOW_LAUNCHER_BUILD_DIR
    pushd $DYNAFLOW_LAUNCHER_BUILD_DIR > /dev/null
    cmake $DYNAFLOW_LAUNCHER_HOME \
        -G "$DYNAFLOW_LAUNCHER_CMAKE_GENERATOR" \
        -DCMAKE_BUILD_TYPE:STRING=$DYNAFLOW_LAUNCHER_BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX:STRING=$DYNAFLOW_LAUNCHER_INSTALL_DIR \
        -DDYNAFLOW_LAUNCHER_HOME:STRING=$DYNAFLOW_LAUNCHER_HOME \
        -DDYNAWO_HOME:STRING=$DYNAWO_HOME \
        -DDYNAWO_ALGORITHMS_HOME:STRING=$DYNAWO_ALGORITHMS_HOME \
        -DDYNAWO_PYTHON_COMMAND:STRING=${DYNAWO_PYTHON_COMMAND} \
        -DDYNAFLOW_LAUNCHER_THIRD_PARTY_DIR=$DYNAFLOW_LAUNCHER_THIRD_PARTY_INSTALL_DIR \
        -DBOOST_ROOT:STRING=$DYNAWO_HOME \
        -DDYNAFLOW_LAUNCHER_LOCALE:STRING=$DYNAFLOW_LAUNCHER_LOCALE \
        -DDYNAFLOW_LAUNCHER_BUILD_TESTS:BOOL=$DYNAFLOW_LAUNCHER_BUILD_TESTS \
        -DUSE_MPI=$DYNAWO_USE_MPI \
        $CMAKE_OPTIONAL
    RETURN_CODE=$?
    popd > /dev/null
    return ${RETURN_CODE}
}

cmake_configure_3rdParty() {
  if [ ! -d "$DYNAFLOW_LAUNCHER_THIRD_PARTY_BUILD_DIR" ]; then
    mkdir -p $DYNAFLOW_LAUNCHER_THIRD_PARTY_BUILD_DIR
  fi
  pushd $DYNAFLOW_LAUNCHER_THIRD_PARTY_BUILD_DIR > /dev/null
  cmake \
    -G "$DYNAFLOW_LAUNCHER_CMAKE_GENERATOR" \
    -DCMAKE_BUILD_TYPE:STRING=$DYNAFLOW_LAUNCHER_BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=$DYNAFLOW_LAUNCHER_THIRD_PARTY_INSTALL_DIR \
    -DDYNAWO_HOME:STRING=$DYNAWO_HOME \
    -DUSE_MPI=$DYNAWO_USE_MPI \
    $DYNAFLOW_LAUNCHER_THIRD_PARTY_SRC_DIR
  RETURN_CODE=$?
  popd > /dev/null
  return ${RETURN_CODE}
}

cmake_build_3rdParty() {
  pushd $DYNAFLOW_LAUNCHER_THIRD_PARTY_BUILD_DIR > /dev/null
  cmake --build .
  RETURN_CODE=$?
  popd > /dev/null
  return ${RETURN_CODE}
}

cmake_build() {
    apply_clang_format
    cmake --build $DYNAFLOW_LAUNCHER_BUILD_DIR --target install -j $DYNAFLOW_LAUNCHER_PROCESSORS_USED
    RETURN_CODE=$?
    return ${RETURN_CODE}
}

cmake_tests() {
    pushd $DYNAFLOW_LAUNCHER_HOME > /dev/null
    find tests -type d -name "resultsTestsTmp" -prune -exec rm -rf {} \;
    popd > /dev/null
    pushd $DYNAFLOW_LAUNCHER_BUILD_DIR > /dev/null
    ctest -j $DYNAFLOW_LAUNCHER_PROCESSORS_USED --output-on-failure
    RETURN_CODE=$?
    popd > /dev/null
    return ${RETURN_CODE}
}


unittest_gdb() {
    list_of_tests=($(find $DYNAFLOW_LAUNCHER_BUILD_DIR -executable -type f -exec basename {} \; | grep Test))
    if [[ ${#list_of_tests[@]} == 0 ]]; then
      echo "The list of tests is empty. This should not happen."
      exit 1
    fi
    if [ -z "$2" ]; then
      echo "You need to give the name of unittest to run."
      echo "List of available unittests:"
      for name in ${list_of_tests[@]}; do
        echo "  $name"
      done
      exit 1
    fi
    unittest_exe=$(find $DYNAFLOW_LAUNCHER_BUILD_DIR -name "$2" -executable -type f)
    if [ -z "$unittest_exe" ]; then
      echo "The unittest you gave is not available."
      echo "List of available unittests:"
      for name in ${list_of_tests[@]}; do
        echo "  $name"
      done
      exit 1
    fi

    pushd $(dirname $unittest_exe) > /dev/null
    gdb -q --args $unittest_exe
    RETURN_CODE=$?
    popd > /dev/null
    return ${RETURN_CODE}

}
verify_browser() {
  if [ ! -x "$(command -v $DYNAFLOW_LAUNCHER_BROWSER)" ]; then
    error_exit "Specified browser DYNAFLOW_LAUNCHER_BROWSER=$DYNAFLOW_LAUNCHER_BROWSER not found."
  fi
}

cmake_coverage() {
    pushd $DYNAFLOW_LAUNCHER_HOME > /dev/null
    find tests -type d -name "resultsTestsTmp" -prune -exec rm -rf {} \;
    export GTEST_COLOR=1
    ctest \
        -S cmake/CTestScript.cmake \
        -DDYNAFLOW_LAUNCHER_HOME=$DYNAFLOW_LAUNCHER_HOME \
        -DDYNAWO_HOME=$DYNAWO_HOME \
        -DDYNAWO_ALGORITHMS_HOME=$DYNAWO_ALGORITHMS_HOME \
        -DBOOST_ROOT=$DYNAWO_HOME \
        -DDYNAFLOW_LAUNCHER_THIRD_PARTY_DIR=$DYNAFLOW_LAUNCHER_THIRD_PARTY_INSTALL_DIR \
        -DDYNAWO_PYTHON_COMMAND:STRING=${DYNAWO_PYTHON_COMMAND} \
        -DDYNAWO_USE_MPI=${DYNAWO_USE_MPI} \
        -VV

    RETURN_CODE=$?
    if [ ${RETURN_CODE} -ne 0 ]; then
        popd > /dev/null
        exit ${RETURN_CODE}
    fi

    mkdir -p $DYNAFLOW_LAUNCHER_HOME/build/coverage/coverage-sonar || error_exit "Impossible to create $DYNAFLOW_LAUNCHER_HOME/build/coverage/coverage-sonar."
    cd $DYNAFLOW_LAUNCHER_HOME/build/coverage/coverage-sonar
    for file in $(find $DYNAFLOW_LAUNCHER_HOME/build/coverage -name "*.gcno" | grep -v "/tests/" | grep -v "/googletest-build/"); do
        cpp_file_name=$(basename $file .gcno)
        cpp_file=$(find $DYNAFLOW_LAUNCHER_HOME/sources -name "$cpp_file_name" 2> /dev/null)
        gcov -pb $cpp_file -o $file > /dev/null
    done
    find $DYNAFLOW_LAUNCHER_HOME/build/coverage/coverage-sonar -type f -not -name "*dynaflow-launcher*" -exec rm -f {} \;
    popd > /dev/null
}

clean_build_all() {
    clean
    build_user
}

build_user() {
    cmake_configure_3rdParty || error_exit "Error during 3rd party cmake configuration."
    cmake_build_3rdParty || error_exit "Error during 3rd party cmake build."
    cmake_configure || error_exit "Error during cmake configuration."
    cmake_build || error_exit "Error during cmake build."
    build_test_doxygen_doc || error_exit "Error during build_test_doxygen_doc."
}

build_tests_coverage() {
    #rm -rf $DYNAFLOW_LAUNCHER_HOME/build/coverage
    cmake_configure_3rdParty || error_exit "Error during 3rd party cmake configuration."
    cmake_build_3rdParty || error_exit "Error during 3rd party cmake build."
    cmake_coverage || error_exit "Error during coverage."
    if [ "$DYNAFLOW_LAUNCHER_BROWSER_SHOW" = true ] ; then
        verify_browser
        $DYNAFLOW_LAUNCHER_BROWSER $DYNAFLOW_LAUNCHER_HOME/build/coverage/coverage/index.html
    fi
}

launch() {
    if [ ! -f "$2" ]; then
        error_exit "IIDM network file $2 doesn't exist"
    fi
    if [ ! -f "$3" ]; then
        error_exit "DFL configuration file $3 doesn't exist"
    fi
    $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher \
    --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
    --network $2 \
    --config $3
}

launch_gdb() {
    if [ ! -f "$2" ]; then
        error_exit "IIDM network file $1 doesn't exist"
    fi
    if [ ! -f "$3" ]; then
        error_exit "DFL configuration file $2 doesn't exist"
    fi
    gdb --args $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher \
    --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
    --network $2 \
    --config $3
}

launch_valgrind() {
    if [ ! -f "$2" ]; then
        error_exit "IIDM network file $1 doesn't exist"
    fi
    if [ ! -f "$3" ]; then
        error_exit "DFL configuration file $2 doesn't exist"
    fi
    valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher \
    --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
    --network $2 \
    --config $3
}


launch_sa() {
    if [ ! -f "$2" ]; then
        error_exit "IIDM network file $2 doesn't exist"
    fi
    if [ ! -f "$3" ]; then
        error_exit "DFL configuration file $3 doesn't exist"
    fi
    if [ ! -f "$4" ]; then
        error_exit "Security Analysis contingencies file $4 doesn't exist"
    fi
    export_preload
    if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
      "$MPIRUN_PATH" -np $NBPROCS $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
                                                                                      --network $2 \
                                                                                      --config $3 \
                                                                                      --contingencies $4
    else
      $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
                                                          --network $2 \
                                                          --config $3 \
                                                          --contingencies $4
    fi
    unset LD_PRELOAD
}

launch_nsa() {
    if [ ! -f "$2" ]; then
        error_exit "IIDM network file $2 doesn't exist"
    fi
    if [ ! -f "$3" ]; then
        error_exit "DFL configuration file $3 doesn't exist"
    fi
    if [ ! -f "$4" ]; then
        error_exit "Security Analysis contingencies file $4 doesn't exist"
    fi
    export_preload
    if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
      "$MPIRUN_PATH" -np $NBPROCS $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
                                                                                      --network $2 \
                                                                                      --config $3 \
                                                                                      --contingencies $4 \
                                                                                      --nsa # Steady state calculations.
    else
      $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
                                                          --network $2 \
                                                          --config $3 \
                                                          --contingencies $4 \
                                                          --nsa # Steady state calculations.
    fi
    unset LD_PRELOAD
}

launch_sa_gdb() {
    if [ ! -f "$2" ]; then
        error_exit "IIDM network file $2 doesn't exist"
    fi
    if [ ! -f "$3" ]; then
        error_exit "DFL configuration file $3 doesn't exist"
    fi
    if [ ! -f "$4" ]; then
        error_exit "Security Analysis contingencies file $4 doesn't exist"
    fi
    export_preload
    if [ "${DYNAWO_USE_MPI}" == "YES" ]; then
      "$MPIRUN_PATH" -np $NBPROCS xterm -e gdb -q --args $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
                                                                                                              --network $2 \
                                                                                                              --config $3 \
                                                                                                              --contingencies $4
    else
      gdb -q --args $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --log-level $DYNAFLOW_LAUNCHER_LOG_LEVEL \
                                                                        --network $2 \
                                                                        --config $3 \
                                                                        --contingencies $4
    fi
    unset LD_PRELOAD
}

version() {
    $DYNAFLOW_LAUNCHER_INSTALL_DIR/bin/DynaFlowLauncher --version
}

update_references() {
    ${DYNAWO_PYTHON_COMMAND} $HERE/updateMainReference.py
}

apply_clang_format() {
    CLANGF=$(which clang-format)
    if [ -z $CLANGF ]; then
        echo "WARNING: Clang format not found"
    else
        pushd $DYNAFLOW_LAUNCHER_HOME > /dev/null
        for file in $(git diff-index --name-only HEAD | grep -iE '\.(cpp|cc|h|hpp)$'); do
            $CLANGF --style=file -i $file
        done
        popd > /dev/null
    fi
}

# Compile Dynaflo Launcher Doxygen doc
build_test_doxygen_doc() {
  build_doxygen_doc || error_exit
  test_doxygen_doc || error_exit
}

build_doxygen_doc() {
  if [ ! -d "$DYNAFLOW_LAUNCHER_BUILD_DIR" ]; then
    error_exit "You need to build Dynaflow launcher first to build doxygen documentation."
  fi
  mkdir -p $DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/
  cmake --build $DYNAFLOW_LAUNCHER_BUILD_DIR --target doc
  RETURN_CODE=$?
  return ${RETURN_CODE}
}

test_doxygen_doc() {
  if [ -f "$DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/warnings.txt"  ] ; then
    rm -f $DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/warnings_filtered.txt
    # need to filter "return type of member (*) is not documented" as it is a doxygen bug detected on 1.8.17 that will be solved in 1.8.18
    grep -Fvf $DYNAFLOW_LAUNCHER_HOME/etc/warnings_to_filter.txt $DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/warnings.txt > $DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/warnings_filtered.txt
    nb_warnings=$(wc -l $DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/warnings_filtered.txt | awk '{print $1}')
    if [ ${nb_warnings} -ne 0 ]; then
      echo "===================================="
      echo "| Result of doxygen doc generation |"
      echo "===================================="
      echo " nbWarnings = ${nb_warnings} > 0 => doc is incomplete"
      echo " edit ${DYNAFLOW_LAUNCHER_INSTALL_DIR}/doxygen/warnings_filtered.txt  to have more details"
      error_exit "Doxygen doc is not complete"
    fi
  fi
}

open_doxygen_doc() {
  if [ ! -f "$DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/html/index.html" ]; then
    echo "Doxygen documentation not yet generated"
    echo "Generating ..."
    build_test_doxygen_doc
    RETURN_CODE=$?
    if [ ${RETURN_CODE} -ne 0 ]; then
      exit ${RETURN_CODE}
    fi
    echo "... end of doc generation"
  fi
  verify_browser
  $DYNAFLOW_LAUNCHER_BROWSER $DYNAFLOW_LAUNCHER_INSTALL_DIR/doxygen/html/index.html
}

build_doc() {
  if [ ! -d "$DYNAFLOW_LAUNCHER_HOME/documentation" ]; then
    error_exit "$DYNAFLOW_LAUNCHER_HOME/documentation does not exist."
  fi
  cd $DYNAFLOW_LAUNCHER_HOME/documentation
  bash dynaflow_launcher_documentation.sh
}

clean_doc() {
  if [ ! -d "$DYNAFLOW_LAUNCHER_HOME/documentation" ]; then
    error_exit "$DYNAFLOW_LAUNCHER_HOME/documentation does not exist."
  fi
  cd $DYNAFLOW_LAUNCHER_HOME/documentation
  bash clean.sh
}

open_pdf() {
  if [ -z "$1" ]; then
    error_exit "You need to specify a pdf file to open."
  fi
  reset_environment_variables #conflict zlib
  if [ ! -z "$DYNAFLOW_LAUNCHER_PDFVIEWER" ]; then
    if [ -x "$(command -v $DYNAFLOW_LAUNCHER_PDFVIEWER)" ]; then
      if [ -f "$1" ]; then
        $DYNAFLOW_LAUNCHER_PDFVIEWER $1
      else
        error_exit "Pdf file $1 you try to open does not exist."
      fi
    else
      error_exit "pdfviewer $DYNAFLOW_LAUNCHER_PDFVIEWER seems not to be executable."
    fi
  elif [ -x "$(command -v xdg-open)" ]; then
      xdg-open $1
  else
    error_exit "Cannot determine how to open pdf document from command line. Use DYNAFLOW_LAUNCHER_PDFVIEWER environment variable."
  fi
}

open_doc() {
  open_pdf $DYNAFLOW_LAUNCHER_HOME/documentation/dynaflowLauncherDocumentation/DynaflowLauncherDocumentation.pdf
}

#################################
########### Main script #########
#################################

MODE=0 # normal
CMD="$1"
case "$1" in
    tests)
        # environment used for unit tests is defined only in cmakelist
        MODE=1
        ;;
    build-tests-coverage)
        # environment used for unit tests in coverage case is defined only in cmakelist
        MODE=2
        ;;
    *)
        ;;
esac

# Nb Threads
ARGS=""
while (($#)); do
    key="$1"
    case "$key" in
      --nbThreads|-np)
        NBPROCS=$2
        shift 2 # pass argument and value
        ;;
      --nbThreads=*)
        NBPROCS="${1#*=}"
        shift # past value
      ;;
    *)
        shift # pass argument
        ARGS="$ARGS $key"
        ;;
    esac
done
set_environment $MODE

case $CMD in
    build-user)
        build_user || error_exit "Failed to build DFL"
        ;;
    build-tests-coverage)
        build_tests_coverage || error_exit "Failed to perform coverage"
        ;;
    clean)
        clean || error_exit "Failed to clean DFL"
        ;;
    clean-build-all)
        clean_build_all || error_exit "Failed to clean build DFL"
        ;;
    help)
        help
        ;;
    launch)
        launch $ARGS || error_exit "Failed to perform launch"
        ;;
    launch-gdb)
        launch_gdb $ARGS || error_exit "Failed to perform launch with network=$2, config=$3"
        ;;
    launch-valgrind)
        launch_valgrind $ARGS || error_exit "Failed to perform valgrind with network=$2, config=$3"
        ;;
    launch-sa)
        launch_sa $ARGS || error_exit "Failed to perform launch-sa"
        ;;
    launch-nsa)
        launch_nsa $ARGS || error_exit "Failed to perform launch-nsa"
        ;;
    launch-sa-gdb)
        launch_sa_gdb $ARGS || error_exit "Failed to perform launch-sa-gdb"
        ;;
    reset-environment)
        reset_environment_variables || error_exit "Failed to reset environment variables"
        ;;
    build-doc)
        build_doc || error_exit "Error during the build of dynawo documentation"
        ;;
    clean-doc)
        clean_doc || error_exit "Error during the clean of Dynawo documentation"
        ;;
    doc)
        open_doc || error_exit "Error during the opening of Dynawo documentation"
        ;;
    doxygen-doc)
        open_doxygen_doc || error_exit "Error during Dynawo Doxygen doc visualisation"
        ;;
    tests)
        cmake_tests || error_exit "Failed to perform tests"
        ;;
    tests-gdb)
        unittest_gdb $ARGS || error_exit "Failed to perform tests with gdb"
        ;;
    update-references)
        update_references || error_exit "Failed to update MAIN references"
        ;;
    format)
        apply_clang_format || error_exit "Failed to format files"
        ;;
    build-doxygen-doc)
        build_test_doxygen_doc || error_exit "Error while building doxygen documentation"
        ;;
    doxygen-doc)
        open_doxygen_doc || error_exit "Error during Dynaflow Launcher Doxygen doc visualisation"
        ;;
    version)
        version
        ;;
    *)
        echo "$CMD is an invalid option"
        help
        exit 1
        ;;
esac
