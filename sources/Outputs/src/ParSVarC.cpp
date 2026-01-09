//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ParSVarC.h"

#include "ParCommon.h"

#include <PARMacroParameterSet.h>

namespace dfl {
namespace outputs {

const std::string ParSVarC::macroParameterSetStaticCompensator_("MacroParameterSetStaticCompensator");

void ParSVarC::write(const std::unique_ptr<parameters::ParametersSetCollection> &paramSetCollection,
                     dfl::inputs::Configuration::StartingPointMode startingPointMode) {
  if (!svarcsDefinitions_.empty()) {
    paramSetCollection->addMacroParameterSet(writeMacroParameterSetStaticVarCompensators(startingPointMode));
    for (const auto &svarc : svarcsDefinitions_) {
      if (svarc.isNetwork()) {
        continue;
      }
      paramSetCollection->addParametersSet(writeStaticVarCompensator(svarc));
    }
  }
}

std::unique_ptr<parameters::MacroParameterSet>
ParSVarC::writeMacroParameterSetStaticVarCompensators(dfl::inputs::Configuration::StartingPointMode startingPointMode) {
  std::unique_ptr<parameters::MacroParameterSet> macro =
      std::unique_ptr<parameters::MacroParameterSet>(new parameters::MacroParameterSet(macroParameterSetStaticCompensator_));

  switch (startingPointMode) {
  case dfl::inputs::Configuration::StartingPointMode::WARM:
    macro->addReference(helper::buildReference("SVarC_U0Pu", "v_pu", "DOUBLE"));
    macro->addReference(helper::buildReference("SVarC_UPhase0", "angle_pu", "DOUBLE"));
    break;
  case dfl::inputs::Configuration::StartingPointMode::FLAT:
    macro->addParameter(helper::buildParameter("SVarC_U0Pu", 1.0));
    macro->addParameter(helper::buildParameter("SVarC_UPhase0", 0.));
    break;
  }

  macro->addReference(helper::buildReference("SVarC_P0Pu", "p_pu", "DOUBLE"));
  macro->addReference(helper::buildReference("SVarC_Q0Pu", "q_pu", "DOUBLE"));

  return macro;
}

std::shared_ptr<parameters::ParametersSet> ParSVarC::writeStaticVarCompensator(const algo::StaticVarCompensatorDefinition &svarc) {
  std::shared_ptr<parameters::ParametersSet> set = parameters::ParametersSetFactory::newParametersSet(constants::uuid(svarc.id));

  set->addMacroParSet(std::unique_ptr<parameters::MacroParSet>(new parameters::MacroParSet(macroParameterSetStaticCompensator_)));
  double value;

  if (svarc.isRemoteRegulation()) {
    value = svarc.voltageSetPoint / svarc.UNomRemote;
    set->addParameter(helper::buildParameter("SVarC_URef0Pu", value));
  } else {
    value = svarc.voltageSetPoint / svarc.UNom;
    set->addParameter(helper::buildParameter("SVarC_URef0Pu", value));
  }
  set->addParameter(helper::buildParameter("SVarC_UNom", svarc.UNom));
  value = computeBPU(svarc.b0, svarc.UNom);
  set->addParameter(helper::buildParameter("SVarC_BShuntPu", value));
  value = computeBPU(svarc.bMax, svarc.UNom);
  set->addParameter(helper::buildParameter("SVarC_BMaxPu", value));
  value = computeBPU(svarc.bMin, svarc.UNom);
  set->addParameter(helper::buildParameter("SVarC_BMinPu", value));
  switch (svarc.model) {
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVMODEHANDLING:
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTE:
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVREMOTEMODEHANDLING:
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROP:
    value = svarc.slope * Sb_ / svarc.UNom;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPMODEHANDLING:
    value = svarc.slope * Sb_ / svarc.UNom;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTE:
    value = svarc.slope * Sb_ / svarc.UNomRemote;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    break;
  case algo::StaticVarCompensatorDefinition::ModelType::SVARCPVPROPREMOTEMODEHANDLING:
    value = svarc.slope * Sb_ / svarc.UNomRemote;
    set->addParameter(helper::buildParameter("SVarC_LambdaPu", value));
    set->addParameter(helper::buildParameter("SVarC_UNomRemote", svarc.UNomRemote));
    set->addReference(helper::buildReference("SVarC_Mode0", "regulatingMode", "INT"));
    set->addParameter(helper::buildParameter("SVarC_URefDown", svarc.USetPointMin));
    set->addParameter(helper::buildParameter("SVarC_URefUp", svarc.USetPointMax));
    set->addParameter(helper::buildParameter("SVarC_UThresholdDown", svarc.UMinActivation));
    set->addParameter(helper::buildParameter("SVarC_UThresholdUp", svarc.UMaxActivation));
    set->addParameter(helper::buildParameter("SVarC_tThresholdDown", static_cast<double>(svarcThresholdDown_)));
    set->addParameter(helper::buildParameter("SVarC_tThresholdUp", static_cast<double>(svarcThresholdUp_)));
    break;
  default:
    break;
  }

  return set;
}

}  // namespace outputs
}  // namespace dfl
