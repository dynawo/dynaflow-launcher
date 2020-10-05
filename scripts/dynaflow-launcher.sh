#!/bin/bash
INSTALL=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=$INSTALL/external/dynawo
export LD_LIBRARY_PATH=$DYNAWO_HOME/lib:$LD_LIBRARY_PATH
export IIDM_XML_XSD_PATH=$DYNAWO_HOME/share/iidm/xsd
export DYNAFLOW_LAUNCHER_LOCALE=en_GB
export DYNAFLOW_LAUNCHER_INSTALL=$INSTALL

$INSTALL/bin/DynaFlowLauncher $@
