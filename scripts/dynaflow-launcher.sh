#!/bin/bash

error_exit() {
  RETURN_CODE=$?
  echo "${1:-"Unknown Error"}" 1>&2
  exit ${RETURN_CODE}
}

export_var_env() {
  local var=$@
  local name=${var%%=*}
  local value=${var#*=}

  if eval "[ \$$name ]"; then
    eval "value=\${$name}"
    return
  fi
  export $name="$value"
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
    # echo "Use downloaded tcmalloc library $externalTcMallocLib"
    export LD_PRELOAD=$externalTcMallocLib
    return
  fi

  nativeTcMallocLib=$(ldconfig -p | grep -e $lib$ | cut -d ' ' -f4)
  if [ -n "$nativeTcMallocLib" ]; then
    # echo "Use native tcmalloc library $nativeTcMallocLib"
    export LD_PRELOAD=$nativeTcMallocLib
    return
  fi
}

INSTALL=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=$INSTALL
export DYNAWO_ALGORITHMS_HOME=$INSTALL
export LD_LIBRARY_PATH=$INSTALL/lib64:$INSTALL/lib:$LD_LIBRARY_PATH
export IIDM_XML_XSD_PATH=$DYNAWO_HOME/share/iidm/xsd
export_var_env DYNAFLOW_LAUNCHER_LOCALE=en_GB
export DYNAFLOW_LAUNCHER_INSTALL=$INSTALL
export DYNAWO_RESOURCES_DIR=$DYNAWO_HOME/share:$DYNAWO_HOME/share/xsd
export DYNAWO_DICTIONARIES=dictionaries_mapping
export DYNAWO_DDB_DIR=$DYNAWO_HOME/ddb

if [ -z "$DYNAWO_IIDM_EXTENSION" ]; then
  if [ -f $DYNAWO_HOME/lib/libdynawo_RTE_DataInterfaceIIDMExtension.so ]; then
    export DYNAWO_IIDM_EXTENSION=$DYNAWO_HOME/lib/libdynawo_RTE_DataInterfaceIIDMExtension.so
  fi
fi
export DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_HOME/lib

export DYNAWO_INSTALL_DIR=$DYNAWO_HOME
export DYNAFLOW_LAUNCHER_XSD=$INSTALL/etc/xsd
export DYNAFLOW_LAUNCHER_LIBRARIES=$DYNAWO_DDB_DIR

export DYNAWO_ALGORITHMS_LOCALE=$DYNAFLOW_LAUNCHER_LOCALE

export_preload
MPIRUN_PATH="$INSTALL/bin/mpirun"

args=""
NBPROCS=1
USEMPI=false
DISPLAY_VERSION=false
while (($#)); do
case $1 in
  --contingencies)
    if [ ! -z "$2" ]; then
      USEMPI=true
    fi
    args="$args --contingencies"
    shift
    ;;
  --contingencies=*)
    if [ ! -z "${1#*=}" ]; then
      USEMPI=true
    fi
    args="$args --contingencies=${1#*=}"
    shift
    ;;
  --nbThreads|-np)
    NBPROCS=$2
    shift 2 # pass argument and value
    ;;
  --nbThreads=*)
    NBPROCS="${1#*=}"
    shift # past value
    ;;
  --version)
    DISPLAY_VERSION=true
    args="$args $1"
    shift
    ;;
  *)
    args="$args $1"
    shift
    ;;
  esac
done


if [ "$DISPLAY_VERSION" = true ]; then
  VERSION=$($INSTALL/bin/DynaFlowLauncher $args 2>&1 > /dev/null)
  VERSION=`echo $VERSION | sed 's/Invalid MIT-MAGIC-COOKIE-1 key//g'`
  >&2 echo "$VERSION"
elif [ "$USEMPI" = true ]; then
  "$MPIRUN_PATH" -np $NBPROCS $INSTALL/bin/DynaFlowLauncher $args || error_exit "Dynaflow-launcher execution failed"
else
  $INSTALL/bin/DynaFlowLauncher $args || error_exit "Dynaflow-launcher execution failed"
fi
unset LD_PRELOAD
