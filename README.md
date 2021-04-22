# DynaFlow Launcher

Dynawflow Launcher (DFL) depends on the [core dynawo libraries of Dyna&omega;o](https://github.com/dynawo/dynawo)

## Build
To build DynaFlow launcher, you must first deploy the Dyna&omega;o library with **c++11 enabled**.

``` bash
$> ./myEnvDynawo.sh build-all
$> ./myEnvDynawo.sh deploy
```

This command creates a deploy folder in ${DYNAWO_HOME}.
The path to dynawo deploy is then the path to the subdirectory `dynawo` in the deploy folder. It is generally similar to:

``` bash
PATH_TO_DYNAWO_DEPLOY=${DYNAWO_HOME}/deploy/gcc8/shared/dynawo/
```

### By script

To build DynaFlow Launcher you need to clone the repository and launch the following commands in the source code directory, it will create a `myEnvDFL.sh` file that will be your personal entrypoint to launch DFL and parametrise some options.

``` bash
$> git clone https://github.com/dynawo/dynaflow-launcher
$> cd dynaflow-launcher
$> echo '#!/bin/bash

# Required
export DYNAFLOW_LAUNCHER_HOME=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=<PATH_TO_DYNAWO_DEPLOY>
export DYNAFLOW_LAUNCHER_BUILD_TYPE=Debug
export DYNAFLOW_LAUNCHER_LOCALE=en_GB

# Optional
# export DYNAFLOW_LAUNCHER_CMAKE_GENERATOR=Ninja # default is Unix Makefiles
# export DYNAFLOW_LAUNCHER_PROCESSORS_USED=8 # default 1
# export DYNAFLOW_LAUNCHER_SHARED_LIB=ON # default OFF
# export DYNAFLOW_LAUNCHER_USE_DOXYGEN=OFF # default ON
# export DYNAFLOW_LAUNCHER_BUILD_TESTS=OFF # default ON
# export DYNAFLOW_LAUNCHER_LOG_LEVEL=INFO # default INFO: can be DEBUG, INFO, WARN, ERROR

# Run
$DYNAFLOW_LAUNCHER_HOME/scripts/envDFL.sh $@' > myEnvDFL.sh
$> chmod +x myEnvDFL.sh
```

Then update the path "PATH_TO_DYNAWO_DEPLOY" in the file to your deployed installation of Dyna&omega;o and launch the following command:
``` bash
$> ./myEnvDFL.sh build-user
```

All commands described in the rest of this README are accessible throught this script. To access all options of the script, type:
``` bash
$> ./myEnvDFL.sh help
```

### Direct CMake compilation
You can generate the cmake cache in your build directory without using the script. Go to your build directory and then launch:

```bash
$> cmake <SRC_DIR> -DCMAKE_BUILD_TYPE=Release -DDYNAWO_HOME=<DYNAWO_HOME> -DBOOST_ROOT=<DYNAWO_HOME> -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
```

where:
* SRC_DIR is the root directory of the dynaflow launcher repository
* INSTALL_DIR is the directory where dynaflow launcher will be installed
* DYNAWO_HOME is the directory of the deployed Dyna&omega;o library (since Boost is deployed along with Dyna&omega;o, we use the same directory as Boost root)
* CMAKE_INSTALL_PREFIX is the directory where DynaFlow Launcher will be installed

The following **optional** CMake variables can be added:
* DYNAFLOW_LAUNCHER_LOCALE is the reference locale value to generate log keys. default value is "en_GB" (English)
* DYNAFLOW_LAUNCHER_SHARED_LIB is the option (ON/OFF) to compile DynaFlowLauncher in shared libraries. default value is OFF.
* DYNAFLOW_LAUNCHER_USE_DOXYGEN is the option to generate the doxygen documentation. default value is ON
* DYNAFLOW_LAUNCHER_BUILD_TESTS is the option to build the unit tests with DynaFlowLauncher. default value is ON

Then build the project in the build directory:

```bash
$> cmake --build . --target install
```

## Run
To run DynaFlow launcher, use the script dynaflow-launcher.sh provided with the installation. This script will set the required environment variables.

Runtime options of binary are given by doing:

```bash
$> ./dynaflow-launcher.sh --help
```
from the install directlory

Only network file and configuration are required.

You can also use the script with the "launch" or "launch-dir" option.

## Testing
DynaFlow Launcher testing relies on cmake tests. To launch units tests, launch from *build* directory:

```bash
$> ctest -j<N> --output-on-failure
```

where N is the number of jobs to use for the testing session.

You can also use the script with the "tests" option.

## Coverage
Coverage analysis is based on ctest. To launch coverage, launch from source directory:

```bash
$> ctest -S cmake/CTestScript.cmake -DDYNAWO_HOME=<DYNAWO_HOME> -DBOOST_ROOT=<DYNAWO_HOME> -VV
```

Outputs are gcov and lcov reports.

You can also use the script with the "build-tests-coverage" option.

## Developement standard
### Check style
The check style, based on Dynawo's, is based on a [clang-format file](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) using [clang-format](https://clang.llvm.org/docs/ClangFormat.html). It is recommended for developpers to apply it before commiting to ensure coherence with future developpements.

### SCM
git commits have a standard pattern, similar to the one in Dynawo core. This pattern can be checked by hooks. In order to install this hook, the
developper must run the script:

```bash
$> ./scripts/set_commit_hook.sh
```

This is done automatically when using the script with any option.
