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

To build DynaFlow Launcher you need to clone the repository and launch the following commands in the source code directory, it will create a `myEnvDFL.sh` file that will be your personal entrypoint to launch DFL and parametrise some options.

``` bash
$> git clone https://github.com/dynawo/dynaflow-launcher
$> cd dynaflow-launcher
$> echo '#!/bin/bash

# Required
export DYNAFLOW_LAUNCHER_HOME=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=<PATH_TO_DYNAWO_DEPLOY>
export DYNAFLOW_LAUNCHER_BUILD_TYPE=Release
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

All commands described in the rest of this README are accessible throught this script. To access all options of the script myEnvDFL.sh, type:
``` bash
$> ./myEnvDFL.sh help
```


## Run
To run DynaFlow launcher, use the script dynaflow-launcher.sh provided with the installation. This script will set the required environment variables.

Runtime options of binary are given by doing:

```bash
$> ./dynaflow-launcher.sh --help
```
from the install directlory

Only network file and configuration are required.

You can use the script myEnvDFL.sh with the "launch" or "launch-dir" option:
```bash
$> ./myEnvDFL.sh launch <network> <config>
```
where network is the path to the network file (IIDM) and config the path to the configuration file

```bash
$> ./myEnvDFL.sh launch-dir <dir>
```
where dir is the directory to use. This command is equivalent to
```bash
$> ./myEnvDFL.sh launch <dir>/network.xml <dir>/config.json
```

## Testing
DynaFlow Launcher testing relies on cmake tests.

You can use the script myEnvDFL.sh with the "tests" option. DynaFlow Launcher must be compiled

```bash
$> ./myEnvDFL.sh tests
```

## Coverage
Coverage analysis is based on ctest.
Outputs are gcov and lcov reports.

You can use the script myEnvDFL.sh with the "build-tests-coverage" option.

```bash
$> ./myEnvDFL.sh build-tests-coverage
```

## Developement standard
### Check style
The check style, based on Dynawo's, is based on a [clang-format file](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) using [clang-format](https://clang.llvm.org/docs/ClangFormat.html). It is recommended for developpers to apply it before commiting to ensure coherence with future developpements.

### SCM
git commits have a standard pattern, similar to the one in Dynawo core. This pattern can be checked by hooks.
This hook is installed automatically when using the script myEnvDFL.sh with any option.
