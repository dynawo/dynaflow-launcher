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
 * @file  OutputsConstants.cpp
 *
 * @brief Dynaflow launcher common to all writers impelmentation file
 *
 */

#include "OutputsConstants.h"

#include <algorithm>

#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace dfl {
namespace outputs {
namespace constants {

std::string
uuid(const std::string& str) {
  static boost::uuids::name_generator_sha1 gen(boost::uuids::uuid{});  // null root, change as necessary

  return boost::uuids::to_string(gen(str));
}

std::string
diagramFilename(const std::string& id) {
  // Remove '/' and '\' characters from id and replace it with '_' to avoid mistakes when parsing the path
  auto idCpy = id;
  std::replace(idCpy.begin(), idCpy.end(), '/', '_');
  std::replace(idCpy.begin(), idCpy.end(), '\\', '_');
  return idCpy + "_Diagram.txt";
}

}  // namespace constants
}  // namespace outputs
}  // namespace dfl
