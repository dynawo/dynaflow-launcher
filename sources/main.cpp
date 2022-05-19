//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

#include "Configuration.h"
#include "Context.h"
#include "Contingencies.h"
#include "Log.h"
#include "Options.h"
#include "gitversion_dfl.h"
#include "version.h"

#include <DYNError.h>
#include <DYNInitXml.h>
#include <DYNIoDico.h>
#include <DYNMPIContext.h>
#include <boost/filesystem.hpp>
#include <chrono>
#include <cstdlib>
#include <sstream>

static std::string
getMandatoryEnvVar(const std::string& key) {
  char* var = getenv(key.c_str());
  if (var != NULL) {
    return std::string(var);
  } else {
    // we cannot use dictionnary errors since they may not be initialized yet
    throw Error(EnvVariableMissing, key);
  }
}

static void
initializeDynawo(const std::string& locale) {
  DYN::IoDicos& dicos = DYN::IoDicos::instance();
  dicos.addPath(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
  dicos.addDico("ERROR", "DYNError", locale);
  dicos.addDico("TIMELINE", "DYNTimeline", locale);
  dicos.addDico("CONSTRAINT", "DYNConstraint", locale);
  dicos.addDico("LOG", "DYNLog", locale);
  dicos.addDico("DFLLOG", "DFLLog", locale);
  dicos.addDico("DFLERROR", "DFLError", locale);
}

static inline double
elapsed(const std::chrono::steady_clock::time_point& timePoint) {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timePoint);
  return static_cast<double>(duration.count()) / 1000;  // To have the time in seconds as a double
}

static dfl::Context::SimulationKind
getSimulationKind(const dfl::common::Options::RuntimeConfiguration& runtimeConfig) {
  if (runtimeConfig.contingenciesFilePath.empty()) {
    return dfl::Context::SimulationKind::STEADY_STATE_CALCULATION;
  } else {
    return dfl::Context::SimulationKind::SECURITY_ANALYSIS;
  }
}

int
main(int argc, char* argv[]) {
  auto& mpiContext = DYNAlgorithms::mpi::context();  // MUST be at the beginning to initialize the instance
  DYN::InitXerces xerces;
  DYN::InitLibXml2 libxml2;
  auto timeStart = std::chrono::steady_clock::now();
  boost::shared_ptr<dfl::Context> context;
  DYN::Trace::init();
  dfl::common::Options options;

  auto parsing_status = options.parse(argc, argv);

  if (!std::get<0>(parsing_status) || std::get<1>(parsing_status) == dfl::common::Options::Request::HELP) {
    if (mpiContext.isRootProc())
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag) << options.desc() << DYN::Trace::endline;
    return EXIT_SUCCESS;
  }
  if (std::get<1>(parsing_status) == dfl::common::Options::Request::VERSION) {
    if (mpiContext.isRootProc())
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag) << DYNAFLOW_LAUNCHER_VERSION_STRING
          << " (rev:" << DYNAFLOW_LAUNCHER_GIT_BRANCH << "-" << DYNAFLOW_LAUNCHER_GIT_HASH << DYN::Trace::endline;
    return EXIT_SUCCESS;
  }

  auto& runtimeConfig = options.config();
  dfl::inputs::Configuration config(boost::filesystem::path(runtimeConfig.configPath));
  try {
    if (mpiContext.isRootProc()) {
      dfl::common::Log::init(options, config.outputDir().generic_string());
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag)
          << " " << runtimeConfig.programName << " v" << DYNAFLOW_LAUNCHER_VERSION_STRING << DYN::Trace::endline;
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag)
          << " "
          << " revision " << DYNAFLOW_LAUNCHER_GIT_BRANCH << "-" << DYNAFLOW_LAUNCHER_GIT_HASH << DYN::Trace::endline;
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }

    std::string root = getMandatoryEnvVar("DYNAFLOW_LAUNCHER_INSTALL");
    std::string locale = getMandatoryEnvVar("DYNAFLOW_LAUNCHER_LOCALE");
    auto res = boost::filesystem::path(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
    initializeDynawo(locale);

    if (!boost::filesystem::exists(boost::filesystem::path(runtimeConfig.networkFilePath))) {
      throw Error(NetworkFileNotFound, runtimeConfig.networkFilePath);
    }
    if (!runtimeConfig.contingenciesFilePath.empty() && !boost::filesystem::exists(boost::filesystem::path(runtimeConfig.contingenciesFilePath))) {
      throw Error(ContingenciesFileNotFound, runtimeConfig.contingenciesFilePath);
    }

    auto simulationKind = getSimulationKind(runtimeConfig);
    switch (simulationKind) {
    case dfl::Context::SimulationKind::STEADY_STATE_CALCULATION:
      LOG(info, SteadyStateInfo, runtimeConfig.networkFilePath, runtimeConfig.configPath);
      break;
    case dfl::Context::SimulationKind::SECURITY_ANALYSIS:
      LOG(info, SecurityAnalysisInfo, runtimeConfig.networkFilePath, runtimeConfig.contingenciesFilePath, runtimeConfig.configPath);
      break;
    }

    boost::filesystem::path parFilesDir(root);
    parFilesDir.append("etc");

    dfl::Context::ContextDef def{simulationKind,
                                 runtimeConfig.networkFilePath,
                                 config.settingFilePath(),
                                 config.assemblingFilePath(),
                                 runtimeConfig.contingenciesFilePath,
                                 runtimeConfig.dynawoLogLevel,
                                 parFilesDir,
                                 res,
                                 locale};
    context = boost::shared_ptr<dfl::Context>(new dfl::Context(def, config));

    if (!context->process()) {
      LOG(info, InitEnd, elapsed(timeStart));
      throw Error(ContextProcessError, context->basename());
    }
    LOG(info, InitEnd, elapsed(timeStart));

    auto timeFilesStart = std::chrono::steady_clock::now();
    context->exportOutputs();
    LOG(info, FilesEnd, elapsed(timeFilesStart));
  } catch (DYN::Error& e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Initialization failed: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " Initialization failed: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  } catch (DYN::MessageError& e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Initialization failed: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " Initialization failed: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  } catch (std::exception& e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Initialization failed: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " Initialization failed: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  } catch (...) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Initialization failed" << std::endl;
      std::cerr << "Unknown error" << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " Initialization failed" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " Unknown error" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  }
  DYNAlgorithms::mpi::Context::sync();

  try {
    auto timeSimuStart = std::chrono::steady_clock::now();
    context->execute();

    if (mpiContext.isRootProc()) {
      LOG(info, SimulationEnded, context->basename(), elapsed(timeSimuStart));
      DYN::Trace::info(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      LOG(info, DFLEnded, context->basename(), elapsed(timeStart));
    }
    return EXIT_SUCCESS;
  } catch (DYN::Error& e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed" << std::endl;
      std::cerr << "Dynawo: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Simulation failed" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Dynawo: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  } catch (DYN::MessageError& e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed" << std::endl;
      std::cerr << "Dynawo: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Simulation failed" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Dynawo: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  } catch (std::exception& e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed" << std::endl;
      std::cerr << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Simulation failed" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  } catch (...) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed" << std::endl;
      std::cerr << "Unknown error" << std::endl;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Simulation failed" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << "Unknown error" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::dynaflowLauncherLogTag) << " ============================================================ " << DYN::Trace::endline;
    }
    return EXIT_FAILURE;
  }
}
