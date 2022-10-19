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
                    const double lossFactor1,
                    const double lossFactor2,
                    const boost::optional<double> powerFactor1 = boost::none,
                    const boost::optional<double> powerFactor2 = boost::none) :
    id{id},
    converterType{converterType},
    converter1(converter1),
    converter2(converter2),
    activePowerControl{activePowerControl},
    pMax{pMax},
    isConverter1Rectifier{isConverter1Rectifier},
    vdcNom_(vdcNom),
    pSetPoint_(pSetPoint),
    rdc_(rdc),
    lossFactor1_(lossFactor1),
    lossFactor2_(lossFactor2),
    powerFactor1_(powerFactor1),
    powerFactor2_(powerFactor2) {
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
                const double lossFactor1,
                const double lossFactor2,
                const boost::optional<double> powerFactor1,
                const boost::optional<double> powerFactor2) {
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
                                                                lossFactor1,
                                                                lossFactor2,
                                                                powerFactor1,
                                                                powerFactor2));
  converter1->hvdcLine = hvdcLineCreated;
  converter2->hvdcLine = hvdcLineCreated;

  switch (converterType) {
  case ConverterType::VSC:
    if (powerFactor1.is_initialized() || powerFactor2.is_initialized()) {
      throw std::runtime_error("powerFactor1 and powerFactor2 shouldn't be initialized for a VSC HVDC");
    }
    break;
  case ConverterType::LCC:
    if (!powerFactor1.is_initialized() || !powerFactor2.is_initialized()) {
      throw std::runtime_error("powerFactor1 and powerFactor2 should be initialized for a LCC HVDC");
    }
    break;
  }

  return hvdcLineCreated;
}

}  // namespace inputs
}  // namespace dfl
