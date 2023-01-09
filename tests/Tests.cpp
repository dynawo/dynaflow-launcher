//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "Tests.h"

#include <DYNFileSystemUtils.h>
#include <fstream>

namespace dfl {
namespace test {

void
checkFilesEqual(const std::string& lfilepath, const std::string& rfilepath) {
  std::ofstream outFile((lfilepath + ".tmp").c_str());
  std::ifstream readFile(lfilepath.c_str());
  std::string readout;
  bool replacement_done = false;
  while (std::getline(readFile, readout)) {
    if (readout.find("resultsTestsTmp") != std::string::npos) {
      std::size_t pos = readout.find("value=\"");
      outFile << readout.substr(0, pos + 7) << readout.substr(readout.find("resultsTestsTmp"), readout.size()) << std::endl;
      replacement_done = true;
    } else {
      outFile << readout << std::endl;
    }
  }
  if (replacement_done) {
    remove(lfilepath);
    copy(lfilepath + ".tmp", lfilepath);
    remove(lfilepath + ".tmp");
  } else {
    remove(lfilepath + ".tmp");
  }

  try {
    std::ifstream liss(lfilepath, std::ifstream::binary | std::ifstream::ate);
    std::ifstream riss(rfilepath, std::ifstream::binary | std::ifstream::ate);

    ASSERT_FALSE(liss.fail() || riss.fail()) << "checkFilesEqual fails for " << lfilepath << " and " << rfilepath << " : cannot open" << std::endl;

    ASSERT_FALSE(liss.tellg() != riss.tellg()) << "checkFilesEqual fails for " << lfilepath << " and " << rfilepath << " : file sizes are different"
                                               << std::endl;

    // seek back to beginning and use std::equal to compare contents
    liss.seekg(0, std::ifstream::beg);
    riss.seekg(0, std::ifstream::beg);
    bool result = std::equal(std::istreambuf_iterator<char>(liss.rdbuf()), std::istreambuf_iterator<char>(), std::istreambuf_iterator<char>(riss.rdbuf()));
    ASSERT_TRUE(result) << "checkFilesEqual fails for " << lfilepath << " and " << rfilepath << " : files are not equal" << std::endl;
  } catch (std::exception& e) {
    ASSERT_TRUE(false) << "checkFilesEqual fails for " << lfilepath << " and " << rfilepath << " : " << e.what() << std::endl;
  }
}

}  // namespace test
}  // namespace dfl
