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
HvdcLine::HvdcLine(const std::string& id,
                    const ConverterType converterType,
                    const std::shared_ptr<Converter>& converter1,
                    const std::shared_ptr<Converter>& converter2,
                    const boost::optional<ActivePowerControl>& activePowerControl,
                    double pMax,
                    bool isConverter1Rectifier,
                    const double vdcNom,
                    const double pSetPoint,
                    const double rdc,
                    const std::array<double, 2>& lossFactors) :
    id{id},
    converterType{converterType},
    converter1(converter1),
    converter2(converter2),
    activePowerControl{activePowerControl},
    pMax{pMax},
    isConverter1Rectifier{isConverter1Rectifier},
    vdcNom(vdcNom),
    pSetPoint(pSetPoint),
    rdc(rdc),
    lossFactors(lossFactors) {
  // converters are required
  assert(converter1);
  assert(converter2);
  // The converters must have the same type as the HVDC line
  switch (converterType) {
  case ConverterType::LCC:
    assert(std::dynamic_pointer_cast<LCCConverter>(converter1));
    assert(std::dynamic_pointer_cast<LCCConverter>(converter2));
    break;
  case ConverterType::VSC:
    assert(std::dynamic_pointer_cast<VSCConverter>(converter1));
    assert(std::dynamic_pointer_cast<VSCConverter>(converter2));
    break;
  default:  // impossible case by definition of the enum
    break;
  }
}

std::shared_ptr<HvdcLine>
HvdcLine::build(const std::string& id,
                const ConverterType converterType,
                const std::shared_ptr<Converter>& converter1,
                const std::shared_ptr<Converter>& converter2,
                const boost::optional<ActivePowerControl>& activePowerControl,
                double pMax,
                bool isConverter1Rectifier,
                const double vdcNom,
                const double pSetPoint,
                const double rdc,
                const std::array<double, 2>& lossFactors) {
  auto hvdcLineCreated = std::shared_ptr<HvdcLine>(new HvdcLine(id,
                                                                converterType,
                                                                converter1,
                                                                converter2,
                                                                activePowerControl,
                                                                pMax,
                                                                isConverter1Rectifier,
                                                                vdcNom,
                                                                pSetPoint,
                                                                rdc,
                                                                lossFactors));
  converter1->hvdcLine = hvdcLineCreated;
  converter2->hvdcLine = hvdcLineCreated;

  return hvdcLineCreated;
}

}  // namespace inputs
}  // namespace dfl
