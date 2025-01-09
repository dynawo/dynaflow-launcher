//
// Copyright (c) 2025, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0
//

#include "ZipErrorMessage.h"

#include <sstream>


std::string formatZipErrorMessage(const zip::ZipException& e) {
  std::stringstream zipErrorStream;
  switch (e.getErrorCode()) {
    case zip::Error::Code::LIBARCHIVE_INTERNAL_ERROR:
      zipErrorStream << "Libarchive internal error";
      break;
    case zip::Error::Code::FILE_NOT_FOUND:
      zipErrorStream << "File " << e.what() << " not found";
      break;
    case zip::Error::Code::CANNOT_OPEN_FILE:
      zipErrorStream << "File " << e.what() << " cannot be opened";
      break;
    default:
      zipErrorStream << "Libzip unknown error";
      break;
  }
  return zipErrorStream.str();
}
