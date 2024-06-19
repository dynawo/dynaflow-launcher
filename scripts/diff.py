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
import sys
import argparse
import json
try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest
import iidmDiff
import constraintsDiff

from diffCommon import compare_dyd_and_par_files

from scriptsException import UnknownBuildType


def get_argparser():
    parser = argparse.ArgumentParser()

    parser.add_argument("--verbose", "-v",
                        help="Print comparing result", action="store_true")
    parser.add_argument("root", type=str, help="Root directory to process")
    parser.add_argument("testdir", type=str, help="Test directory to process")
    parser.add_argument("config", type=str, help="Simulation configuration file")

    return parser


if __name__ == "__main__":
    parser = get_argparser()
    options = parser.parse_args()
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")
    nb_differences = 0
    config_root = os.path.join(options.root, options.config)

    chosen_outputs = dict()
    if buildType == "Release":
        with open(config_root, 'r') as configuration_file:
            configuration_data = json.load(configuration_file)
            try:
                chosen_outputs = configuration_data["dfl-config"]["ChosenOutputs"]
            except KeyError:
                pass

    #output IIDM
    if buildType == "Debug" or buildType == "Release" or buildType == "Coverage":

        is_constraints_generated = (buildType == "Debug" or (buildType == "Release" and "CONSTRAINTS" in chosen_outputs))
        is_lost_equipement_generated = (buildType == "Debug" or (buildType == "Release" and "LOSTEQ" in chosen_outputs))

        result_path = os.path.realpath(os.path.join(
            options.root, "resultsTestsTmp", options.testdir, "outputs/finalState/outputIIDM.xml"))
        reference_path = os.path.realpath(os.path.join(
            options.root, "reference", options.testdir, "outputIIDM.xml"))

        if not os.path.exists(reference_path):
            print("Reference path " + reference_path +
                " does not exist : not checked")
        else:
            if options.verbose:
                print("comparing " + result_path + " and " + reference_path)

            (nb_differences_local, msg) = iidmDiff.OutputIIDMCloseEnough(
                result_path, reference_path)
            if nb_differences_local > 0:
                print("[ERROR] " + result_path + ": " + msg)
            elif options.verbose:
                print("No difference")
            nb_differences += nb_differences_local

        # dyd and par
        results_root = os.path.realpath(os.path.join(options.root, "resultsTestsTmp", options.testdir))
        reference_root = os.path.realpath(os.path.join(options.root, "reference", options.testdir))
        nb_differences += compare_dyd_and_par_files(results_root, reference_root, options.verbose)

        if is_constraints_generated:
            #constraints
            result_path = os.path.realpath(os.path.join(
                options.root, "resultsTestsTmp", options.testdir, "outputs/constraints/constraints.xml"))
            reference_path = os.path.realpath(os.path.join(
                options.root, "reference", options.testdir, "constraints.xml"))

            if not os.path.exists(reference_path):
                print("Reference path " + reference_path +
                        " does not exist : not checked")
            else:
                if options.verbose:
                    print("comparing " + result_path + " and " + reference_path)

                (nb_differences_local, msg) = constraintsDiff.output_constraints_close_enough(
                    result_path, reference_path)
                if nb_differences_local > 0:
                    print("[ERROR] constraints file " + result_path + " different from reference file " + reference_path)
                    print(msg)
                elif options.verbose:
                    print("No difference")
                nb_differences += nb_differences_local

        if is_lost_equipement_generated:
            #lost equipments
            result_path = os.path.realpath(os.path.join(
                options.root, "resultsTestsTmp", options.testdir, "outputs/lostEquipments/lostEquipments.xml"))
            reference_path = os.path.realpath(os.path.join(
                options.root, "reference", options.testdir, "lostEquipments.xml"))

            if not os.path.exists(reference_path):
                print("Reference path " + reference_path +
                        " does not exist : not checked")
            else:
                if options.verbose:
                    print("comparing " + result_path + " and " + reference_path)

                # compare line per line with universal newline, stop at first diff
                with open(result_path) as f1, open(reference_path) as f2:
                    identical = all(l1 == l2 for l1, l2 in zip_longest(f1, f2))
                if identical:
                    print("No difference")
                else:
                    print("[ERROR] lost equipments file " + result_path + " different from reference file " + reference_path)
                    nb_differences += 1
    else:
        raise UnknownBuildType(buildType)

    sys.exit(nb_differences)
