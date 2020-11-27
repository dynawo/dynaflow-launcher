//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

/**
 * @file  HvdcLine.cpp
 *
 * @brief Hvdc line structure implementation file
 *
 */

#include "HvdcLine.h"
namespace dfl {
namespace inputs {
HvdcLine::HvdcLine(const std::string& id, const ConverterType converterType, const ConverterId& converter1_id, const BusId& converter1_busId,
                   const boost::optional<bool>& converter1_voltageRegulationOn, const ConverterId& converter2_id, const BusId& converter2_busId,
                   const boost::optional<bool>& converter2_voltageRegulationOn) :
    id{id},
    converterType{converterType},
    converter1_id{converter1_id},
    converter1_busId{converter1_busId},
    converter1_voltageRegulationOn{converter1_voltageRegulationOn},
    converter2_id{converter2_id},
    converter2_busId{converter2_busId},
    converter2_voltageRegulationOn{converter2_voltageRegulationOn} {}

bool
HvdcLine::operator==(const HvdcLine& other) const {
  return id == other.id && converterType == other.converterType && converter1_id == other.converter1_id && converter1_busId == other.converter1_busId &&
         converter2_voltageRegulationOn == other.converter2_voltageRegulationOn && converter2_id == other.converter2_id &&
         converter2_busId == other.converter2_busId && converter2_voltageRegulationOn == other.converter2_voltageRegulationOn;
}
}  // namespace inputs
}  // namespace dfl