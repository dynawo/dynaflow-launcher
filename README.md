# DynaFlow Launcher

Dynawflow Launcher (DFL) depends on the [core dynawo libraries of Dyna&omega;o](https://github.com/dynawo/dynawo)
and [Dyna&omega;o algorithms](https://github.com/dynawo/dynawo/dynawo-algorithms)

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

Also, Dyna&omega;o Algorithms library must be deployed:
``` bash
$> ./myEnvDynawoAlgorithms.sh build
$> ./myEnvDynawoAlgorithms.sh deploy
```

To build DynaFlow Launcher you need to clone the repository and launch the following commands in the source code directory, it will create a `myEnvDFL.sh` file that will be your personal entrypoint to launch DFL and configure some options.

``` bash
$> git clone https://github.com/dynawo/dynaflow-launcher
$> cd dynaflow-launcher
$> echo '#!/bin/bash

# Required
export DYNAFLOW_LAUNCHER_HOME=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
export DYNAWO_HOME=<PATH_TO_DYNAWO_DEPLOY>
export DYNAWO_ALGORITHMS_HOME=<PATH_TO_DYNAWO_ALGORITHMS_DEPLOY>
export DYNAFLOW_LAUNCHER_BUILD_TYPE=Release
export DYNAFLOW_LAUNCHER_LOCALE=en_GB

# Optional
# export DYNAFLOW_LAUNCHER_CMAKE_GENERATOR=Ninja # default is Unix Makefiles
# export DYNAFLOW_LAUNCHER_PROCESSORS_USED=8 # default 1
# export DYNAFLOW_LAUNCHER_SHARED_LIB=ON # default OFF
# export DYNAFLOW_LAUNCHER_USE_DOXYGEN=OFF # default ON
# export DYNAFLOW_LAUNCHER_BUILD_TESTS=OFF # default ON
# export DYNAFLOW_LAUNCHER_LOG_LEVEL=INFO # default INFO: can be DEBUG, INFO, WARN, ERROR

# Optional external links : optional variables used to add other models to DynaFlow simulation
# DYNAWO_EXTERNAL_HOME=<PATH_TO_EXTERNAL_INSTALL>
# export DYNAFLOW_LAUNCHER_EXTERNAL_DDB=$DYNAWO_EXTERNAL_HOME/ddb
# export DYNAFLOW_LAUNCHER_EXTERNAL_IIDM_EXTENSION=<PATH_TO_IIDM_EXTENSIONS_LIBRARIES>
# export DYNAFLOW_LAUNCHER_EXTERNAL_LIBRARIES=$DYNAWO_EXTERNAL_HOME/lib
# export DYNAFLOW_LAUNCHER_EXTERNAL_RESOURCES_DIR=$DYNAWO_EXTERNAL_HOME/share:$DYNAWO_EXTERNAL_HOME/share/xsd

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
To run DynaFlow launcher, you can use the script myEnvDFL.sh with the "launch" or "launch-dir" option:
```bash
$> ./myEnvDFL.sh launch <network> <config>
```
where network is the path to the network file (IIDM) and config the path to the configuration file

## Testing
DynaFlow Launcher testing relies on cmake tests.

You can use the script myEnvDFL.sh with the "tests" option. DynaFlow Launcher must be compiled

```bash
$> ./myEnvDFL.sh tests
```

For MAIN unit tests, which are composed of complete Dyna&omega;o simulations, reference tests can be updated using the script myEnvDFL.sh
```bash
$> ./myEnvDFL.sh update-references
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
The check style, based on Dynawo's, is based on a [clang-format file](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) using [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

The check style is checked:
- before commiting. This check is performed by hooks. This hook is installed automatically when using the script myEnvDFL.sh with any option. The commit will be refused if formatting was not applied to the commited file.
- at compilation: coding style is applied on all modified files (git POV)

### SCM
git commits have a standard pattern, similar to the one in Dynawo core. This pattern can be checked by hooks.
This hook is installed automatically when using the script myEnvDFL.sh with any option.
