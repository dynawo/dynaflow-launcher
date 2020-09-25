//
// Copyright (c) 2020, RTE (http://www.rte-france.com)
// See AUTHORS.txt
// All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, you can obtain one at http://mozilla.org/MPL/2.0/.
// SPDX-License-Identifier: MPL-2.0

#include "Context.h"
#include "Log.h"
#include "Options.h"

#include <DYNError.h>
#include <sstream>

int
main(int argc, char* argv[]) {
  try {
    DYN::Trace::init();
    dfl::common::Options options;

    if (!options.parse(argc, argv)) {
      LOG(error) << options << LOG_ENDL;
      return 0;
    }
    dfl::common::Log::init(options);

    dfl::Context context(options.config().networkFilePath);

    return 0;
  } catch (DYN::MessageError& e) {
    std::cerr << "Dynawo: " << e.what() << std::endl;
    LOG(error) << "Dynawo: " << e.what() << LOG_ENDL;
    return -1;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    LOG(error) << e.what() << LOG_ENDL;
  } catch (...) {
    std::cerr << "Unknown error" << std::endl;
    LOG(error) << "Unknown error" << LOG_ENDL;
  }
}
