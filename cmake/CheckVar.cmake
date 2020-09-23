# Copyright (c) 2020, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

function(check_env_var _var)
  if (NOT DEFINED ${_var} AND NOT DEFINED ENV{${_var}})
    message(FATAL_ERROR "Variable ${_var} is not defined")
  endif()
endfunction()
