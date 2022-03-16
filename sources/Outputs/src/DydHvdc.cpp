//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydHvdc.h"

#include "Constants.h"
#include "DydCommon.h"

#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>

namespace dfl {
namespace outputs {

const std::unordered_map<algo::HVDCDefinition::HVDCModel, std::string> DydHvdc::hvdcModelsNames_ = {
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhi, "HvdcPTanPhi"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDangling, "HvdcPTanPhiDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDanglingDiagramPQ, "HvdcPTanPhiDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPTanPhiDiagramPQ, "HvdcPTanPhiDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQProp, "HvdcPQProp"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDangling, "HvdcPQPropDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDanglingDiagramPQ, "HvdcPQPropDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQ, "HvdcPQPropDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulation, "HvdcPQPropDiagramPQEmulation"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulation, "HvdcPQPropEmulation"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPV, "HvdcPV"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDangling, "HvdcPVDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ, "HvdcPVDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQ, "HvdcPVDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulation, "HvdcPVDiagramPQEmulation"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVEmulation, "HvdcPVEmulation"),
};

void
DydHvdc::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename) {
  for (const auto& keyValue : hvdcDefinitions_.hvdcLines) {
    auto hvdcLine = keyValue.second;
    auto blackBoxModel = helper::buildBlackBoxStaticId(hvdcLine.id, hvdcLine.id, hvdcModelsNames_.at(hvdcLine.model), basename + ".par", hvdcLine.id);
    if (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
      blackBoxModel->addStaticRef("hvdc_PInj1Pu", "p2");
      blackBoxModel->addStaticRef("hvdc_QInj1Pu", "q2");
      blackBoxModel->addStaticRef("hvdc_state", "state2");
      blackBoxModel->addStaticRef("hvdc_PInj2Pu", "p1");
      blackBoxModel->addStaticRef("hvdc_QInj2Pu", "q1");
      blackBoxModel->addStaticRef("hvdc_state", "state1");
    } else {
      // terminal 1 on 1 in case both are in main connex component
      blackBoxModel->addStaticRef("hvdc_PInj1Pu", "p1");
      blackBoxModel->addStaticRef("hvdc_QInj1Pu", "q1");
      blackBoxModel->addStaticRef("hvdc_state", "state1");
      blackBoxModel->addStaticRef("hvdc_PInj2Pu", "p2");
      blackBoxModel->addStaticRef("hvdc_QInj2Pu", "q2");
      blackBoxModel->addStaticRef("hvdc_state", "state2");
    }
    dynamicModelsToConnect->addModel(blackBoxModel);
    writeConnect(dynamicModelsToConnect, hvdcLine);
  }
  for (const auto& keyValue : hvdcDefinitions_.vscBusVSCDefinitionsMap) {
    std::string id = constants::modelSignalNQprefix_ + keyValue.first;
    auto blackBoxModelVRRemote = helper::buildBlackBox(id, "VRRemote", basename + ".par", id);
    dynamicModelsToConnect->addModel(blackBoxModelVRRemote);
    dynamicModelsToConnect->addConnect(id, "vrremote_URegulated", constants::networkModelName, keyValue.first + "_U_value");
  }
}

void
DydHvdc::writeConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const algo::HVDCDefinition& hvdcLine) {
  const std::string vrremoteNqValue("vrremote_NQ");
  if (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter1BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter2BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
  } else {
    // case both : 1 <-> 1 and 2 <-> 2
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter1BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter2BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
  }
  if (hvdcLine.hasPQPropModel()) {
    const auto& busId1 = (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) ? hvdcLine.converter2BusId : hvdcLine.converter1BusId;
    dynamicModelsToConnect->addConnect(hvdcLine.id, "hvdc_NQ1", constants::modelSignalNQprefix_ + busId1, vrremoteNqValue);
    if (hvdcLine.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT) {
      dynamicModelsToConnect->addConnect(hvdcLine.id, "hvdc_NQ2", constants::modelSignalNQprefix_ + hvdcLine.converter2BusId, vrremoteNqValue);
    }
  }
}
}  // namespace outputs
}  // namespace dfl