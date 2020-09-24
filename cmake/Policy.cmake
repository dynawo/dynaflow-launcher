# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

function(policy _policy _value)
  if (POLICY ${_policy})
    cmake_policy(SET ${_policy} ${_value})
    message(STATUS "Policy ${_policy} is set to ${_value}")
  else()
    message(WARNING "Policy ${_policy} cannot be set")
  endif()
endfunction()
