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
#include "SimulationParams.h"
#include "gitversion_dfl.h"
#include "version.h"

#include <DYNError.h>
#include <DYNFileSystemUtils.h>
#include <DYNInitXml.h>
#include <DYNIoDico.h>
#include <DYNMultiProcessingContext.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <libzip/ZipFileFactory.h>
#include <libzip/ZipOutputStream.h>

#include <chrono>
#include <cstdlib>
#include <sstream>
#include <unordered_map>


static inline std::string getMandatoryEnvVar(const std::string &key) {
  char *var = getenv(key.c_str());
  if (var != NULL) {
    return std::string(var);
  } else {
    // we cannot use dictionnary errors since they may not be initialized yet
    throw Error(EnvVariableMissing, key);
  }
}

static void initializeDynawo(const std::string &locale) {
  DYN::IoDicos &dicos = DYN::IoDicos::instance();
  dicos.addPath(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
  dicos.addDicos(getMandatoryEnvVar("DYNAWO_DICTIONARIES"), locale);
  dicos.addDico("DFLLOG", "DFLLog", locale);
  dicos.addDico("DFLERROR", "DFLError", locale);
}

static inline double elapsed(const std::chrono::steady_clock::time_point &timePoint) {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timePoint);
  return static_cast<double>(duration.count()) / 1000;  // To have the time in seconds as a double
}

static boost::shared_ptr<dfl::Context> buildContext(dfl::inputs::SimulationParams const& params,
                                                    dfl::inputs::Configuration& config,
                                                    std::unordered_map<std::string, std::string>& mapData) {
  auto timeContextStart = std::chrono::steady_clock::now();
  bool outputIsZip = !params.runtimeConfig->zipArchivePath.empty();
  dfl::Context::ContextDef def{config.getStartingPointMode(),
                               params.simulationKind,
                               params.networkFilePath,
                               config.settingFilePath(),
                               config.assemblingFilePath(),
                               params.runtimeConfig->contingenciesFilePath,
                               outputIsZip,
                               params.runtimeConfig->dynawoLogLevel,
                               params.resourcesDirPath,
                               params.locale};

  boost::shared_ptr<dfl::Context> context = boost::shared_ptr<dfl::Context>(new dfl::Context(def, config, mapData));

  if (config.getStartingPointMode() == dfl::inputs::Configuration::StartingPointMode::FLAT && context->dynamicDataBaseAssemblingContainsSVC()) {
    throw Error(NoSVCInFlatStartingPointMode);
  }

  if (config.getStartingPointMode() == dfl::inputs::Configuration::StartingPointMode::WARM) {
    if (!context->isPartiallyConditioned()) {
      throw Error(MissingICInWarmStartingPointMode);
    } else {
      if (!context->isFullyConditioned()) {
        LOG(warn, NetworkNotFullyConditioned);
      }
    }
  }
  LOG(info, StaticEnd, elapsed(timeContextStart));

  return context;
}

