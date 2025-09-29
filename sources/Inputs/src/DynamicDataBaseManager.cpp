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
 * @file  DynamicDataBaseManager.cpp
 *
 * @brief Automaton manager implementation file
 *
 */

#include "DynamicDataBaseManager.h"

namespace dfl {
namespace inputs {

DynamicDataBaseManager::DynamicDataBaseManager(const std::vector<boost::filesystem::path> & settingFilePaths,
                                               const std::vector<boost::filesystem::path> & assemblingFilePaths) :
  assembling_(assemblingFilePaths),
  setting_(settingFilePaths) {}
}  // namespace inputs

}  // namespace dfl
