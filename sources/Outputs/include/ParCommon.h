//
// Copyright (c) 2021, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  ParCommon.h
 *
 * @brief Dynaflow launcher common methods for handling Parameters
 *
 */

#pragma once

#include "GeneratorDefinitionAlgorithm.h"
#include "OutputsConstants.h"

#include <DYNCommon.h>
#include <PARParameter.h>
#include <PARParameterFactory.h>
#include <PARParametersSetFactory.h>
#include <PARParametersSetCollection.h>
#include <PARParametersSetCollectionFactory.h>
#include <PARReference.h>
#include <PARReferenceFactory.h>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <string>

namespace dfl {
namespace outputs {

namespace helper {
/**
 * @brief Helper function to build a Dynawo parameter
 *
 * @param name the parameter name
 * @param value the value of the parameter
 *
 * @return Dynawo parameter with this name and value
 */
template <class T> std::unique_ptr<parameters::Parameter> buildParameter(const std::string &name, const T &value) {
  return parameters::ParameterFactory::newParameter(name, value);
}

/**
 * @brief Helper function to build a Dynawo reference
 *
 * @param name name of the reference
 * @param origName reference's origin name
 * @param type reference's type
 * @param componentId component where the reference should be found
 * @return the new reference
 */
inline std::unique_ptr<parameters::Reference> buildReference(const std::string &name, const std::string &origName, const std::string &type,
                                                               const boost::optional<std::string> &componentId = {}) {
  std::unique_ptr<parameters::Reference> ref = parameters::ReferenceFactory::newReference(name, parameters::Reference::OriginData::IIDM);
  ref->setOrigName(origName);
  ref->setType(type);
  if (componentId.is_initialized()) {
    ref->setComponentId(componentId.value());
  }

  return ref;
}

/**
 * @brief create a the Macro Parameter Set Id object
 *
 * @param modelType string to identify the macro parameter
 * @return a the Macro Parameter Set Id
 */
inline std::string getMacroParameterSetId(const std::string &modelType) { return "macro_" + modelType; }

/**
 * @brief Helper function to build a Dynawo macro parameter set for vrremote
 *
 * @param modelType string to identify the macro parameter
 * @return a Dynawo macro parameter set for vrremote
 */
inline std::unique_ptr<parameters::MacroParameterSet> buildMacroParameterSetVRRemote(const std::string &modelType) {
  std::unique_ptr<parameters::MacroParameterSet> macroParameterSet =
      std::unique_ptr<parameters::MacroParameterSet>(new parameters::MacroParameterSet(modelType));
  if (modelType == getMacroParameterSetId(constants::remoteVControlVRParId)) {
    macroParameterSet->addParameter(buildParameter("vrremote_Gain", 1.));
    macroParameterSet->addParameter(buildParameter("vrremote_tIntegral", 0.01));
    macroParameterSet->addParameter(buildParameter("vrremote_FreezingActivated", true));
  }
  return macroParameterSet;
}

/**
 * @brief Write remote voltage regulators parameter set
 *
 * @param busId the bus id to use
 * @param elementId the element id to use (generator or VSC converter)
 *
 * @returns the parameter set
 */
inline std::shared_ptr<parameters::ParametersSet> writeVRRemote(const std::string &busId, const std::string &elementId) {
  std::shared_ptr<parameters::ParametersSet> set = parameters::ParametersSetFactory::newParametersSet("Model_Signal_NQ_" + busId);
  set->addReference(buildReference("vrremote_U0Pu", "targetV_pu", "DOUBLE", elementId));
  set->addReference(buildReference("vrremote_URef0Pu", "targetV_pu", "DOUBLE", elementId));
  set->addMacroParSet(std::unique_ptr<parameters::MacroParSet>(new parameters::MacroParSet(getMacroParameterSetId(constants::remoteVControlVRParId))));
  return set;
}

/**
 * @brief Get the Generator Parameter Set Id object
 *
 * @param generator generator definition
 * @return Generator Parameter Set Id object
 */
inline static std::string getGeneratorParameterSetId(const algo::GeneratorDefinition &generator) {
  std::string id;
  bool fixedP = DYN::doubleIsZero(generator.targetP);
  switch (generator.model) {
  case algo::GeneratorDefinition::ModelType::PROP_SIGNALN_INFINITE:
    id = fixedP ? constants::propSignalNGeneratorFixedPParId : constants::propSignalNGeneratorParId;
    if (generator.isNuclear)
      id += "_Nuc";
    break;
  case algo::GeneratorDefinition::ModelType::REMOTE_SIGNALN_INFINITE:
    id = fixedP ? constants::remoteSignalNGeneratorFixedP : constants::remoteVControlParId;
    if (generator.isNuclear)
      id += "_Nuc";
    break;
  case algo::GeneratorDefinition::ModelType::SIGNALN_TFO_INFINITE:
    id = constants::signalNTfoGeneratorParId;
    if (generator.isNuclear)
      id += "_Nuc";
    break;
  case algo::GeneratorDefinition::ModelType::SIGNALN_INFINITE:
    id = fixedP ? constants::signalNGeneratorFixedPParId : constants::signalNGeneratorParId;
    if (generator.isNuclear)
      id += "_Nuc";
    break;
  default:
    id = constants::uuid(generator.id);
    break;
  }
  return id;
}

/**
 * @brief return true if the generator shares its parameter set
 *
 * @param generator generator definition
 * @return true if the generator shares its parameter set
 */
inline static bool generatorSharesParId(const algo::GeneratorDefinition &generator) {
  std::string parId = getGeneratorParameterSetId(generator);
  return parId != constants::uuid(generator.id);
}

}  // namespace helper

}  // namespace outputs
}  // namespace dfl
