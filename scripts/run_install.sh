#!/bin/bash
INSTALL=`dirname "$0"`
export DYNAWO_HOME=$INSTALL/external/dynawo
export LD_LIBRARY_PATH=$DYNAWO_HOME/lib:$LD_LIBRARY_PATH
export IIDM_XML_XSD_PATH=$DYNAWO_HOME/share/iidm/xsd
export DYNAFLOW_LAUNCHER_LOCALE=en_GB
export DYNAFLOW_LAUNCHER_DICTS=$INSTALL/etc
export DYNAFLOW_LAUNCHER_PAR=$INSTALL/etc

$INSTALL/bin/DynaFlowLauncher $@