static void execSimulation(boost::shared_ptr<dfl::Context> context, dfl::inputs::SimulationParams const &params) {
  std::string simuName =
      params.simulationKind == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS ? "Security analysis simulation" : "Steady state simulation";
  DYNAlgorithms::multiprocessing::Context &mpiContext = DYNAlgorithms::multiprocessing::context();
  try {
    if (!context->process()) {
      LOG(info, InitEnd, elapsed(params.timeStart));
      throw Error(ContextProcessError, context->basename());
    }
    LOG(info, InitEnd, elapsed(params.timeStart));
    auto timeFilesStart = std::chrono::steady_clock::now();
    context->exportOutputs();
    DYN::Trace::resetPersistentCustomAppender(dfl::common::Log::getTag(), DYN::DEBUG);  // to force flush
    LOG(info, FilesEnd, elapsed(timeFilesStart));

    DYNAlgorithms::multiprocessing::Context::sync();

    auto timeSimuStart = std::chrono::steady_clock::now();
    if (params.simulationKind == dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS) {
      context->execute();
      context->exportResults(true);
    } else {
      // For steady-state calculation, only the root process is allowed to
      // perform the simulation.
      if (mpiContext.isRootProc()) {
        context->execute();
        context->exportResults(true);
      }
    }

    if (mpiContext.isRootProc()) {
      LOG(info, SimulationEnded, context->basename(), elapsed(timeSimuStart));
      DYN::Trace::info(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      LOG(info, DFLEnded, context->basename(), elapsed(params.timeStart));
    }
  } catch (DYN::Error &) {
    context->exportResults(false);
    throw;
  } catch (DYN::MessageError &) {
    context->exportResults(false);
    throw;
  } catch (std::exception &) {
    context->exportResults(false);
    throw;
  } catch (...) {
    context->exportResults(false);
    throw;
  }
}

void dumpLogData(std::unordered_map<std::string, std::string>& mapData,
                  boost::filesystem::path outputPath,
                  const dfl::common::Options::RuntimeConfiguration& runtimeConfig) {
  bool outputIsZip = !runtimeConfig.zipArchivePath.empty();

  if (outputIsZip) {
    const std::string programLogFileRelativePath = runtimeConfig.programName + ".log";
    const std::string programLogFileAbsolutePath = createAbsolutePath(programLogFileRelativePath, outputPath.generic_string());
    dfl::common::Log::addLogFileContentInMapData(programLogFileRelativePath, programLogFileAbsolutePath, mapData);

    boost::shared_ptr<zip::ZipFile> archive = zip::ZipFileFactory::newInstance();

    for (const std::pair<std::string, std::string>& outputFile : mapData) {
      archive->addEntry(outputFile.first, outputFile.second);
    }

    const std::string archivePath = createAbsolutePath("output.zip", outputPath.generic_string());
    zip::ZipOutputStream::write(archivePath, archive);
  }
}

int main(int argc, char *argv[]) {
  DYNAlgorithms::multiprocessing::Context mpiContext;  // Should only be used once per process in the main thread
  DYN::InitXerces xerces;
  DYN::InitLibXml2 libxml2;
  auto timeStart = std::chrono::steady_clock::now();
  DYN::Trace::init();
  dfl::common::Options options;

  const auto userRequest = options.parse(argc, argv);

  switch (userRequest) {
  case dfl::common::Options::Request::HELP:
  case dfl::common::Options::Request::ERROR:
    if (mpiContext.isRootProc()) {
      DYN::Trace::info(dfl::common::Log::getTag()) << options.desc() << DYN::Trace::endline;
    }
    return EXIT_SUCCESS;
  case dfl::common::Options::Request::VERSION:
    if (mpiContext.isRootProc()) {
      DYN::Trace::info(dfl::common::Log::getTag()) << DYNAFLOW_LAUNCHER_VERSION_STRING << " (rev:" << DYNAFLOW_LAUNCHER_GIT_BRANCH << "-"
                                                   << DYNAFLOW_LAUNCHER_GIT_HASH << ")" << DYN::Trace::endline;
    }
    return EXIT_SUCCESS;
  default:
    break;
  }

  auto &runtimeConfig = options.config();

  std::string root;
  std::string locale;
  boost::filesystem::path resourcesDir;
  boost::filesystem::path configPath(runtimeConfig.configPath);
  boost::filesystem::path outputDir;
  std::unordered_map<std::string, std::string> mapData;
  try {
    dfl::inputs::Configuration configN(configPath, dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION);

    outputDir = configN.outputDir();

    try {
      if (mpiContext.isRootProc()) {
        dfl::common::Log::init(options, outputDir.generic_string());

        // IMPORTANT: Call inputs::Configuration::sanityCheck() after the constructor to ensure configuration is correct.
        // Sanity checks are performed after log initialization so that any potential errors in the configuration can be logged.
        configN.sanityCheck();

        DYN::Trace::info(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        DYN::Trace::info(dfl::common::Log::getTag()) << " " << runtimeConfig.programName << " v" << DYNAFLOW_LAUNCHER_VERSION_STRING << DYN::Trace::endline;
        DYN::Trace::info(dfl::common::Log::getTag()) << " "
                                                     << " revision " << DYNAFLOW_LAUNCHER_GIT_BRANCH << "-" << DYNAFLOW_LAUNCHER_GIT_HASH
                                                     << DYN::Trace::endline;
        DYN::Trace::info(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      }

      resourcesDir = boost::filesystem::path(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
      root = getMandatoryEnvVar("DYNAFLOW_LAUNCHER_INSTALL");
      locale = getMandatoryEnvVar("DYNAFLOW_LAUNCHER_LOCALE");

      initializeDynawo(locale);
    } catch (DYN::Error &e) {
      if (mpiContext.isRootProc()) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " Initialization failed: " << e.what() << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      }
      dumpLogData(mapData, outputDir, runtimeConfig);
      return EXIT_FAILURE;
    } catch (DYN::MessageError &e) {
      if (mpiContext.isRootProc()) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " Initialization failed: " << e.what() << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      }
      dumpLogData(mapData, outputDir, runtimeConfig);
      return EXIT_FAILURE;
    } catch (std::exception &e) {
      if (mpiContext.isRootProc()) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " Initialization failed: " << e.what() << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      }
      dumpLogData(mapData, outputDir, runtimeConfig);
      return EXIT_FAILURE;
    } catch (...) {
      if (mpiContext.isRootProc()) {
        std::cerr << "Initialization failed" << std::endl;
        std::cerr << "Unknown error" << std::endl;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " Initialization failed" << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " Unknown error" << DYN::Trace::endline;
        DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      }
      dumpLogData(mapData, outputDir, runtimeConfig);
      return EXIT_FAILURE;
    }

    if (!boost::filesystem::exists(boost::filesystem::path(runtimeConfig.networkFilePath))) {
      throw Error(NetworkFileNotFound, runtimeConfig.networkFilePath);
    }
    if (!runtimeConfig.contingenciesFilePath.empty() && !boost::filesystem::exists(boost::filesystem::path(runtimeConfig.contingenciesFilePath))) {
      throw Error(ContingenciesFileNotFound, runtimeConfig.contingenciesFilePath);
    }
    if (userRequest == dfl::common::Options::Request::RUN_SIMULATION_NSA && runtimeConfig.contingenciesFilePath.empty()) {
      throw Error(ContingenciesFileNotFound, runtimeConfig.contingenciesFilePath);
    }

    switch (userRequest) {
    case dfl::common::Options::Request::RUN_SIMULATION_N:
      LOG(info, SteadyStateInfo, runtimeConfig.networkFilePath, runtimeConfig.configPath);
      break;
    case dfl::common::Options::Request::RUN_SIMULATION_SA:
      LOG(info, SecurityAnalysisInfo, runtimeConfig.networkFilePath, runtimeConfig.contingenciesFilePath, runtimeConfig.configPath);
      break;
    case dfl::common::Options::Request::RUN_SIMULATION_NSA:
      LOG(info, SteadyStateAndSecurityAnalysisInfo, runtimeConfig.networkFilePath, runtimeConfig.contingenciesFilePath, runtimeConfig.configPath);
      break;
    default:
      break;
    }

    boost::filesystem::path parFilesDir(root);
    parFilesDir.append("etc");

    dfl::inputs::SimulationParams params;
    params.runtimeConfig = &runtimeConfig;
    params.timeStart = timeStart;
    params.resourcesDirPath = resourcesDir;
    params.networkFilePath = runtimeConfig.networkFilePath;
    params.locale = locale;
    bool successN = true;

    if (userRequest == dfl::common::Options::Request::RUN_SIMULATION_N || userRequest == dfl::common::Options::Request::RUN_SIMULATION_NSA) {
      params.simulationKind = dfl::inputs::Configuration::SimulationKind::STEADY_STATE_CALCULATION;
      if (userRequest == dfl::common::Options::Request::RUN_SIMULATION_NSA) {
        configN.addChosenOutput(dfl::inputs::Configuration::ChosenOutputEnum::DUMPSTATE);
      }

      boost::shared_ptr<dfl::Context> context = buildContext(params, configN, mapData);
      try {
        execSimulation(context, params);
      } catch (DYN::Error &e) {
        if (mpiContext.isRootProc()) {
          std::cerr << "Simulation failed: " << e.what() << std::endl;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed: " << e.what() << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        }
        successN = false;
      } catch (DYN::MessageError &e) {
        if (mpiContext.isRootProc()) {
          std::cerr << "Simulation failed: " << e.what() << std::endl;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed: " << e.what() << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        }
        successN = false;
      } catch (std::exception &e) {
        if (mpiContext.isRootProc()) {
          std::cerr << "Simulation failed: " << e.what() << std::endl;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed: " << e.what() << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        }
        successN = false;
      } catch (...) {
        if (mpiContext.isRootProc()) {
          std::cerr << "Simulation failed" << std::endl;
          std::cerr << "Unknown error" << std::endl;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed" << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " Unknown error" << DYN::Trace::endline;
          DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
        }
        successN = false;
      }
    }

    // NSA: wait so that the steady state computation is over for everyone and the results written by root process
    DYNAlgorithms::multiprocessing::Context::sync();
    mpiContext.broadcast(successN);
    // NSA: Every process has to fail if the N ran by the root process failed
    if (!successN) {
      dumpLogData(mapData, outputDir, runtimeConfig);
      return EXIT_FAILURE;
    }

    if (userRequest == dfl::common::Options::Request::RUN_SIMULATION_SA || userRequest == dfl::common::Options::Request::RUN_SIMULATION_NSA) {
      dfl::inputs::Configuration configSA(configPath, dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS);
      // IMPORTANT: Call inputs::Configuration::sanityCheck() after the constructor to ensure configuration is correct.
      configSA.sanityCheck();
      if (configSA.getStartingPointMode() == dfl::inputs::Configuration::StartingPointMode::FLAT) {
        if (userRequest == dfl::common::Options::Request::RUN_SIMULATION_NSA)
          configSA.setStartingPointMode(dfl::inputs::Configuration::StartingPointMode::WARM);  // In NSA starting point mode for SA is forced to warm
        else
          throw Error(NoFlatStartingPointModeInSA);
      }
      params.simulationKind = dfl::inputs::Configuration::SimulationKind::SECURITY_ANALYSIS;

      if (userRequest == dfl::common::Options::Request::RUN_SIMULATION_NSA) {
        params.networkFilePath = absolute("outputs/finalState/outputIIDM.xml", outputDir.string());
        configSA.setStartingDumpFilePath(absolute("outputs/finalState/outputState.dmp", outputDir.string()));
        configSA.setStopTime(configN.getStopTime() + (configSA.getStopTime() - configSA.getStartTime()));
        configSA.setStartTime(configN.getStopTime());
        configSA.setTimeOfEvent(configN.getStopTime() + configSA.getTimeOfEvent());
      }
      boost::shared_ptr<dfl::Context> context = buildContext(params, configSA, mapData);
      execSimulation(context, params);
    }
  } catch (DYN::Error &e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
    }
    dumpLogData(mapData, outputDir, runtimeConfig);
    return EXIT_FAILURE;
  } catch (DYN::MessageError &e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
    }
    dumpLogData(mapData, outputDir, runtimeConfig);
    return EXIT_FAILURE;
  } catch (std::exception &e) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed: " << e.what() << std::endl;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed: " << e.what() << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
    }
    dumpLogData(mapData, outputDir, runtimeConfig);
    return EXIT_FAILURE;
  } catch (...) {
    if (mpiContext.isRootProc()) {
      std::cerr << "Simulation failed" << std::endl;
      std::cerr << "Unknown error" << std::endl;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " Simulation failed" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " Unknown error" << DYN::Trace::endline;
      DYN::Trace::error(dfl::common::Log::getTag()) << " ============================================================ " << DYN::Trace::endline;
    }
    dumpLogData(mapData, outputDir, runtimeConfig);
    return EXIT_FAILURE;
  }

  dumpLogData(mapData, outputDir, runtimeConfig);
  return EXIT_SUCCESS;
}
