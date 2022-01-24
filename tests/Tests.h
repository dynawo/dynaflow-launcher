//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

static const std::string outputPathResults("resultsTestsTmp");

namespace dfl {
namespace test {
void checkFilesEqual(const std::string& lfilepath, const std::string& rfilepath);
}  // namespace test
}  // namespace dfl
