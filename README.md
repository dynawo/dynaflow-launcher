<!--
    Copyright (c) 2022, RTE (http://www.rte-france.com)
    See AUTHORS.txt
    All rights reserved.
    This Source Code Form is subject to the terms of the Mozilla Public
    License, v. 2.0. If a copy of the MPL was not distributed with this
    file, you can obtain one at http://mozilla.org/MPL/2.0/.
    SPDX-License-Identifier: MPL-2.0

    This file is part of Dynawo, an hybrid C++/Modelica open source suite
    of simulation tools for power systems.
-->

# Dynaflow-launcher

[![Build Status](https://github.com/dynawo/dynaflow-launcher/workflows/CI/badge.svg)](https://github.com/dynawo/dynaflow-launcher/actions)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=dynawo_dynaflow-launcher&metric=alert_status)](https://sonarcloud.io/summary/new_code?id=dynawo_dynaflow-launcher)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=dynawo_dynaflow-launcher&metric=coverage)](https://sonarcloud.io/summary/new_code?id=dynawo_dynaflow-launcher)
[![MPL-2.0 License](https://img.shields.io/badge/license-MPL_2.0-blue.svg)](https://www.mozilla.org/en-US/MPL/2.0/)

**Dynaflow-launcher** is a utility tool used to easily run [Dynaflow](https://dynawo.github.io/about/dynaflow) starting from a minimal set of inputs.

It provides the following possibilities:
- **Unitary simulations**: computation of the steady-state solution on a given network (IIDM format) with Dynaflow;
- **Systematic analysis**: assessment of the stability of a single base network subject to different events;

Dynaflow-launcher depends on the [core dynawo libraries of Dyna&omega;o](https://github.com/dynawo/dynawo)
and [Dyna&omega;o algorithms](https://github.com/dynawo/dynawo-algorithms).

## Get involved!
Dynaflow-launcher is an open-source project and as such, questions, discussions, feedbacks and more generally any form of contribution are very welcome and greatly appreciated! For further informations about contributing guidelines, please refers to the [contributing documentation](https://github.com/dynawo/.github/blob/master/CONTRIBUTING.md).

## Dynaflow-launcher Distribution

You can download a pre-built Dynaflow-launcher release to start testing it. Pre-built releases are available for **Linux**:
- [Linux](https://github.com/dynawo/dynaflow-launcher/releases/download/v1.3.2/DynaFlowLauncher_Linux_v1.3.2.zip)

### Linux Requirements for Distribution

- Compilers: C and C++ ([gcc](https://www.gnu.org/software/gcc/) or [clang](https://clang.llvm.org/)), C++11 compatible for C++ standard
- Python2 or Python3
- Binary utilities: [curl](https://curl.haxx.se/) and unzip
- [CMake](https://cmake.org/)

**Note** For Python you need to have the `python` command available in your PATH. If you don't have one, you can use an environment variable to point to your Python version with `export DYNAWO_PYTHON_COMMAND="python3"`.

You can install the dependencies for Ubuntu or Fedora with:

``` bash
$> apt install -y g++ unzip curl python
$> dnf install -y gcc-c++ unzip curl python
```

### Using a distribution

#### Linux

You can launch the following commands to download and test the latest distribution:

``` bash
$> curl -L $(curl -s -L -X GET https://api.github.com/repos/dynawo/dynaflow-launcher/releases/latest | grep "DynaFlowLauncher_Linux" | grep url | cut -d '"' -f 4) -o DynaflowLauncher_Linux_latest.zip
$> unzip DynaflowLauncher_Linux_latest.zip
$> cd dynaflow-launcher
$> ./dynaflow-launcher.sh help
```

## Building Dynaflow-launcher

### Dyna&omega;o deploy

To build Dynaflow-launcher, you must first deploy the [Dyna&omega;o library](https://github.com/dynawo/dynawo).

``` bash
$> ./myEnvDynawo.sh build-all
$> ./myEnvDynawo.sh deploy
```

This command creates a deploy folder in ${DYNAWO_HOME}.
The path to dynawo deploy is then the path to the subdirectory `dynawo` in the deploy folder. It is generally similar to:

``` bash
PATH_TO_DYNAWO_DEPLOY=${DYNAWO_HOME}/deploy/gcc8/shared/dynawo/
```
### Dyna&omega;o-algorithms deploy

The [Dyna&omega;o Algorithms library](https://github.com/dynawo/dynawo-algorithms) must also be deployed:

``` bash
$> ./myEnvDynawoAlgorithms.sh build
$> ./myEnvDynawoAlgorithms.sh deploy
```

The latter command creates a deploy folder in ${DYNAWO_ALGORITHMS_HOME}.
The path to dynawo-algorithms deploy is then the path to the subdirectory `dynawo-algorithms` in the deploy folder. It is generally similar to:

``` bash
PATH_TO_DYNAWO_ALGORITHMS_DEPLOY=${DYNAWO_ALGORITHMS_HOME}/deploy/gcc8/dynawo-algorithms/
```
### Dynaflow-launcher

To build Dynaflow-launcher you need to clone the repository and launch the following commands in the source code directory, it will create a `myEnvDFL.sh` file that will be your personal entrypoint to launch DFL and configure some options.

``` bash
$> git clone https://github.com/dynawo/dynaflow-launcher
$> cd dynaflow-launcher
$> echo '#!/bin/bash

# Required
export DYNAFLOW_LAUNCHER_HOME=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
# Variables used to build Dynaflow-launcher
export DYNAWO_HOME=<PATH_TO_DYNAWO_DEPLOY>
export DYNAWO_ALGORITHMS_HOME=<PATH_TO_DYNAWO_ALGORITHMS_DEPLOY>
export DYNAFLOW_LAUNCHER_BUILD_TYPE=Release

# Optional
# export DYNAFLOW_LAUNCHER_LOCALE=en_GB
# export DYNAFLOW_LAUNCHER_CMAKE_GENERATOR=Ninja # default is Unix Makefiles
# export DYNAFLOW_LAUNCHER_PROCESSORS_USED=8 # default 1
# export DYNAFLOW_LAUNCHER_BUILD_TESTS=OFF # default ON
# export DYNAFLOW_LAUNCHER_LOG_LEVEL=INFO # default INFO: can be DEBUG, INFO, WARN, ERROR
# export DYNAFLOW_LAUNCHER_BROWER=firefox # browser command used to visualize test coverage. default: firefox

# Optional external links : optional variable used at runtime to use additional iidm extension
# export DYNAWO_IIDM_EXTENSION=<PATH_TO_IIDM_EXTENSIONS_LIBRARY>

# Run
$DYNAFLOW_LAUNCHER_HOME/scripts/envDFL.sh $@' > myEnvDFL.sh
$> chmod +x myEnvDFL.sh
```

Then update the path "PATH_TO_DYNAWO_DEPLOY" in the file to your deployed installation of Dyna&omega;o, as well as the path "PATH_TO_DYNAWO_ALGORITHMS_DEPLOY", and then launch the following command:
``` bash
$> ./myEnvDFL.sh build-user
```

All commands described in the rest of this README are accessible throught this script. To access all options of the script myEnvDFL.sh, type:
``` bash
$> ./myEnvDFL.sh help
```

## Run steady-state simulation
To run Dynaflow-launcher, you can use the script myEnvDFL.sh with the "launch" option:
```bash
$> ./myEnvDFL.sh launch <network> <config>
```
where network is the path to the network file (IIDM) and config the path to the configuration file.

## Run systematic analysis
To run a systematic analysis with Dynaflow-launcher, you can use the script myEnvDFL.sh with the "launch-sa" option:
```bash
$> ./myEnvDFL.sh launch-sa <network> <config> <contingencies>
```
where network is the path to the network file (IIDM), config the path to the configuration file and contingencies the path to the contingency file (json).

## Run steady-state simulation automatically followed by a systematic analysis
To run a steady-state simulation automatically followed by a systematic analysis with Dynaflow-launcher, you can use the script myEnvDFL.sh with the "launch-nsa" option:
```bash
$> ./myEnvDFL.sh launch-nsa <network> <config> <contingencies>
```
where network is the path to the network file (IIDM), config the path to the configuration file and contingencies the path to the contingency file (json).

## Testing
Dynaflow-launcher testing relies on cmake tests.

You can use the script myEnvDFL.sh with the "tests" option. Dynaflow-launcher must be compiled.

```bash
$> ./myEnvDFL.sh tests
```

For MAIN and MAIN_SA unit tests, which are composed of complete Dyna&omega;o simulations, reference tests can be updated using the script myEnvDFL.sh
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
- at compilation: coding style is applied on all modified files (git POV).

### SCM
git commits have a standard pattern, similar to the one in Dynawo core. This pattern can be checked by hooks.
This hook is installed automatically when using the script myEnvDFL.sh with any option.

## Dynaflow-launcher Documentation
You can download Dynaflow-launcher documentation [here](https://github.com/dynawo/dynaflow-launcher/releases/download/v1.3.2/DynaflowLauncherDocumentation.pdf).

## Quoting Dyna&omega;o

If you use Dyna&omega;o, Dyna&omega;o-algorithms or Dynaflow-launcher in your work or research, it is not mandatory but we kindly ask you to quote the following paper in your publications or presentations:

A. Guironnet, M. Saugier, S. Petitrenaud, F. Xavier, and P. Panciatici, “Towards an Open-Source Solution using Modelica for Time-Domain Simulation of Power Systems,” 2018 IEEE PES Innovative Smart Grid Technologies Conference Europe (ISGT-Europe), Oct. 2018.

## License

Dynaflow-launcher is licensed under the terms of the Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with this file, you can obtain one at http://mozilla.org/MPL/2.0. You can also see the [LICENSE](LICENSE.txt) file for more information.

Dynaflow-launcher is using some external libraries to run simulations:
* [MPICH](https://www.mpich.org/), an implementation of the Message Passing Interface (MPI) standard distributed under a BSD-like license. Dynaflow-launcher currently using the version 3.4.2.

## Maintainers

Dynaflow-launcher is currently maintained by the following people in RTE:

* Mathilde Bongrain, [mathilde.bongrain@rte-france.com](mailto:mathilde.bongrain@rte-france.com)
* Gautier Bureau, [gautier.bureau@rte-france.com](mailto:gautier.bureau@rte-france.com)
* Marco Chiaramello, [marco.chiaramello@rte-france.com](mailto:marco.chiaramello@rte-france.com)
* Quentin Cossart, [quentin.cossart@rte-france.com](mailto:quentin.cossart@rte-france.com)
* Adrien Guironnet, [adrien.guironnet@rte-france.com](mailto:adrien.guironnet@rte-france.com)
* Florentine Rosiere, [florentine.rosiere@rte-france.com](mailto:florentine.rosiere@rte-france.com)

In case of questions or issues, you can also send an e-mail to [rte-dynawo@rte-france.com](mailto:rte-dynawo@rte-france.com).

## Links

For more information about Dyna&omega;o and Dyna&omega;o-algorithms:

* Consult [Dyna&omega;o website](http://dynawo.org)
* Contact us at [rte-dynawo@rte-france.com](mailto:rte-dynawo@rte-france.com)
