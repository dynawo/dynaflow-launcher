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
 * @file  ParCommon.hpp
 *
 * @brief Dynaflow launcher common methods for handling Parameters
 *
 */

#pragma once

#include <PARParameter.h>
#include <PARParameterFactory.h>
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
 */
template<class T>
boost::shared_ptr<parameters::Parameter>
buildParameter(const std::string& name, const T& value) {
  return parameters::ParameterFactory::newParameter(name, value);
}
}  // namespace helper

}  // namespace outputs
}  // namespace dfl
