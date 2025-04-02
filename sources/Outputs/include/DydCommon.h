//
// Copyright (c) 2022, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  DydCommon.h
 *
 * @brief Dynaflow launcher common methods for handling dyd informations
 *
 */

#pragma once

#include <DYDBlackBoxModel.h>
#include <DYDBlackBoxModelFactory.h>
#include <boost/shared_ptr.hpp>

namespace dfl {
namespace outputs {

namespace helper {

/**
 * @brief Helper function to build a black box model referring to a static id
 *
 * @param modelId id of black box model
 * @param staticId reference to static id
 * @param lib black box lib name
 * @param parFile par file name
 * @param parId par id referring to
 * @return a black box model referring to a static id
 */
inline boost::shared_ptr<dynamicdata::BlackBoxModel> buildBlackBoxStaticId(const std::string &modelId, const std::string &staticId, const std::string &lib,
                                                                           const std::string &parFile, const std::string &parId) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(modelId);
  model->setStaticId(staticId);
  model->setLib(lib);
  model->setParFile(parFile);
  model->setParId(parId);
  return model;
}

/**
 * @brief Helper function to build a black box model
 *
 * @param modelId id of black box model
 * @param lib black box lib name
 * @param parFile par file name
 * @param parId par id referring to
 * @return a black box model referring to a static id
 */
inline boost::shared_ptr<dynamicdata::BlackBoxModel> buildBlackBox(const std::string &modelId, const std::string &lib, const std::string &parFile,
                                                                   const std::string &parId) {
  auto model = dynamicdata::BlackBoxModelFactory::newModel(modelId);
  model->setLib(lib);
  model->setParFile(parFile);
  model->setParId(parId);
  return model;
}

}  // namespace helper

}  // namespace outputs
}  // namespace dfl
