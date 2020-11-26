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
 * @file  Node.cpp
 *
 * @brief Node structure implementation file
 *
 */

#include "HvdcLine.h"
namespace dfl {
namespace inputs {
HvdcLine::HvdcLine(const std::string& id, const ConverterType converterType, const inputs::ConverterInterface& converter1,
                   const inputs::ConverterInterface& converter2) :
    id{id},
    converterType{converterType},
    converter1{converter1},
    converter2{converter2} {}

}  // namespace inputs
}  // namespace dfl