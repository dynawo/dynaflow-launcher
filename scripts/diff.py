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

# import iidmDiff
import os
import sys
import argparse
import json
import filecmp
import iidmDiff
import constraintsDiff

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
                print(msg)
            elif options.verbose:
                print("No difference")
            nb_differences += nb_differences_local

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

                identical = filecmp.cmp (result_path, reference_path, shallow=False)
                if identical:
                    print("No difference")
                else:
                    nb_differences += 1
    else:
        raise UnknownBuildType(buildType)

    sys.exit(nb_differences)
