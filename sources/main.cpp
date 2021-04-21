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
#include "Dico.h"
#include "Log.h"
#include "Message.hpp"
#include "Options.h"
#include "version.h"

#include <DYNError.h>
#include <DYNInitXml.h>
#include <DYNIoDico.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <cstdlib>
#include <sstream>

static const char* dictPrefix = "DFLMessages_";

static std::string
getMandatoryEnvVar(const std::string& key) {
  char* var = getenv(key.c_str());
  if (var != NULL) {
    return std::string(var);
  } else {
    // we cannot use dictionnary errors since they may not be initialized yet
    LOG(error) << "Cannot find environnement variable " << key << " : please check runtime environement" << LOG_ENDL;
    std::exit(EXIT_FAILURE);
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
}

static inline double
elapsed(const std::chrono::steady_clock::time_point& timePoint) {
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - timePoint);
  return static_cast<double>(duration.count()) / 1000;  // To have the time in seconds as a double
}

dfl::Context::SimulationKind
getSimulationKind(const dfl::common::Options::RuntimeConfiguration& runtimeConfig) {
  if (runtimeConfig.contingenciesFilePath.empty()) {
    return dfl::Context::SimulationKind::STEADY_STATE_CALCULATION;
  }
  else {
    return dfl::Context::SimulationKind::SECURITY_ANALYSIS;
  }
}

void logContingencies(const std::string& contingenciesFilePath) {
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(contingenciesFilePath, tree);
    LOG(info) << "Contingencies" << LOG_ENDL;
    for (const boost::property_tree::ptree::value_type& v : tree.get_child("contingencies")) {
      const boost::property_tree::ptree& contingency = v.second;
      LOG(info) << contingency.get<std::string>("id") << LOG_ENDL;
      for (const boost::property_tree::ptree::value_type& e : contingency.get_child("elements")) {
        LOG(info) << "  " << e.second.get<std::string>("id") << " (" << e.second.get<std::string>("type") << ")" << LOG_ENDL;
      }
    }
}

