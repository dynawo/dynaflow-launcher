# DynaFlow Launcher

## Build
To build DynaFlow launcher, you must first deploy the Dynawo library with c++11 enabled. Then generate the cmake cache in your build directory:

`cmake <SRC_DIR> -DCMAKE_BUILD_TYPE=Release -DDYNAWO_HOME=<DYNAWO_HOME> -DBOOST_ROOT=<DYNAWO_HOME> -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>`

where:
* SRC_DIR is the root directory of the dynaflow launcher repository
* INSTALL_DIR is the directory where dynaflow launcher will be installed
* DYNAWO_HOME is the directory of the deployed DYNAWO library (since Boost is deployed along with Dynawo, we use the same directory as Boost root)
* CMAKE_INSTALL_PREFIX is the directory where DynaFlow Launcher will be installed

The following **optional** CMake variables can be added:
* DYNAFLOW_LAUNCHER_LOCALE is the reference locale value to generate log keys. default value is "en_GB" (English)
* DYNAFLOW_LAUNCHER_SHARED_LIB is the option (ON/OFF) to compile DynaFlowLauncher in shared libraries. default value is OFF.
* DYNAFLOW_LAUNCHER_USE_DOXYGEN is the option to generate the doxygen documentation. default value is ON
* DYNAFLOW_LAUNCHER_BUILD_TESTS is the option to build the unit tests with DynaFlowLauncher. default value is ON

Then build the project in the build directory:

`cmake --build . --target install`

## Run
To run DynaFlow launcher, use the script dynaflow-launcher.sh provided with the installation. This script will set the required environment variables.

Runtime options of binary are given by doing:

`dynaflow-launcher.sh --help`

Only network file and configuration are required

## Testing
DynaFlow Launcher testing relies on cmake tests. To launch units tests, launch from *build* directory:

`ctest -j<N> --output-on-failure`

where N is the number of jobs to use for the testing session

## Coverage
Coverage analysis is based on ctest. To launch coverage, launch from source directory:

`ctest -S cmake/CTestScript.cmake -DDYNAWO_HOME=<DYNAWO_HOME> -DBOOST_ROOT=<DYNAWO_HOME> -VV`

Outputs are gcov and lcov reports
