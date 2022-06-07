//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParHvdc.h"

#include "ParCommon.h"

namespace dfl {
namespace outputs {

void
ParHvdc::write(boost::shared_ptr<parameters::ParametersSetCollection>& paramSetCollection, const std::string& basename,
               const boost::filesystem::path& dirname) {
  for (const auto& hvdcLine : hvdcDefinitions_.hvdcLines) {
    paramSetCollection->addParametersSet(writeHdvcLine(hvdcLine.second, basename, dirname));
  }
  // adding parameters sets related to remote voltage control or multiple VSC regulating same bus
  for (const auto& keyValue : hvdcDefinitions_.vscBusVSCDefinitionsMap) {
    if (!paramSetCollection->hasMacroParametersSet(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr"))) {
      paramSetCollection->addMacroParameterSet(helper::buildMacroParameterSetVRRemote(helper::getMacroParameterSetId(constants::remoteVControlParId + "_vr")));
    }
    paramSetCollection->addParametersSet(helper::writeVRRemote(keyValue.first, keyValue.second.id));
  }
}

boost::shared_ptr<parameters::ParametersSet>
ParHvdc::writeHdvcLine(const algo::HVDCDefinition& hvdcDefinition, const std::string& basename, const boost::filesystem::path& dirname) {
  auto dirnameDiagram = dirname;
  dirnameDiagram.append(basename + constants::diagramDirectorySuffix);

  // Define this function as a lambda instead of a class function to avoid too much arguments that would make it less readable
  auto updateHVDCParams = [&hvdcDefinition, &dirnameDiagram](boost::shared_ptr<parameters::ParametersSet> set,
                                                             const algo::HVDCDefinition::ConverterId& converterId, unsigned int converterNumber,
                                                             unsigned int parameterNumber) {
    constexpr double factorPU = 100;
    std::size_t hashId = constants::hash(converterId);
    std::string hashIdStr = std::to_string(hashId);
    auto dirnameDiagramLocal = dirnameDiagram;
    dirnameDiagramLocal.append(constants::diagramFilename(converterId));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MinTableFile", dirnameDiagramLocal.generic_string()));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MinTableName", hashIdStr + constants::diagramMinTableSuffix));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MaxTableFile", dirnameDiagramLocal.generic_string()));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MaxTableName", hashIdStr + constants::diagramMaxTableSuffix));
    if (hvdcDefinition.converterType == algo::HVDCDefinition::ConverterType::VSC) {
      const auto& vscDefinition = (converterId == hvdcDefinition.converter1Id) ? *hvdcDefinition.vscDefinition1 : *hvdcDefinition.vscDefinition2;
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Min0Pu", (vscDefinition.qmin - 1) / factorPU));
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Max0Pu", (vscDefinition.qmax + 1) / factorPU));
    } else {
      // assuming that converterNumber is 1 or 2 (pre-condition)
      auto qMax = constants::computeQmax(hvdcDefinition.powerFactors.at(converterNumber - 1), hvdcDefinition.pMax);
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Min0Pu", (-qMax - 1) / factorPU));
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Max0Pu", (qMax + 1) / factorPU));
    }
  };

  auto set = boost::shared_ptr<parameters::ParametersSet>(new parameters::ParametersSet(hvdcDefinition.id));
  std::string first = "1";
  std::string second = "2";
  if (hvdcDefinition.position == dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    first = "2";
    second = "1";
  }
  set->addReference(helper::buildReference("hvdc_P10Pu", "p" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_Q10Pu", "q" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_U10Pu", "v" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_UPhase10", "angle" + first + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_P20Pu", "p" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_Q20Pu", "q" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_U20Pu", "v" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_UPhase20", "angle" + second + "_pu", "DOUBLE"));
  set->addReference(helper::buildReference("hvdc_PMaxPu", "pMax_pu", "DOUBLE"));
  set->addParameter(helper::buildParameter("hvdc_KLosses", 1.0));

  if (!hvdcDefinition.hasDiagramModel()) {
    set->addParameter(helper::buildParameter("hvdc_Q1MinPu", std::numeric_limits<double>::lowest()));
    set->addParameter(helper::buildParameter("hvdc_Q1MaxPu", std::numeric_limits<double>::max()));
    set->addParameter(helper::buildParameter("hvdc_Q2MinPu", std::numeric_limits<double>::lowest()));
    set->addParameter(helper::buildParameter("hvdc_Q2MaxPu", std::numeric_limits<double>::max()));
  } else {
    const auto& hvdcConverterIdMain =
        (hvdcDefinition.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcDefinition.converter2Id : hvdcDefinition.converter1Id;
    size_t converterNumber = (hvdcDefinition.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? 2 : 1;
    constexpr size_t parameterMainNumber = 1;
    updateHVDCParams(set, hvdcConverterIdMain, converterNumber, parameterMainNumber);

    if (hvdcDefinition.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
      // always converter 2 on parameter number 2 to add in that case
      updateHVDCParams(set, hvdcDefinition.converter2Id, 2, 2);
    }
  }

  if (hvdcDefinition.converterType == dfl::inputs::HvdcLine::ConverterType::VSC) {
    if (hvdcDefinition.position == dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
      set->addParameter(helper::buildParameter("hvdc_modeU10", hvdcDefinition.converter2VoltageRegulationOn.value()));
      set->addParameter(helper::buildParameter("hvdc_modeU20", hvdcDefinition.converter1VoltageRegulationOn.value()));
      set->addReference(helper::buildReference("hvdc_Q1Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_Q2Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_U1Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_U2Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter1Id));
    } else {
      set->addParameter(helper::buildParameter("hvdc_modeU10", hvdcDefinition.converter1VoltageRegulationOn.value()));
      set->addParameter(helper::buildParameter("hvdc_modeU20", hvdcDefinition.converter2VoltageRegulationOn.value()));
      set->addReference(helper::buildReference("hvdc_Q1Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_Q2Ref0Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_U1Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_U2Ref0Pu", "targetV_pu", "DOUBLE", hvdcDefinition.converter2Id));
    }
  }

  if (hvdcDefinition.converterType == dfl::inputs::HvdcLine::ConverterType::LCC) {
    if (hvdcDefinition.position == dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
      set->addReference(helper::buildReference("hvdc_CosPhi1Ref0", "powerFactor", "DOUBLE", hvdcDefinition.converter2Id));
      set->addReference(helper::buildReference("hvdc_CosPhi2Ref0", "powerFactor", "DOUBLE", hvdcDefinition.converter1Id));
    } else {
      set->addReference(helper::buildReference("hvdc_CosPhi1Ref0", "powerFactor", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_CosPhi2Ref0", "powerFactor", "DOUBLE", hvdcDefinition.converter2Id));
    }
  }

  if (hvdcDefinition.hasPQPropModel()) {
    switch (hvdcDefinition.position) {
    case dfl::algo::HVDCDefinition::Position::FIRST_IN_MAIN_COMPONENT:
      set->addReference(helper::buildReference("hvdc_QPercent1", "qMax_pu", "DOUBLE", hvdcDefinition.converter1Id));
      break;
    case dfl::algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT:
      set->addReference(helper::buildReference("hvdc_QPercent1", "qMax_pu", "DOUBLE", hvdcDefinition.converter2Id));
      break;
    case dfl::algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT:
      set->addReference(helper::buildReference("hvdc_QPercent1", "qMax_pu", "DOUBLE", hvdcDefinition.converter1Id));
      set->addReference(helper::buildReference("hvdc_QPercent2", "qMax_pu", "DOUBLE", hvdcDefinition.converter2Id));
      break;
    }
  }
  if (!hvdcDefinition.hasDanglingModel() && !hvdcDefinition.hasEmulationModel()) {
    set->addReference(helper::buildReference("P1Ref_ValueIn", "p1_pu", "DOUBLE"));
  }
  if (hvdcDefinition.hasDiagramModel()) {
    set->addParameter(helper::buildParameter("hvdc_tFilter", 0.001));
  }
  if (hvdcDefinition.hasEmulationModel()) {
    set->addParameter(helper::buildParameter("acemulation_tFilter", 50.));
    auto kac = computeKAC(*hvdcDefinition.droop);  // since the model is an emulation one, the extension is defined (see algo)
    auto pSet = hvdcDefinition.isConverter1Rectifier
                    ? computePSET(*hvdcDefinition.p0)
                    : -1.0 * computePSET(*hvdcDefinition.p0);  // since the model is an emulation one, the extension is defined (see algo)
    set->addParameter(helper::buildParameter("acemulation_KACEmulation", kac));
    set->addParameter(helper::buildParameter("acemulation_PRefSet0Pu", pSet));
  }
  return set;
}

}  // namespace outputs
}  // namespace dfl
