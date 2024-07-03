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

#include "Constants.h"
#include "Log.h"
#include "ParCommon.h"

namespace dfl {
namespace outputs {

void ParHvdc::write(boost::shared_ptr<parameters::ParametersSetCollection> &paramSetCollection, const std::string &basename,
                    const boost::filesystem::path &dirname, dfl::inputs::Configuration::StartingPointMode startingPointMode,
                    const inputs::DynamicDataBaseManager &dynamicDataBaseManager) {
  for (const auto &hvdcLine : hvdcDefinitions_.hvdcLines) {
    paramSetCollection->addParametersSet(writeHdvcLine(hvdcLine.second, basename, dirname, startingPointMode, dynamicDataBaseManager));
  }
}

boost::shared_ptr<parameters::ParametersSet> ParHvdc::writeHdvcLine(const algo::HVDCDefinition &hvdcDefinition, const std::string &basename,
                                                                    const boost::filesystem::path &dirname,
                                                                    dfl::inputs::Configuration::StartingPointMode startingPointMode,
                                                                    const inputs::DynamicDataBaseManager &dynamicDataBaseManager) {
  auto dirnameDiagram = dirname;
  dirnameDiagram.append(basename + common::constants::diagramDirectorySuffix);

  // Define this function as a lambda instead of a class function to avoid too much arguments that would make it less readable
  auto updateHVDCParams = [&hvdcDefinition, &dirnameDiagram, &dynamicDataBaseManager](boost::shared_ptr<parameters::ParametersSet> set,
                                                                                      const algo::HVDCDefinition::ConverterId &converterId,
                                                                                      size_t converterNumber, size_t parameterNumber) {
    constexpr double factorPU = 100;
    std::string uuid = constants::uuid(converterId);
    auto dirnameDiagramLocal = dirnameDiagram;
    dirnameDiagramLocal.append(constants::diagramFilename(converterId));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MinTableFile", dirnameDiagramLocal.generic_string()));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MinTableName", uuid + constants::diagramMinTableSuffix));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MaxTableFile", dirnameDiagramLocal.generic_string()));
    set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "MaxTableName", uuid + constants::diagramMaxTableSuffix));
    if (hvdcDefinition.converterType == algo::HVDCDefinition::ConverterType::VSC) {
      const auto &vscDefinition = (converterId == hvdcDefinition.converter1Id) ? *hvdcDefinition.vscDefinition1 : *hvdcDefinition.vscDefinition2;
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Min0Pu", (vscDefinition.qmin - 1) / factorPU));
      set->addParameter(helper::buildParameter("hvdc_QInj" + std::to_string(parameterNumber) + "Max0Pu", (vscDefinition.qmax + 1) / factorPU));

      if (hvdcDefinition.hasRpcl2()) {
        const auto &databaseSetting =
            dynamicDataBaseManager.setting().getSet(dynamicDataBaseManager.assembling().getSingleAssociationFromHvdcLine(hvdcDefinition.id));
        std::string parameter = "hvdc_QNom";
        auto paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                                    [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter; });
        if (paramIt != databaseSetting.doubleParameters.end()) {
          set->addParameter(helper::buildParameter<double>("hvdc_Q" + std::to_string(parameterNumber) + "Nom", paramIt->value));
        }
        parameter = "hvdc_LambdaPu";
        paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                               [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter; });
        if (paramIt != databaseSetting.doubleParameters.end()) {
          set->addParameter(helper::buildParameter<double>("hvdc_Lambda" + std::to_string(parameterNumber) + "Pu", paramIt->value));
        }
      }

      if (!set->hasParameter("hvdc_Q" + std::to_string(parameterNumber) + "Nom"))
        set->addParameter(
            helper::buildParameter<double>("hvdc_Q" + std::to_string(parameterNumber) + "Nom", std::max(fabs(vscDefinition.qmin), fabs(vscDefinition.qmax))));
      if (!set->hasParameter("hvdc_Lambda" + std::to_string(parameterNumber) + "Pu"))
        set->addParameter(helper::buildParameter<double>("hvdc_Lambda" + std::to_string(parameterNumber) + "Pu", 0.));
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

  switch (startingPointMode) {
  case dfl::inputs::Configuration::StartingPointMode::WARM:
    set->addReference(helper::buildReference("hvdc_P10Pu", "p" + first + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_P1RefSetPu", "p" + first + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_Q10Pu", "q" + first + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_U10Pu", "v" + first + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_UPhase10", "angle" + first + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_P20Pu", "p" + second + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_Q20Pu", "q" + second + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_U20Pu", "v" + second + "_pu", "DOUBLE"));
    set->addReference(helper::buildReference("hvdc_UPhase20", "angle" + second + "_pu", "DOUBLE"));
    break;
  case dfl::inputs::Configuration::StartingPointMode::FLAT:
    set->addParameter(helper::buildParameter("hvdc_U10Pu", 1.));
    set->addParameter(helper::buildParameter("hvdc_UPhase10", 0.));
    set->addParameter(helper::buildParameter("hvdc_U20Pu", 1.));
    set->addParameter(helper::buildParameter("hvdc_UPhase20", 0.));

    const double rdc = hvdcDefinition.rdc;
    const double pSetPoint = -hvdcDefinition.pSetPoint;
    const double vdcNom = hvdcDefinition.vdcNom;
    const std::array<double, 2> &lossFactors = hvdcDefinition.lossFactors;

    const double pdcLoss = rdc * (pSetPoint / vdcNom) * (pSetPoint / vdcNom) / 100.;
    const double p0dc = pSetPoint / 100.;

    double p01 = std::numeric_limits<double>::min();
    double p02 = std::numeric_limits<double>::min();
    if (first == "1" && second == "2") {
      double factor = 1.;
      if (!hvdcDefinition.isConverter1Rectifier)
        factor = -1.;
      p01 = -factor * p0dc;
      p02 = factor * ((p0dc * (1 - lossFactors.at(0))) - pdcLoss) * (1. - lossFactors.at(1));
    } else {
      double factor = 1.;
      if (hvdcDefinition.isConverter1Rectifier)
        factor = -1.;
      p01 = factor * ((p0dc * (1 - lossFactors.at(1))) - pdcLoss) * (1. - lossFactors.at(0));
      p02 = -factor * p0dc;
    }
    set->addParameter(helper::buildParameter("hvdc_P10Pu", p01));
    set->addParameter(helper::buildParameter("hvdc_P1RefSetPu", p01));
    set->addParameter(helper::buildParameter("hvdc_P20Pu", p02));

    switch (hvdcDefinition.converterType) {
    case algo::HVDCDefinition::ConverterType::VSC:
      if (first == "1" && second == "2") {
        set->addReference(helper::buildReference("hvdc_Q10Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter1Id));
        set->addReference(helper::buildReference("hvdc_Q20Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter2Id));
      } else {
        set->addReference(helper::buildReference("hvdc_Q10Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter2Id));
        set->addReference(helper::buildReference("hvdc_Q20Pu", "targetQ_pu", "DOUBLE", hvdcDefinition.converter1Id));
      }
      break;
    case algo::HVDCDefinition::ConverterType::LCC:
      const double q01 = -fabs(hvdcDefinition.powerFactors.at(0) * p01);
      const double q02 = -fabs(hvdcDefinition.powerFactors.at(1) * p02);
      set->addParameter(helper::buildParameter("hvdc_Q10Pu", q01));
      set->addParameter(helper::buildParameter("hvdc_Q20Pu", q02));
      break;
    }
    break;
  }
  set->addReference(helper::buildReference("hvdc_PMaxPu", "pMax_pu", "DOUBLE"));
  set->addParameter(helper::buildParameter("hvdc_KLosses", 1.0));

  if (!hvdcDefinition.hasDiagramModel()) {
    set->addParameter(helper::buildParameter("hvdc_Q1MinPu", std::numeric_limits<double>::lowest()));
    set->addParameter(helper::buildParameter("hvdc_Q1MaxPu", std::numeric_limits<double>::max()));
    set->addParameter(helper::buildParameter("hvdc_Q2MinPu", std::numeric_limits<double>::lowest()));
    set->addParameter(helper::buildParameter("hvdc_Q2MaxPu", std::numeric_limits<double>::max()));
  } else {
    const auto &hvdcConverterIdMain =
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

  if (hvdcDefinition.hasRpcl2()) {
    std::vector<std::string> parameters = {"reactivePowerControlLoop_QrPu", "reactivePowerControlLoop_CqMaxPu", "reactivePowerControlLoop_DeltaURefMaxPu",
                                           "reactivePowerControlLoop_Tech", "reactivePowerControlLoop_Ti"};

    const auto &databaseSetting =
        dynamicDataBaseManager.setting().getSet(dynamicDataBaseManager.assembling().getSingleAssociationFromHvdcLine(hvdcDefinition.id));
    for (auto parameter : parameters) {
      auto paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                                  [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter; });
      if (paramIt != databaseSetting.doubleParameters.end())
        set->addParameter(helper::buildParameter(parameter, paramIt->value));
      else
        throw Error(MissingGeneratorHvdcParameterInSettings, parameter, hvdcDefinition.id);
    }

    // Try to use values from the setting ddb (for the case with no diagram, otherwise done in updateHVDCParams)
    std::string parameter = "hvdc_QNom";
    auto paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                                [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter; });
    if (paramIt != databaseSetting.doubleParameters.end()) {
      if (!set->hasParameter("hvdc_Q1Nom"))
        set->addParameter(helper::buildParameter<double>("hvdc_Q1Nom", paramIt->value));
      if (!set->hasParameter("hvdc_Q2Nom"))
        set->addParameter(helper::buildParameter<double>("hvdc_Q2Nom", paramIt->value));
    }
    parameter = "hvdc_LambdaPu";
    paramIt = std::find_if(databaseSetting.doubleParameters.begin(), databaseSetting.doubleParameters.end(),
                           [&parameter](const inputs::SettingDataBase::Parameter<double> &setting) { return setting.name == parameter; });
    if (paramIt != databaseSetting.doubleParameters.end()) {
      if (!set->hasParameter("hvdc_Lambda1Pu"))
        set->addParameter(helper::buildParameter<double>("hvdc_Lambda1Pu", paramIt->value));
      if (!set->hasParameter("hvdc_Lambda2Pu"))
        set->addParameter(helper::buildParameter<double>("hvdc_Lambda2Pu", paramIt->value));
    }
  }
  if (!hvdcDefinition.hasDiagramModel() && hvdcDefinition.converterType == dfl::inputs::HvdcLine::ConverterType::VSC) {
    // final chance: no diagram and no value in the setting ddb, put default value
    if (!set->hasParameter("hvdc_Q1Nom")) {
      set->addParameter(helper::buildParameter<double>("hvdc_Q1Nom", 100.));
    }
    if (!set->hasParameter("hvdc_Lambda1Pu"))
      set->addParameter(helper::buildParameter<double>("hvdc_Lambda1Pu", 0.));
    if (!set->hasParameter("hvdc_Q2Nom"))
      set->addParameter(helper::buildParameter<double>("hvdc_Q2Nom", 100.));
    if (!set->hasParameter("hvdc_Lambda2Pu"))
      set->addParameter(helper::buildParameter<double>("hvdc_Lambda2Pu", 0.));
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
  if (hvdcDefinition.hasEmulationModel()) {
    set->addParameter(helper::buildParameter("acemulation_tFilter", 50.));
    auto kac = computeKAC(*hvdcDefinition.droop);  // since the model is an emulation one, the extension is defined (see algo)
    auto pSet = computePSET(*hvdcDefinition.p0);   // since the model is an emulation one, the extension is defined (see algo)
    set->addParameter(helper::buildParameter("acemulation_KACEmulation", kac));
    set->addParameter(helper::buildParameter("acemulation_PRefSet0Pu", pSet));
  }
  return set;
}

}  // namespace outputs
}  // namespace dfl
