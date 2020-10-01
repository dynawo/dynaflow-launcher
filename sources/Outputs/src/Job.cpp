//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Job.h"

#include <boost/filesystem.hpp>
#include <fstream>
#include <xml/sax/formatter/AttributeList.h>

namespace dfl {
namespace outputs {

const std::chrono::seconds Job::timeStart_{0};
const std::chrono::seconds Job::durationSimu_{30};
const std::string Job::solverFilename = "solver.par";
const std::string Job::solverName_ = "dynawo_SolverSIM";
const std::string Job::solverParId_ = "SimplifiedSolver";

Job::Job(const JobDefinition& def) : def_{def} {}

void
Job::write() {
  boost::filesystem::path path(def_.dirname);

  if (!boost::filesystem::is_directory(path)) {
    boost::filesystem::create_directories(path);
  }

  path.append(def_.filename + ".jobs");
  std::ofstream os(path.c_str());

  auto formatter = xml::sax::formatter::Formatter::createFormatter(os);
  formatter->addNamespace("dyn", "http://www.rte-france.com/dynawo");

  formatter->startDocument();
  xml::sax::formatter::AttributeList attrs;

  formatter->startElement("dyn", "jobs", attrs);

  attrs.add("name", def_.filename);
  formatter->startElement("dyn", "job", attrs);
  attrs.clear();

  writeSolver(*formatter);
  writeModeler(*formatter);
  writeSimulation(*formatter);
  writeOutputs(*formatter);

  formatter->endElement();  // job

  formatter->endElement();  // jobs
  formatter->endDocument();
}

void
Job::writeSolver(xml::sax::formatter::Formatter& formatter) {
  xml::sax::formatter::AttributeList attrs;

  attrs.add("lib", solverName_);
  attrs.add("parFile", solverFilename);
  attrs.add("parId", solverParId_);
  formatter.startElement("dyn", "solver", attrs);
  formatter.endElement();  // solver
}

void
Job::writeModeler(xml::sax::formatter::Formatter& formatter) {
  xml::sax::formatter::AttributeList attrs;
  attrs.add("compileDir", "outputs/compilation");
  formatter.startElement("dyn", "modeler", attrs);
  attrs.clear();

  attrs.add("iidmFile", def_.filename + ".iidm");
  attrs.add("parFile", "Network.par");
  attrs.add("parId", "Network");
  formatter.startElement("dyn", "network", attrs);
  attrs.clear();
  formatter.endElement();  // network

  attrs.add("dydFile", def_.filename + ".dyd");
  formatter.startElement("dyn", "dynModels", attrs);
  attrs.clear();
  formatter.endElement();  // dynModels

  attrs.add("useStandardModels", true);
  formatter.startElement("dyn", "precompiledModels", attrs);
  attrs.clear();
  formatter.endElement();  // precompiledModels

  attrs.add("useStandardModels", true);
  formatter.startElement("dyn", "modelicaModels", attrs);
  attrs.clear();
  formatter.endElement();  // modelicaModels

  formatter.endElement();  // modeler
}

void
Job::writeSimulation(xml::sax::formatter::Formatter& formatter) {
  xml::sax::formatter::AttributeList attrs;

  attrs.add("startTime", timeStart_.count());
  attrs.add("stopTime", durationSimu_.count());
  formatter.startElement("dyn", "simulation", attrs);
  formatter.endElement();  // simulation
}

void
Job::writeOutputs(xml::sax::formatter::Formatter& formatter) {
  xml::sax::formatter::AttributeList attrs;

  attrs.add("directory", "outputs");
  formatter.startElement("dyn", "outputs", attrs);

  attrs.clear();
  attrs.add("exportIIDMFile", true);
  attrs.add("exportDumpFile", false);
  formatter.startElement("dyn", "finalState", attrs);

  attrs.clear();
  formatter.startElement("dyn", "logs");

  attrs.add("tag", "");
  attrs.add("file", "dynawo.log");
  attrs.add("lvlFilter", def_.dynawoLogLevel);
  formatter.startElement("dyn", "appenders", attrs);
  attrs.clear();
  formatter.endElement();

  formatter.endElement();  // logs

  formatter.endElement();  // finalState

  formatter.endElement();  // outputs
}

}  // namespace outputs
}  // namespace dfl
