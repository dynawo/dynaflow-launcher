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
 * @file  Contingencies.cpp
 *
 * @brief Contingencies implementation file
 *
 */

#include "Contingencies.h"

#include "Log.h"

namespace dfl {
namespace inputs {

boost::optional<ContingencyElement::Type>
ContingencyElement::typeFromString(const std::string& str) {
  if (str == "LOAD") {
    return Type::LOAD;
  } else if (str == "GENERATOR") {
    return Type::GENERATOR;
  } else if (str == "BRANCH") {
    return Type::BRANCH;
  } else if (str == "LINE") {
    return Type::LINE;
  } else if (str == "TWO_WINDINGS_TRANSFORMER") {
    return Type::TWO_WINDINGS_TRANSFORMER;
  } else if (str == "THREE_WINDINGS_TRANSFORMER") {
    return Type::THREE_WINDINGS_TRANSFORMER;
  } else if (str == "SHUNT_COMPENSATOR") {
    return Type::SHUNT_COMPENSATOR;
  } else if (str == "STATIC_VAR_COMPENSATOR") {
    return Type::STATIC_VAR_COMPENSATOR;
  } else if (str == "DANGLING_LINE") {
    return Type::DANGLING_LINE;
  } else if (str == "HVDC_LINE") {
    return Type::HVDC_LINE;
  } else if (str == "BUSBAR_SECTION") {
    return Type::BUSBAR_SECTION;
  }
  return boost::none;
}

std::string
ContingencyElement::toString(Type type) {
  switch (type) {
  case Type::LOAD:
    return "LOAD";
  case Type::GENERATOR:
    return "GENERATOR";
  case Type::BRANCH:
    return "BRANCH";
  case Type::LINE:
    return "LINE";
  case Type::TWO_WINDINGS_TRANSFORMER:
    return "TWO_WINDINGS_TRANSFORMER";
  case Type::THREE_WINDINGS_TRANSFORMER:
    return "THREE_WINDINGS_TRANSFORMER";
  case Type::SHUNT_COMPENSATOR:
    return "SHUNT_COMPENSATOR";
  case Type::STATIC_VAR_COMPENSATOR:
    return "STATIC_VAR_COMPENSATOR";
  case Type::DANGLING_LINE:
    return "DANGLING_LINE";
  case Type::HVDC_LINE:
    return "HVDC_LINE";
  case Type::BUSBAR_SECTION:
    return "BUSBAR_SECTION";
  }
  // Impossible value if all enum values have been considered in the switch
  return "UNKNOWN_ELEMENT_TYPE";
}

bool
ContingencyElement::isCompatible(Type type, Type referenceType) {
  if (type == Type::BRANCH) {
    if (referenceType == Type::LINE || referenceType == Type::TWO_WINDINGS_TRANSFORMER) {
      return true;
    }
  } else if (referenceType == Type::BRANCH) {
    if (type == Type::LINE || type == Type::TWO_WINDINGS_TRANSFORMER) {
      return true;
    }
  }
  return type == referenceType;
}

}  // namespace inputs
}  // namespace dfl
