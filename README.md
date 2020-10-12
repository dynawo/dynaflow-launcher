# DynaFlow Launcher

## Build
To build DynaFlow launcher, you must first deploy the Dynawo library with c++11 enabled. Then generate the cmake cache in your build directory:

`cmake <SRC_DIR> -DDYNAWO_HOME=<DYNAWO_HOME> -DBOOST_ROOT=<DYNAWO_HOME> -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DDYNAFLOW_LAUNCHER_LOCALE=<LOCALE>`

where:
* SRC_DIR is the root directory of the dynaflow launcher repository
* INSTALL_DIR is the directory where dynaflow launcher will be installed
* DYNAWO_HOME is the directory of the deployed DYNAWO library (since Boost is deployed along with Dynawo, we use the same directory as Boost root)
* CMAKE_INSTALL_PREFIX is the directory where DynaFlow Launcher will be installed
* DYNAFLOW_LAUNCHER_LOCALE is the **optional** reference locale value to generate log keys. default value is "en_GB" (English)

Then build the project in the build directory:

`cmake --build . --target install`

## Run
To run DynaFlow launcher, use the script dynaflow-launcher.sh provided with the installation. This script will set the required environment variables.

Runtime options of binary are given by doing:

`dynaflow-launcher.sh --help`

Only network file and configuration are required
