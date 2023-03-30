@echo off

:: Copyright (c) 2015-2022, RTE (http://www.rte-france.com)
:: See AUTHORS.txt
:: All rights reserved.
:: This Source Code Form is subject to the terms of the Mozilla Public
:: License, v. 2.0. If a copy of the MPL was not distributed with this
:: file, you can obtain one at http://mozilla.org/MPL/2.0/.
:: SPDX-License-Identifier: MPL-2.0
::
:: This file is part of Dynawo, an hybrid C++/Modelica open source suite
:: of simulation tools for power systems.


:: internal facility for build option
if /i "%~1"=="_EXIT_" (
  if not "%~2"=="" del "%~2" 2>NUL
  exit /B %~3
)

setlocal

:: check options
set _verbose=
if not defined DYNAWO_BUILD_TYPE set DYNAWO_BUILD_TYPE=Release
set _show=
:NEXT_OPT
if /i "%~1"=="VERBOSE" (
  set _verbose=true
) else if /i "%~1"=="DEBUG" (
  set DYNAWO_BUILD_TYPE=Debug
) else if /i "%~1"=="SHOW" (
  set _show=true
) else (
  goto:LAST_OPT
)
shift /1
goto:NEXT_OPT

:LAST_OPT

:: look for dynaflowLauncher build environment
if not defined DYNAFLOW_LAUNCHER_HOME (
  if exist "%~dp0..\scripts\%~nx0" (
    :: command is run from dynaflowLauncher build environment or in its own deployment directory
    set DYNAFLOW_LAUNCHER_HOME=%~dp0..
  ) else if exist "%~dp0..\dynaflow-launcher\scripts\%~nx0" (
    :: command is run from dynaflowLauncher default installation directory
    set DYNAFLOW_LAUNCHER_HOME=%~dp0..\dynaflow-launcher
  ) else if exist "%~dp0..\..\dynaflow-launcher\scripts\%~nx0" (
    :: command is run from dynaflowLauncher in common deployment directory
    set DYNAFLOW_LAUNCHER_HOME=%~dp0..\..\dynaflow-launcher
  )
)
for %%G in ("%DYNAFLOW_LAUNCHER_HOME%") do set DYNAFLOW_LAUNCHER_HOME=%%~fG
if defined _verbose echo info: using DYNAFLOW_LAUNCHER_HOME=%DYNAFLOW_LAUNCHER_HOME% 1>&2


:: look for dynaflowLauncher installation directory
set DYNAFLOW_LAUNCHER_INSTALL=
if exist "%~dp0bin\DynaFlowLauncher.exe" (
  :: command is run from dynaflowLauncher distribution, deployment or installation directory
  set DYNAFLOW_LAUNCHER_INSTALL=%~dp0.
  if exist "%~dp0lib\cmake\DynaFlowLauncher\DynaFlowLauncherTargets-release.cmake" set DYNAWO_BUILD_TYPE=Release
  if exist "%~dp0lib\cmake\DynaFlowLauncher\DynaFlowLauncherTargets-debug.cmake" set DYNAWO_BUILD_TYPE=Debug
)
if defined _verbose echo info: using DYNAWO_BUILD_TYPE=%DYNAWO_BUILD_TYPE% 1>&2

:: set build directory
if defined DYNAFLOW_LAUNCHER_HOME (
  if %DYNAWO_BUILD_TYPE%==Release set DYNAFLOW_LAUNCHER_BUILD_DIR=%DYNAFLOW_LAUNCHER_HOME%\b
  if %DYNAWO_BUILD_TYPE%==Debug set DYNAFLOW_LAUNCHER_BUILD_DIR=%DYNAFLOW_LAUNCHER_HOME%\bd
)

if defined DYNAFLOW_LAUNCHER_HOME if not defined DYNAFLOW_LAUNCHER_INSTALL (
  setlocal enableDelayedExpansion

  :: command is run from dynaflowLauncher build environment ; try to find installation directory
  for /f "tokens=2 delims==" %%G in ('find "CMAKE_INSTALL_PREFIX:PATH=" "%DYNAFLOW_LAUNCHER_BUILD_DIR%\CMakeCache.txt" 2^>NUL') do set DYNAFLOW_LAUNCHER_INSTALL=%%~fG
  if defined DYNAFLOW_LAUNCHER_INSTALL if not exist "!DYNAFLOW_LAUNCHER_INSTALL!\bin\DynaFlowLauncher.exe" set DYNAFLOW_LAUNCHER_INSTALL=

  if /i "%~1"=="BUILD" (
    :: in case of build, set installation directory from optional argument or to default value
    if not "%~2"=="" set DYNAFLOW_LAUNCHER_INSTALL=%~2
    if not defined DYNAFLOW_LAUNCHER_INSTALL if %DYNAWO_BUILD_TYPE%==Release set DYNAFLOW_LAUNCHER_INSTALL=%DYNAFLOW_LAUNCHER_HOME%\..\dfl-i
    if not defined DYNAFLOW_LAUNCHER_INSTALL if %DYNAWO_BUILD_TYPE%==Debug set DYNAFLOW_LAUNCHER_INSTALL=%DYNAFLOW_LAUNCHER_HOME%\..\dfl-id
  )
  setlocal disableDelayedExpansion
)
for %%G in ("%DYNAFLOW_LAUNCHER_INSTALL%") do set DYNAFLOW_LAUNCHER_INSTALL=%%~fG
if defined _verbose echo info: using DYNAFLOW_LAUNCHER_INSTALL=%DYNAFLOW_LAUNCHER_INSTALL% 1>&2


:: look for dynawoAlgorithms deploy directory
if not defined DYNAWO_ALGORITHMS_HOME (
  if exist "%~dp0bin\dynawoAlgorithms.exe" (
    :: command is run from a dynaflowLauncher distribution directory
    set DYNAWO_ALGORITHMS_HOME=%~dp0.
  ) else (
    :: dynawoAlgorithms deployment directory is found in path
    for /f "tokens=*" %%G in ('where dynawoAlgorithms.exe 2^>NUL') do set DYNAWO_ALGORITHMS_HOME=%%~dpG..
  )
)
if defined DYNAWO_ALGORITHMS_HOME goto:DYNAWO_ALGORITHMS_FOUND

set _dynawo_algorithms_release=
set _dynawo_algorithms_debug=

:: command is run from dynaflowLauncher default installation directory with dynawoAlgorithms in common deployment directory
call:FIND_DYNAWO_ALGORITHMS ..
:: command is run from dynaflowLauncher default installation directory with dynawoAlgorithms in its own deployment directory
call:FIND_DYNAWO_ALGORITHMS ..\dynawo-algorithms
:: command is run from dynaflowLauncher in common deployment directory with dynawoAlgorithms in common deployment directory
call:FIND_DYNAWO_ALGORITHMS ..\..
:: command is run from dynaflowLauncher in common deployment directory with dynawoAlgorithms in its own deployment directory
call:FIND_DYNAWO_ALGORITHMS ..\..\dynawo-algorithms
:: command is run from dynaflowLauncher build environment (or in its own deployment directory) with dynawoAlgorithms in common deployment directory
call:FIND_DYNAWO_ALGORITHMS ..\..\..
:: command is run from dynaflowLauncher build environment (or in its own deployment directory) with dynawoAlgorithms in its own deployment directory
call:FIND_DYNAWO_ALGORITHMS ..\..\..\dynawo-algorithms

:: keep the right dynawoAlgorithms according build type
set DYNAWO_ALGORITHMS_HOME=%_dynawo_algorithms_release%
if %DYNAWO_BUILD_TYPE%==Debug set DYNAWO_ALGORITHMS_HOME=%_dynawo_algorithms_debug%

:DYNAWO_ALGORITHMS_FOUND
for %%G in ("%DYNAWO_ALGORITHMS_HOME%") do set DYNAWO_ALGORITHMS_HOME=%%~fG
if defined _verbose echo info: using DYNAWO_ALGORITHMS_HOME=%DYNAWO_ALGORITHMS_HOME% 1>&2


:: look for dynawo deploy directory
if not defined DYNAWO_HOME (
  if exist "%~dp0bin\dynawo.exe" (
    :: command is run from a dynawoAlgorithms distribution directory
    set DYNAWO_HOME=%~dp0.
  ) else (
    :: dynawo deployment directory is found in path
    for /f "tokens=*" %%G in ('where dynawo.exe 2^>NUL') do set DYNAWO_HOME=%%~dpG..
  )
)
if defined DYNAWO_HOME goto:DYNAWO_FOUND

set _dynawo_release=
set _dynawo_debug=

:: command is run from dynaflowLauncher default installation directory with dynawo in common deployment directory
call:FIND_DYNAWO ..
:: command is run from dynaflowLauncher default installation directory with dynawo in its own deployment directory
call:FIND_DYNAWO ..\dynawo
:: command is run from dynaflowLauncher in common deployment directory with dynawo in common deployment directory
call:FIND_DYNAWO ..\..
:: command is run from dynaflowLauncher in common deployment directory with dynawo in its own deployment directory
call:FIND_DYNAWO ..\..\dynawo
:: command is run from dynaflowLauncher build environment (or in its own deployment directory) with dynawo in common deployment directory
call:FIND_DYNAWO ..\..\..
:: command is run from dynaflowLauncher build environment (or in its own deployment directory) with dynawo in its own deployment directory
call:FIND_DYNAWO ..\..\..\dynawo

:: keep the right dynawo according build type
set DYNAWO_HOME=%_dynawo_release%
if %DYNAWO_BUILD_TYPE%==Debug set DYNAWO_HOME=%_dynawo_debug%

:DYNAWO_FOUND
for %%G in ("%DYNAWO_HOME%") do set DYNAWO_HOME=%%~fG
if defined _verbose echo info: using DYNAWO_HOME=%DYNAWO_HOME% 1>&2


:: check preconditions
if not defined DYNAWO_HOME (
  echo error: unable to find dynawo deployment directory ^(please set DYNAWO_HOME^) ! 1>&2
  exit /B 1
)

if %DYNAWO_BUILD_TYPE%==Release if not exist "%DYNAWO_HOME%\share\dynawo-config-release.cmake" (
  echo error: incompatible dynawo deployment directory ^(should be Release^) ! 1>&2
  exit /B 1
)
if %DYNAWO_BUILD_TYPE%==Debug if not exist "%DYNAWO_HOME%\share\dynawo-config-debug.cmake" (
  echo error: incompatible dynawo deployment directory ^(should be Debug^) ! 1>&2
  exit /B 1
)

if not defined DYNAWO_ALGORITHMS_HOME (
  echo error: unable to find dynawoAlgorithms deployment directory ^(please set DYNAWO_ALGORITHMS_HOME^) ! 1>&2
  exit /B 1
)

if %DYNAWO_BUILD_TYPE%==Release if not exist "%DYNAWO_ALGORITHMS_HOME%\share\dynawoalgorithms-config-release.cmake" (
  echo error: incompatible dynawoAlgorithms deployment directory ^(should be Release^) ! 1>&2
  exit /B 1
)
if %DYNAWO_BUILD_TYPE%==Debug if not exist "%DYNAWO_ALGORITHMS_HOME%\share\dynawoalgorithms-config-debug.cmake" (
  echo error: incompatible dynawoAlgorithms deployment directory ^(should be Debug^) ! 1>&2
  exit /B 1
)

if not defined DYNAFLOW_LAUNCHER_INSTALL if not "%~1"=="" if /i not "%~1"=="HELP" if /i not "%~1"=="CLEAN" if /i not "%~1"=="TESTS" (
  echo error: you have to build dynaflowLauncher before being able to use it ! 1>&2
  exit /B 1
)


:: check python interpreter
if /i not "%~1"=="BUILD" if /i not "%~1"=="TESTS" goto:PYTHON_UNUSED
if not defined DYNAWO_PYTHON_COMMAND set DYNAWO_PYTHON_COMMAND=python
"%DYNAWO_PYTHON_COMMAND%" --version >NUL 2>&1
if %ERRORLEVEL% neq 0 (
  echo error: the python interpreter ^"%DYNAWO_PYTHON_COMMAND%^" does not work ^(please set DYNAWO_PYTHON_COMMAND^) ! 1>&2
  exit /B 1
)
:PYTHON_UNUSED


:: check developper mode
set _devmode=
:: if there is a CMakeLists.txt, developper mode is on
if defined DYNAFLOW_LAUNCHER_HOME if exist "%DYNAFLOW_LAUNCHER_HOME%\CMakeLists.txt" (
  set _devmode=true
)

:: check command
set _exitcode=0
if "%~1"=="" goto:HELP
if /i %~1==N goto:LAUNCH
if /i %~1==SA goto:LAUNCH
if /i %~1==NSA goto:LAUNCH
if /i %~1==VERSION goto:VERSION
if /i %~1==BUILD goto:BUILD
if /i %~1==CLEAN goto:CLEAN
if /i %~1==TESTS goto:TESTS
if /i %~1==HELP goto:HELP

:: show help in case of invalid command
echo error: %~1 is not a valid command ! 1>&2
:ERROR
echo. 1>&2
set _exitcode=1


:HELP
if defined _devmode (
  echo usage: %~n0 [VERBOSE] [DEBUG] [HELP ^| ^<command^>]
) else (
  echo usage: %~n0 [VERBOSE] [HELP ^| ^<command^>]
)
echo.
echo HELP command displays this message.
echo Add VERBOSE option to echo environment variables used.
if defined _devmode echo All commands are run in Release mode by default. Add DEBUG option to turn in Debug mode.
if defined DYNAFLOW_LAUNCHER_INSTALL if exist "%DYNAFLOW_LAUNCHER_INSTALL%\bin\DynaFlowLauncher.exe" (
  echo.
  echo These are commands used by end-user:
  echo   N   ^<network^> ^<config^>                              Run steady state simulation
  echo   SA  ^<network^> ^<config^> ^<contingencies^> [^<nbprocs^>]  Run security analysis
  echo   NSA ^<network^> ^<config^> ^<contingencies^> [^<nbprocs^>]  Run steady state simulation followed by security analysis
  echo   version                                             Print dynaflowLauncher version
  echo.
  echo where ^<network^> is the path of IIDM network file
  echo       ^<config^> is the path of DFL configuration file ^(JSON format^)
  echo       ^<contingencies^> is the path of SA contingencies file ^(JSON format^)
  echo       ^<nbprocs^> is the optional number of MPI processes to use for SA ^(default 1^)
)
if defined _devmode (
  echo.
  echo These are commands used by developer only:
  echo   build [^<install_dir^>]           Build, install and deploy dynaflowLauncher
  echo   [SHOW] tests [^<unit_test^>]      Build and run unit tests or a specific unit test ^(option SHOW lists available unit tests^)
  echo   clean                           Clean build directory
)
exit /B %_exitcode%%


:: launch simulation
:LAUNCH
if not exist "%~2" (
  echo error: IIDM network file %2 doesn't exist ! 1>&2
  goto:ERROR
)
if not exist "%~3" (
  echo error: DFL configuration file %3 doesn't exist ! 1>&2
  goto:ERROR
)
set _nsa=
if /i %~1==SA goto:SA
if /i %~1==NSA goto:NSA

:: N simulation
if not "%~4"=="" (
  echo error: too many options ! 1>&2
  goto:ERROR
)
call:SET_ENV
"%DYNAFLOW_LAUNCHER_INSTALL%\bin\DynaFlowLauncher" --log-level %DYNAFLOW_LAUNCHER_LOG_LEVEL% --network %2 --config %3
exit /B %ERRORLEVEL%

:: NSA simulation
:NSA
set _nsa=--nsa

:: SA simulation
:SA
if not exist "%~4" (
  echo error: SA contingencies file %4 doesn't exist ! 1>&2
  goto:ERROR
)

:: check MPI runtime
mpiexec >NUL 2>&1
if %ERRORLEVEL% neq 0 ( 
  echo error: Microsoft MPI Runtime should be installed ! 1>&2
  exit /B 1
)
set _nbprocs=1
if not [%~5]==[] set _nbprocs=%~5

call:SET_ENV
mpiexec -n %_nbprocs% "%DYNAFLOW_LAUNCHER_INSTALL%\bin\DynaFlowLauncher" --log-level %DYNAFLOW_LAUNCHER_LOG_LEVEL% --network %2 --config %3 --contingencies %4 %_nsa%
exit /B %ERRORLEVEL%


:: print version
:VERSION
call:SET_ENV
"%DYNAFLOW_LAUNCHER_INSTALL%\bin\DynaFlowLauncher" -v
exit /B %ERRORLEVEL%


:: build dynaflowLauncher
:BUILD
if not defined _devmode (
  echo error: 'build' command is only available in developper mode ! 1>&2
  exit /B 1
)

:: default is to deploy while installing
if not defined DYNAFLOW_LAUNCHER_DEPLOY set DYNAFLOW_LAUNCHER_DEPLOY=ON

:: delegate to a temporary cmd file in case of self update !
cmake -E make_directory %DYNAFLOW_LAUNCHER_INSTALL%
set _build_tmp=%DYNAFLOW_LAUNCHER_INSTALL%\~build.tmp.cmd
(
  echo cmake -S "%DYNAFLOW_LAUNCHER_HOME%" -B "%DYNAFLOW_LAUNCHER_BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%DYNAFLOW_LAUNCHER_INSTALL%" -DDYNAFLOW_LAUNCHER_DEPLOY=%DYNAFLOW_LAUNCHER_DEPLOY% -DDYNAFLOW_LAUNCHER_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=%DYNAWO_BUILD_TYPE% -DDYNAWO_ALGORITHMS_HOME="%DYNAWO_ALGORITHMS_HOME%" -DDYNAWO_HOME="%DYNAWO_HOME%" -DDYNAFLOW_LAUNCHER_THIRD_PARTY_DIR="%DYNAFLOW_LAUNCHER_HOME%" -DDYNAWO_PYTHON_COMMAND="%DYNAWO_PYTHON_COMMAND%" -G "NMake Makefiles" -Wno-dev
  echo cmake --build "%DYNAFLOW_LAUNCHER_BUILD_DIR%" --target install
  echo %0 _EXIT_ "%_build_tmp%" %ERRORLEVEL%
) > "%_build_tmp%"
:: pass control to temporary batch file
"%_build_tmp%"

:: must never be reached
goto:eof


:: clean build directory
:CLEAN
if not defined _devmode (
  echo error: 'clean' command is only available in developper mode ! 1>&2
  exit /B 1
)
rd /s /q "%DYNAFLOW_LAUNCHER_BUILD_DIR%" 2>NUL
exit /B %ERRORLEVEL%


:: build and run unit tests
:TESTS
if not defined _devmode (
  echo error: 'tests' command is only available in developper mode ! 1>&2
  exit /B 1
)

:: indicates where a GTest installation may already exist
if not defined DYNAWO_GTEST_HOME set DYNAWO_GTEST_HOME=%DYNAWO_HOME%
if not defined DYNAWO_GMOCK_HOME set DYNAWO_GMOCK_HOME=%DYNAWO_HOME%

:: build with GTest and run all tests or a specific test
set _test=%~2
if "%~2"=="" set _test=.*
cmake -S "%DYNAFLOW_LAUNCHER_HOME%" -B "%DYNAFLOW_LAUNCHER_BUILD_DIR%" -DCMAKE_INSTALL_PREFIX="%DYNAFLOW_LAUNCHER_INSTALL%" -DDYNAFLOW_LAUNCHER_BUILD_TESTS=ON -DDYNAFLOW_LAUNCHER_REGEX_TESTS="%_test%" -DGTEST_ROOT="%DYNAWO_GTEST_HOME%" -DGMOCK_HOME="%DYNAWO_GMOCK_HOME%" -DCMAKE_BUILD_TYPE=%DYNAWO_BUILD_TYPE% -DDYNAWO_ALGORITHMS_HOME="%DYNAWO_ALGORITHMS_HOME%" -DDYNAWO_HOME="%DYNAWO_HOME%" -DDYNAFLOW_LAUNCHER_THIRD_PARTY_DIR="%DYNAFLOW_LAUNCHER_HOME%" -DDYNAWO_PYTHON_COMMAND="%DYNAWO_PYTHON_COMMAND%" -G "NMake Makefiles" -Wno-dev
if defined _show goto:LIST_TESTS
cmake --build "%DYNAFLOW_LAUNCHER_BUILD_DIR%" --target tests
pushd "%DYNAFLOW_LAUNCHER_BUILD_DIR%"
ctest -R "%_test%"
popd
exit /B %ERRORLEVEL%

:: list unit tests
:LIST_TESTS
echo.
echo Available unit tests are :
pushd "%DYNAFLOW_LAUNCHER_BUILD_DIR%"
for /f "tokens=3" %%G in ('ctest -N  -R %_test% ^| find "  Test "') do echo     %%G
popd
exit /B %ERRORLEVEL%


:: look for dynawAlgorithms installation
:FIND_DYNAWO_ALGORITHMS
if not defined _dynawo_algorithms_release if exist "%~dp0%1\deploy\dynawo-algorithms\share\dynawoalgorithms-config-release.cmake" set _dynawo_algorithms_release=%~dp0%1\deploy\dynawo-algorithms
if not defined _dynawo_algorithms_debug   if exist "%~dp0%1\deployd\dynawo-algorithms\share\dynawoalgorithms-config-debug.cmake"  set _dynawo_algorithms_debug=%~dp0%1\deployd\dynawo-algorithms
if not defined _dynawo_algorithms_debug   if exist "%~dp0%1\deploy\dynawo-algorithms\share\dynawoalgorithms-config-debug.cmake"   set _dynawo_algorithms_debug=%~dp0%1\deploy\dynawo-algorithms
exit /B 0


:: look for dynawo installation
:FIND_DYNAWO
if not defined _dynawo_release if exist "%~dp0%1\deploy\dynawo\share\dynawo-config-release.cmake" set _dynawo_release=%~dp0%1\deploy\dynawo
if not defined _dynawo_debug   if exist "%~dp0%1\deployd\dynawo\share\dynawo-config-debug.cmake"  set _dynawo_debug=%~dp0%1\deployd\dynawo
if not defined _dynawo_debug   if exist "%~dp0%1\deploy\dynawo\share\dynawo-config-debug.cmake"   set _dynawo_debug=%~dp0%1\deploy\dynawo
exit /B 0


:: set dynawo environment variables for runtime
:SET_ENV
if not defined DYNAFLOW_LAUNCHER_LOCALE set DYNAFLOW_LAUNCHER_LOCALE=en_GB
if defined DYNAFLOW_LAUNCHER_INSTALL set DYNAFLOW_LAUNCHER_XSD=%DYNAFLOW_LAUNCHER_INSTALL%\etc\xsd
set DYNAFLOW_LAUNCHER_LIBRARIES=%DYNAWO_HOME%\ddb
if not defined DYNAFLOW_LAUNCHER_LOG_LEVEL set DYNAFLOW_LAUNCHER_LOG_LEVEL=INFO

set DYNAWO_ALGORITHMS_LOCALE=%DYNAFLOW_LAUNCHER_LOCALE%
set DYNAWO_ALGORITHMS_XSD_DIR=%DYNAWO_ALGORITHMS_HOME%\share\xsd

set DYNAWO_INSTALL_DIR=%DYNAWO_HOME%
set DYNAWO_DDB_DIR=%DYNAFLOW_LAUNCHER_LIBRARIES%
set DYNAWO_DICTIONARIES=dictionaries_mapping
if not defined DYNAWO_USE_XSD_VALIDATION set DYNAWO_USE_XSD_VALIDATION=false
set DYNAWO_RESOURCES_DIR=%DYNAWO_HOME%\share
set DYNAWO_XSD_DIR=%DYNAWO_RESOURCES_DIR%\xsd\
set DYNAWO_RESOURCES_DIR=%DYNAWO_RESOURCES_DIR%;%DYNAWO_XSD_DIR%
if not "%DYNAWO_HOME%"=="%DYNAWO_ALGORITHMS_HOME%" set DYNAWO_RESOURCES_DIR=%DYNAWO_ALGORITHMS_HOME%\share;%DYNAWO_ALGORITHMS_XSD_DIR%;%DYNAWO_RESOURCES_DIR%
if defined DYNAFLOW_LAUNCHER_INSTALL set DYNAWO_RESOURCES_DIR=%DYNAFLOW_LAUNCHER_XSD%;%DYNAWO_RESOURCES_DIR%

set DYNAWO_LIBIIDM_EXTENSIONS=%DYNAWO_HOME%\bin
if not defined DYNAWO_IIDM_EXTENSION if exist "%DYNAWO_LIBIIDM_EXTENSIONS%\dynawo_RTE_DataInterfaceIIDMExtension.dll" (
  set DYNAWO_IIDM_EXTENSION=%DYNAWO_LIBIIDM_EXTENSIONS%\dynawo_RTE_DataInterfaceIIDMExtension.dll
)

:: set path for runtime
set PATH=%DYNAWO_DDB_DIR%;%DYNAWO_HOME%\bin;%PATH%
if not "%DYNAWO_HOME%"=="%DYNAWO_ALGORITHMS_HOME%" set PATH=%DYNAWO_ALGORITHMS_HOME%\bin;%PATH%
if defined DYNAFLOW_LAUNCHER_INSTALL if not "%DYNAWO_HOME%"=="%DYNAFLOW_LAUNCHER_INSTALL%" set PATH=%DYNAFLOW_LAUNCHER_INSTALL%\bin;%PATH%

exit /B 0
