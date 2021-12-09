#!/bin/bash

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

INSTALL=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=$INSTALL
export DYNAWO_ALGORITHMS_HOME=$INSTALL
export LD_LIBRARY_PATH=$INSTALL/lib64:$INSTALL/lib:$LD_LIBRARY_PATH
export IIDM_XML_XSD_PATH=$DYNAWO_HOME/share/iidm/xsd
export DYNAFLOW_LAUNCHER_LOCALE=en_GB
export DYNAFLOW_LAUNCHER_INSTALL=$INSTALL
export DYNAWO_RESOURCES_DIR=$DYNAWO_HOME/share:$DYNAWO_HOME/share/xsd:$DYNAFLOW_LAUNCHER_EXTERNAL_RESOURCES_DIR
if [ ! -z "$DYNAFLOW_LAUNCHER_EXTERNAL_DDB" ]; then
  export DYNAWO_DDB_DIR=$DYNAWO_HOME/ddb
else
  export DYNAWO_DDB_DIR=$DYNAFLOW_LAUNCHER_EXTERNAL_DDB
fi
if [ ! -z "$DYNAWO_IIDM_EXTENSION" ]; then
  export DYNAWO_IIDM_EXTENSION=$DYNAWO_HOME/lib/libdynawo_DataInterfaceIIDMExtension.so
fi
export DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_HOME/lib:$DYNAFLOW_LAUNCHER_EXTERNAL_LIBRARIES
if [ ! -z "$DYNAFLOW_LAUNCHER_EXTERNAL_LIBRARIES" ]; then
  export LD_LIBRARY_PATH="$DYNAFLOW_LAUNCHER_EXTERNAL_LIBRARIES:$LD_LIBRARY_PATH"
fi
export DYNAWO_INSTALL_DIR=$DYNAWO_HOME
export DYNAFLOW_LAUNCHER_XSD=$INSTALL/etc/xsd
export DYNAFLOW_LAUNCHER_LIBRARIES=$DYNAWO_DDB_DIR

export DYNAWO_ALGORITHMS_LOCALE=$DYNAFLOW_LAUNCHER_LOCALE

export_preload
MPIRUN_PATH=$(which mpirun 2> /dev/null)
if [ -z "$MPIRUN_PATH" ]; then
  MPIRUN_PATH="$INSTALL/bin/mpirun"
fi

args=""
NBPROCS=1
USEMPI=false
while (($#)); do
case $1 in
  --contingencies)
    if [ ! -z "$2" ]; then
      USEMPI=true
    fi
    args="$args --contingencies"
    shift
    ;;
  --nbThreads|-np)
    NBPROCS=$2
    shift 2 # pass argument and value
    ;;
  *)
    args="$args $1"
    shift
    ;;
  esac
done


if [ "$USEMPI" = true ]; then
  "$MPIRUN_PATH" -np $NBPROCS $INSTALL/bin/DynaFlowLauncher $args
else
  $INSTALL/bin/DynaFlowLauncher $args
fi
unset LD_PRELOAD
