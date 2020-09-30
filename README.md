# DynaFlow Launcher

## Build
To build DynaFlow launcher, you must first deploy the Dynawo library with c++11 enabled. Then generate the cmake cache in your build directory:

`cmake <SRC_DIR> -DDYNAWO_HOME=<DYNAWO_HOME> -DBOOST_ROOT=<DYNAWO_HOME> -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> -DDYNAFLOW_LAUNCHER_LOCALE=<LOCALE>`

where:
* SRC_DIR is the root directory of the dynaflow launcher repository
* DYNAWO_HOME is the directory of the deployed DYNAWO library (since Boost is deployed along with Dynawo, we use the same directory as Boost root)
* CMAKE_INSTALL_PREFIX is the directory where DynaFlow Launcher will be installed
* DYNAFLOW_LAUNCHER_LOCALE is the reference locale value to generate log keys. Suppoorted values are:
  * en_GB (English)

Then build the project in the build directory:

`cmake --build . --target install`

## Run
To run DynaFlow launcher, the following environment variables must be defined:
* LD_LIBRARY_PATH: must contain the path to the deployed shared libraries of dynawo
* IIDM_XML_XSD_PATH: must be the path to xsd of dynawo (generally ${DYNAWO_HOME}/share/iidm/xsd depending on the deployement of Dynawo)
* DYNAFLOW_LAUNCHER_LOCALE: the locale dictionnary to use. The supported values are the same as for the build. The runtime value can be different from the one used in compilation but all keys must be the same in both files and must use the same number of arguments
* DYNAFLOW_LAUNCHER_DICTS: the directory where the dictionnaries are located

Runtime options of binary are given by doing:

`DynawFlowLauncher --help`

Only network file and configuration are required
