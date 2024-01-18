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

#include "DydCommon.h"
#include "OutputsConstants.h"

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
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropDiagramPQEmulationSet, "HvdcPQPropDiagramPQEmulationSet"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPQPropEmulationSet, "HvdcPQPropEmulationSet"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPV, "HvdcPV"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDangling, "HvdcPVDangling"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQ, "HvdcPVDanglingDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQ, "HvdcPVDiagramPQ"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulationSet, "HvdcPVDiagramPQEmulationSet"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSet, "HvdcPVEmulationSet"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVEmulationSetRpcl2Side1, "HvdcPVEmulationSetRpcl2Side1"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQEmulationSetRpcl2Side1, "HvdcPVDiagramPQEmulationSetRpcl2Side1"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVRpcl2Side1, "HvdcPVRpcl2Side1"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDiagramPQRpcl2Side1, "HvdcPVDiagramPQRpcl2Side1"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDanglingRpcl2Side1, "HvdcPVDanglingRpcl2Side1"),
    std::make_pair(algo::HVDCDefinition::HVDCModel::HvdcPVDanglingDiagramPQRpcl2Side1, "HvdcPVDanglingDiagramPQRpcl2Side1")};

void DydHvdc::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const std::string &basename) {
  for (const auto &keyValue : hvdcDefinitions_.hvdcLines) {
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
}

void DydHvdc::writeConnect(boost::shared_ptr<dynamicdata::DynamicModelsCollection> &dynamicModelsToConnect, const algo::HVDCDefinition &hvdcLine) {
  if (hvdcLine.position == algo::HVDCDefinition::Position::SECOND_IN_MAIN_COMPONENT) {
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter1BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter2BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter2BusId + "_switchOff", hvdcLine.id, "hvdc_switchOffSignal1Side1");
  } else {
    // case both : 1 <-> 1 and 2 <-> 2
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter1BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal1");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter2BusId + "_ACPIN", hvdcLine.id, "hvdc_terminal2");
    dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter1BusId + "_switchOff", hvdcLine.id, "hvdc_switchOffSignal1Side1");
    if (hvdcLine.position == algo::HVDCDefinition::Position::BOTH_IN_MAIN_COMPONENT)
      dynamicModelsToConnect->addConnect(constants::networkModelName, hvdcLine.converter2BusId + "_switchOff", hvdcLine.id, "hvdc_switchOffSignal1Side2");
  }
}
}  // namespace outputs
}  // namespace dfl
