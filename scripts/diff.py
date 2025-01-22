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
import zipfile
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

    parser.add_argument("root", type=str, help="Root directory to process")
    parser.add_argument("testdir", type=str, help="Test directory to process")
    parser.add_argument("config", type=str, help="Simulation configuration file")
    parser.add_argument("--verbose", "-v",
                        help="Print comparing result", action="store_true")
    parser.add_argument("--output-zip", type=str, help="zip archive to process")

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

    if options.output_zip:
        zip_archive_parent_directory = os.path.join(options.root, "resultsTestsTmp", options.testdir)
        zip_archive_path = os.path.join(zip_archive_parent_directory, "output.zip")
        unzipped_archive_path = os.path.join(zip_archive_parent_directory, "output")
        try:
            with zipfile.ZipFile(zip_archive_path, "r") as zip_ref:
                zip_ref.extractall(unzipped_archive_path)
        except FileNotFoundError:
            print("[ERROR] " + zip_archive_path + " is missing")
            nb_differences += 1

    #output IIDM
    if buildType == "Debug" or buildType == "Release" or buildType == "Coverage":

        is_constraints_generated = (buildType == "Debug" or (buildType == "Release" and "CONSTRAINTS" in chosen_outputs))
        is_lost_equipement_generated = (buildType == "Debug" or (buildType == "Release" and "LOSTEQ" in chosen_outputs))

        results_root = os.path.realpath(os.path.join(options.root, "resultsTestsTmp", options.testdir))
        reference_root = os.path.realpath(os.path.join(options.root, "reference", options.testdir))

        outputiidm_result_paths = list()
        outputiidm_result_paths.append(os.path.realpath(os.path.join(results_root, "outputs/finalState/outputIIDM.xml")))
        if options.output_zip:
            outputiidm_result_paths.append(os.path.realpath(os.path.join(results_root, "output/outputs/finalState/outputIIDM.xml")))
        outputiidm_reference_path = os.path.realpath(os.path.join(reference_root, "outputIIDM.xml"))

        if not os.path.exists(outputiidm_reference_path):
            print("Reference path " + outputiidm_reference_path + " does not exist : not checked")
        else:
            for outputiidm_result_path in outputiidm_result_paths:
                if options.verbose:
                    print("comparing " + outputiidm_result_path + " and " + outputiidm_reference_path)

                (nb_differences_local, msg) = iidmDiff.OutputIIDMCloseEnough(outputiidm_result_path, outputiidm_reference_path)
                if nb_differences_local > 0:
                    print("[ERROR] " + outputiidm_result_path + ": " + msg)
                elif options.verbose:
                    print("No difference")
                nb_differences += nb_differences_local

        # dyd and par
        nb_differences += compare_dyd_and_par_files(results_root, reference_root, options.verbose)

        if is_constraints_generated:
            #constraints
            constraints_result_paths = list()
            constraints_result_paths.append(os.path.realpath(os.path.join(results_root, "outputs/constraints/constraints.xml")))
            if options.output_zip:
                constraints_result_paths.append(os.path.realpath(os.path.join(results_root, "output/outputs/constraints/constraints.xml")))
            constraints_reference_path = os.path.realpath(os.path.join(reference_root, "constraints.xml"))

            if not os.path.exists(constraints_reference_path):
                print("Reference path " + constraints_reference_path + " does not exist : not checked")
            else:
                for constraints_result_path in constraints_result_paths:
                    if options.verbose:
                        print("comparing " + constraints_result_path + " and " + constraints_reference_path)

                    (nb_differences_local, msg) = constraintsDiff.output_constraints_close_enough(constraints_result_path, constraints_reference_path)
                    if nb_differences_local > 0:
                        print("[ERROR] constraints file " + constraints_result_path + " different from reference file " + constraints_reference_path)
                        print(msg)
                    elif options.verbose:
                        print("No difference")
                    nb_differences += nb_differences_local

        if is_lost_equipement_generated:
            #lost equipments
            lost_equipments_result_paths = list()
            lost_equipments_result_paths.append(os.path.realpath(os.path.join(results_root, "outputs/lostEquipments/lostEquipments.xml")))
            if options.output_zip:
                lost_equipments_result_paths.append(os.path.realpath(os.path.join(results_root, "output/outputs/lostEquipments/lostEquipments.xml")))
            lost_equipments_reference_path = os.path.realpath(os.path.join(reference_root, "lostEquipments.xml"))

            if not os.path.exists(lost_equipments_reference_path):
                print("Reference path " + lost_equipments_reference_path + " does not exist : not checked")
            else:
                for lost_equipments_result_path in lost_equipments_result_paths:
                    if options.verbose:
                        print("comparing " + lost_equipments_result_path + " and " + lost_equipments_reference_path)

                    # compare line per line with universal newline, stop at first diff
                    with open(lost_equipments_result_path) as f1, open(lost_equipments_reference_path) as f2:
                        identical = all(l1 == l2 for l1, l2 in zip_longest(f1, f2))
                    if identical:
                        print("No difference")
                    else:
                        print("[ERROR] lost equipments file " + lost_equipments_result_path + " different from reference file " + lost_equipments_reference_path)
                        nb_differences += 1
    else:
        raise UnknownBuildType(buildType)

    sys.exit(nb_differences)
