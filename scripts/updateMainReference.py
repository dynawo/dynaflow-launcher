#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2021, RTE (http://www.rte-france.com)
# See AUTHORS.txt
# All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, you can obtain one at http://mozilla.org/MPL/2.0/.
# SPDX-License-Identifier: MPL-2.0
#

import os
import iidmDiff
import shutil

# Execution requires to have iidmDiff module from dynawo library scripts
# (nrt-diff) in pythonpath. To add to python path:
# `export PYTHONPATH=$PYTHONPATH:<DYNAWO_DEPLOY>/sbin/nrt/nrt_diff`
# before calling the python script, where DYNAWO_DEPLOY the path to the
# dynawo deploy installation used by DFL

if __name__ == "__main__":
    root_dir = os.path.realpath(os.path.join(
        os.path.dirname(__file__), "../tests"))
    for root, dir, files in os.walk(root_dir):
        if "results" not in root:
            continue

        for file in files:
            if file != "outputIIDM.xml":
                continue
            filepath = os.path.join(root, file)
            root_filetest = root.split("results")[0]
            splittedPath = root.split("results")[1].split("/")
            # First element after "results" : the splitted string starts with '/'
            testname = splittedPath[1]
            filepathreference = os.path.join(
                root_filetest, "reference", testname, file)

            if len(splittedPath) and splittedPath[2] != "outputs":
                # case main SA
                filepathreference = os.path.join(
                    root_filetest, "reference", testname, splittedPath[2], file)

            if not os.path.exists(filepathreference):
                # No reference : ignore
                continue
            (nb_differences, msg) = iidmDiff.OutputIIDMCloseEnough(
                filepath, filepathreference)
            if nb_differences > 0:
                shutil.copyfile(filepath, filepathreference)
                print("Update reference output: " + filepathreference)
