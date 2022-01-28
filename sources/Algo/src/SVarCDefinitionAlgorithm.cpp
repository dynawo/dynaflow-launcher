//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "SVarCDefinitionAlgorithm.h"

#include <DYNCommon.h>

namespace dfl {
namespace algo {

StaticVarCompensatorAlgorithm::StaticVarCompensatorAlgorithm(SVarCDefinitions& svarcs) : svarcs_(svarcs) {}

void
StaticVarCompensatorAlgorithm::operator()(const NodePtr& node) {
  const auto& svarcs = node->svarcs;
  for (const auto& svarc : svarcs) {
    ModelType model = ModelType::SVARCPV;
    auto hasRemoteRegulation = (svarc.connectedBusId != svarc.regulatedBusId);
    if (svarc.hasStandByAutomaton) {
      model = hasRemoteRegulation ? ModelType::SVARCPVREMOTEMODEHANDLING : ModelType::SVARCPVMODEHANDLING;
      if (svarc.hasVoltagePerReactivePowerControl && !DYN::doubleIsZero(svarc.slope)) {
        model = hasRemoteRegulation ? ModelType::SVARCPVPROPREMOTEMODEHANDLING : ModelType::SVARCPVPROPMODEHANDLING;
      }
    } else {
      model = hasRemoteRegulation ? ModelType::SVARCPVREMOTE : ModelType::SVARCPV;
      if (svarc.hasVoltagePerReactivePowerControl && !DYN::doubleIsZero(svarc.slope)) {
        model = hasRemoteRegulation ? ModelType::SVARCPVPROPREMOTE : ModelType::SVARCPVPROP;
      }
    }
    svarcs_.emplace_back(svarc.id, model, svarc.bMin, svarc.bMax, svarc.voltageSetPoint, svarc.UNom, svarc.UMinActivation, svarc.UMaxActivation,
                        svarc.USetPointMin, svarc.USetPointMax, svarc.b0, svarc.slope, svarc.UNomRemote);
  }
}

}  // namespace algo
}  // namespace dfl