int
main(int argc, char* argv[]) {
  try {
    auto timeStart = std::chrono::steady_clock::now();
    DYN::Trace::init();
    dfl::common::Options options;

    auto parsing_status = options.parse(argc, argv);

    if (!std::get<0>(parsing_status) || std::get<1>(parsing_status) == dfl::common::Options::Request::HELP) {
      LOG(info) << options.desc() << LOG_ENDL;
      return EXIT_SUCCESS;
    }
    if (std::get<1>(parsing_status) == dfl::common::Options::Request::VERSION) {
      LOG(info) << DYNAFLOW_LAUNCHER_VERSION_STRING << LOG_ENDL;
      return EXIT_SUCCESS;
    }

    auto& runtimeConfig = options.config();
    dfl::inputs::Configuration config(boost::filesystem::path(runtimeConfig.configPath));
    dfl::common::Log::init(options, config.outputDir().generic_string());
    LOG(info) << " ============================================================ " << LOG_ENDL;
    LOG(info) << " " << runtimeConfig.programName << " v" << DYNAFLOW_LAUNCHER_VERSION_STRING << LOG_ENDL;
    LOG(info) << " ============================================================ " << LOG_ENDL;

    auto res = boost::filesystem::path(getMandatoryEnvVar("DYNAWO_RESOURCES_DIR"));
    std::string root = getMandatoryEnvVar("DYNAFLOW_LAUNCHER_INSTALL");
    std::string locale = getMandatoryEnvVar("DYNAFLOW_LAUNCHER_LOCALE");
    boost::filesystem::path dictPath(root);
    dictPath.append("etc");
    std::string dict = dictPrefix + locale + ".dic";
    dictPath.append(dict);

    if (!boost::filesystem::is_regular_file(dictPath)) {
      // we cannot use dictionnary errors since they are not be initialized yet
      std::cerr << "Dictionary " << dictPath << " not found: check runtime environment" << std::endl;
      return EXIT_FAILURE;
    }

    dfl::common::Dico::configure(dictPath.c_str());
    initializeDynawo(locale);

    if (!boost::filesystem::exists(boost::filesystem::path(runtimeConfig.networkFilePath))) {
      LOG(error) << MESS(NetworkFileNotFound, runtimeConfig.networkFilePath) << LOG_ENDL;
      return EXIT_FAILURE;
    }
    if (!runtimeConfig.contingenciesFilePath.empty() && !boost::filesystem::exists(boost::filesystem::path(runtimeConfig.contingenciesFilePath))) {
      LOG(error) << MESS(ContingenciesFileNotFound, runtimeConfig.contingenciesFilePath) << LOG_ENDL;
      return EXIT_FAILURE;
    }
    DYN::InitXerces xerces;
    DYN::InitLibXml2 libxml2;

    dfl::Context::SimulationKind simulationKind = getSimulationKind(runtimeConfig);
    switch (simulationKind) {
      case dfl::Context::SimulationKind::STEADY_STATE_CALCULATION:
        LOG(info) << MESS(InputsSteadyStateInfo, runtimeConfig.networkFilePath, runtimeConfig.configPath) << LOG_ENDL;
        break;
      case dfl::Context::SimulationKind::SECURITY_ANALYSIS:
        LOG(info) << MESS(InputsSecurityAnalysisInfo, runtimeConfig.networkFilePath, runtimeConfig.contingenciesFilePath, runtimeConfig.configPath) << LOG_ENDL;
        logContingencies(runtimeConfig.contingenciesFilePath);
        break;
      default:
        LOG(error) << MESS(SimulationKindUnknown, "") << LOG_ENDL;
    }

    boost::filesystem::path parFilesDir(root);
    parFilesDir.append("etc");

    dfl::Context::ContextDef def{
        simulationKind, runtimeConfig.networkFilePath, config.settingFilePath(), config.assemblingFilePath(), runtimeConfig.contingenciesFilePath, runtimeConfig.dynawoLogLevel, parFilesDir, res, locale};
    dfl::Context context(def, config);

    if (!context.process()) {
      LOG(info) << MESS(InitEnd, elapsed(timeStart)) << LOG_ENDL;
      LOG(error) << MESS(ContextProcessError, context.basename()) << LOG_ENDL;
      return EXIT_FAILURE;
    }
    LOG(info) << MESS(InitEnd, elapsed(timeStart)) << LOG_ENDL;

    auto timeFilesStart = std::chrono::steady_clock::now();
    context.exportOutputs();
    LOG(info) << MESS(FilesEnd, elapsed(timeFilesStart)) << LOG_ENDL;

    auto timeSimuStart = std::chrono::steady_clock::now();
    context.execute();

    LOG(info) << MESS(SimulationEnded, context.basename(), elapsed(timeSimuStart)) << LOG_ENDL;
    LOG(info) << " ============================================================ " << LOG_ENDL;
    LOG(info) << MESS(DFLEnded, context.basename(), elapsed(timeStart)) << LOG_ENDL;
    return EXIT_SUCCESS;
  } catch (DYN::Error& e) {
    std::cerr << "Simulation failed" << std::endl;
    std::cerr << "Dynawo: " << e.what() << std::endl;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    LOG(error) << "Simulation failed" << LOG_ENDL;
    LOG(error) << "Dynawo: " << e.what() << LOG_ENDL;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    return EXIT_FAILURE;
  } catch (DYN::MessageError& e) {
    std::cerr << "Simulation failed" << std::endl;
    std::cerr << "Dynawo: " << e.what() << std::endl;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    LOG(error) << "Simulation failed" << LOG_ENDL;
    LOG(error) << "Dynawo: " << e.what() << LOG_ENDL;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    return EXIT_FAILURE;
  } catch (std::exception& e) {
    std::cerr << "Simulation failed" << std::endl;
    std::cerr << e.what() << std::endl;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    LOG(error) << "Simulation failed" << LOG_ENDL;
    LOG(error) << e.what() << LOG_ENDL;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "Simulation failed" << std::endl;
    std::cerr << "Unknown error" << std::endl;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    LOG(error) << "Simulation failed" << LOG_ENDL;
    LOG(error) << "Unknown error" << LOG_ENDL;
    LOG(error) << " ============================================================ " << LOG_ENDL;
    return EXIT_FAILURE;
  }
}
