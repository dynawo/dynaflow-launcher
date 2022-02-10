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
export DYNAWO_RESOURCES_DIR=$DYNAWO_HOME/share:$DYNAWO_HOME/share/xsd
export DYNAWO_DDB_DIR=$DYNAWO_HOME/ddb
export DYNAWO_IIDM_EXTENSION=$DYNAWO_HOME/lib/libdynawo_DataInterfaceIIDMExtension.so
export DYNAWO_LIBIIDM_EXTENSIONS=$DYNAWO_HOME/lib
export DYNAWO_INSTALL_DIR=$DYNAWO_HOME
export DYNAFLOW_LAUNCHER_XSD=$INSTALL/etc/xsd
export DYNAFLOW_LAUNCHER_LIBRARIES=$DYNAWO_DDB_DIR

export DYNAWO_ALGORITHMS_LOCALE=$DYNAFLOW_LAUNCHER_LOCALE

export_preload
$INSTALL/bin/DynaFlowLauncher $@
unset LD_PRELOAD
