//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "DydDynModel.h"

#include "Constants.h"
#include "DydCommon.h"
#include "Log.h"

#include <DYDMacroConnectFactory.h>
#include <DYDMacroConnectorFactory.h>
#include <DYDMacroStaticRefFactory.h>
#include <DYDMacroStaticReferenceFactory.h>

namespace dfl {
namespace outputs {

void
DydDynModel::write(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect, const std::string& basename,
                   const inputs::DynamicDataBaseManager& dynamicDataBaseManager) {
  const auto& assemblingDoc = dynamicDataBaseManager.assemblingDocument();
  std::unordered_map<std::string, inputs::AssemblingXmlDocument::MacroConnection> macrosById;
  std::transform(assemblingDoc.macroConnections().begin(), assemblingDoc.macroConnections().end(), std::inserter(macrosById, macrosById.begin()),
                 [](const inputs::AssemblingXmlDocument::MacroConnection& macro) { return std::make_pair(macro.id, macro); });

  for (const auto& model : dynamicModelsDefinitions_.models) {
    auto blackBoxModel = helper::buildBlackBox(model.second.id, model.second.lib, basename + ".par", model.second.id);
    dynamicModelsToConnect->addModel(blackBoxModel);
    writeMacroConnector(dynamicModelsToConnect, model.second);
  }
  writeMacroConnectors(dynamicModelsToConnect, macrosById);
}

void
DydDynModel::writeMacroConnector(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
                                 const algo::DynamicModelDefinition& dynModel) {
  const auto& connections = dynModel.nodeConnections;

  // Here we compute the number of connections performed by macro connection, in order to generate the corresponding
  // index attributes
  std::unordered_map<std::string, std::tuple<unsigned int, unsigned int>> indexes;
  enum { INDEXES_NB_CONNECTIONS = 0, INDEXES_CURRENT_INDEX };
  for (const auto& connection : connections) {
    if (indexes.count(connection.id) > 0) {
      (std::get<INDEXES_NB_CONNECTIONS>(indexes.at(connection.id)))++;
    } else {
      indexes[connection.id] = std::make_tuple(1, 0);
    }
  }

  for (const auto& connection : connections) {
    auto macroConnect = dynamicdata::MacroConnectFactory::newMacroConnect(connection.id, dynModel.id, constants::networkModelName);
    macroConnect->setName2(connection.connectedElementId);
#if _DEBUG_
    assert(std::get<INDEXES_CURRENT_INDEX>(indexes.at(connection.id)) < std::get<INDEXES_NB_CONNECTIONS>(indexes.at(connection.id)));
#endif
    // We put index1 to 0 even in case there is only one connection, for consistency in the output file
    macroConnect->setIndex1(std::to_string(std::get<INDEXES_CURRENT_INDEX>(indexes.at(connection.id))));
    (std::get<INDEXES_CURRENT_INDEX>(indexes.at(connection.id)))++;
    dynamicModelsToConnect->addMacroConnect(macroConnect);
  }
}

void
DydDynModel::writeMacroConnectors(boost::shared_ptr<dynamicdata::DynamicModelsCollection>& dynamicModelsToConnect,
                                  const std::unordered_map<std::string, inputs::AssemblingXmlDocument::MacroConnection>& macrosById) {
  for (const auto& macro : dynamicModelsDefinitions_.usedMacroConnections) {
    auto macroConnector = dynamicdata::MacroConnectorFactory::newMacroConnector(macro);
    auto found = macrosById.find(macro);
#if _DEBUG_
    assert(found != macrosById.end());
#endif
    if (found == macrosById.end()) {
      // macro used in dynamic model not defined in configuration:  configuration error
      LOG(warn, DynModelMacroNotDefined, macro);
      continue;
    }

    for (const auto& connection : found->second.connections) {
      macroConnector->addConnect(connection.var1, connection.var2);
    }
    dynamicModelsToConnect->addMacroConnector(macroConnector);
  }
}

}  // namespace outputs
}  // namespace dfl
