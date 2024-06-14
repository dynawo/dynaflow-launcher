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
import iidmDiff
import constraintsDiff
try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest

from diffCommon import compare_input_files

from scriptsException import UnknownBuildType


def get_argparser():
    parser = argparse.ArgumentParser()

    parser.add_argument("--verbose", "-v",
                        help="Print comparing result", action="store_true")
    parser.add_argument("root", type=str, help="Root directory to process")
    parser.add_argument("testdir", type=str, help="Test directory to process")
    parser.add_argument("config", type=str, help="Simulation configuration file")
    parser.add_argument("iidm_name", type=str, help="IIDM input file")

    return parser


def compare_dyd_and_par(contingency_folder):
    nb_differences_dyd_and_par = 0

    if len(contingency_folder) > 0:
        dyd_input_filename = options.iidm_name + "-" + contingency_folder + ".dyd"
    else:
        dyd_input_filename = options.iidm_name + ".dyd"
    dyd_result_input_file_path = full_path(options.root, "resultsTestsTmp", options.testdir, dyd_input_filename)
    dyd_reference_input_file_path = full_path(options.root, "reference", options.testdir, dyd_input_filename)
    nb_differences_dyd_and_par += compare_input_files(dyd_result_input_file_path, dyd_reference_input_file_path, "dyd", options.verbose)

    if len(contingency_folder) > 0:
        par_input_filename = options.iidm_name + "-" + contingency_folder + ".par"
    else:
        par_input_filename = options.iidm_name + ".par"
    par_result_input_file_path = full_path(options.root, "resultsTestsTmp", options.testdir, par_input_filename)
    par_reference_input_file_path = full_path(options.root, "reference", options.testdir, par_input_filename)
    nb_differences_dyd_and_par += compare_input_files(par_result_input_file_path, par_reference_input_file_path, "par", options.verbose)

    return nb_differences_dyd_and_par


def compare_file(options, contingency_folder, chosen_outputs):
    """ Will compare a reference file and a result file"""
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")

    nb_differences = 0

    if buildType == "Debug" or buildType == "Release" or buildType == "Coverage":

        if buildType == "Debug" or (buildType == "Release" and "STEADYSTATE" in chosen_outputs) or buildType == "Coverage":
            # output IIDM
            result_path = full_path(
                options.root, "resultsTestsTmp", options.testdir, contingency_folder, "outputs", "finalState", "outputIIDM.xml")
            reference_path = full_path(
                options.root, "reference", options.testdir, contingency_folder, "outputIIDM.xml")

            # A reference file that does not exist is only a problem if there's a result
            # file, there are cases where the file itself should not exist
            if not os.path.exists(reference_path):
                if os.path.exists(result_path):
                    if options.verbose:
                        print("Reference path " + reference_path +
                            " does not exist but the result file does: not checked")
            else:
                if options.verbose:
                    print("comparing " + result_path + " and " + reference_path)

                (nb_differences_local, msg) = iidmDiff.OutputIIDMCloseEnough(
                    result_path, reference_path)
                if nb_differences_local > 0:
                    print("[ERROR] " + contingency_folder + ": " + msg)
                elif options.verbose:
                    print(contingency_folder + ": No difference")
                nb_differences += nb_differences_local

        # dyd and par
        if contingency_folder not in ["logs", "timeLine", "lostEquipments", "constraints", "outputs"]:
            nb_differences_dyd_and_par = compare_dyd_and_par(contingency_folder)
            nb_differences += nb_differences_dyd_and_par

        # constraints
        result_path = full_path(
            options.root, "resultsTestsTmp", options.testdir, "constraints", "constraints_" + contingency_folder + ".xml")
        reference_path = full_path(
            options.root, "reference", options.testdir, contingency_folder, "constraints.xml")

        if not os.path.exists(reference_path):
            if options.verbose:
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

        # lost equipments
        result_path = full_path(
            options.root, "resultsTestsTmp", options.testdir, "lostEquipments", "lostEquipments_" + contingency_folder + ".xml")
        reference_path = full_path(
            options.root, "reference", options.testdir, contingency_folder, "lostEquipments.xml")

        if not os.path.exists(reference_path):
            if options.verbose:
                print("Reference path " + reference_path +
                        " does not exist : not checked")
        else:
            if options.verbose:
                print("comparing " + result_path + " and " + reference_path)

            # compare line per line with universal newline, stop at first diff
            with open(result_path) as f1, open(reference_path) as f2:
                identical = all(l1 == l2 for l1, l2 in zip_longest(f1, f2))
            if identical:
                if options.verbose:
                    print("No difference")
            else:
                print("[ERROR] lost equipments file " + result_path + " different from reference file " + reference_path)
                nb_differences += 1
    else:
        raise UnknownBuildType(buildType)

    return nb_differences

def full_path(a, *paths):
    return os.path.realpath(os.path.join(a,*paths))

if __name__ == "__main__":
    parser = get_argparser()
    options = parser.parse_args()
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")
    total_diffs = 0
    config_root = full_path(options.root, options.config)

    chosen_outputs = dict()
    if buildType == "Release":
        with open(config_root, 'r') as configuration_file:
            configuration_data = json.load(configuration_file)
            try:
                chosen_outputs = configuration_data["dfl-config"]["ChosenOutputs"]
            except KeyError:
                pass

    results_root = full_path(options.root, "resultsTestsTmp", options.testdir)
    reference_root = full_path(options.root, "reference", options.testdir)

    print(results_root)
    for folder in os.listdir(results_root):
        if os.path.isdir(os.path.join(results_root, folder)):
            nb_differences = compare_file(options, folder, chosen_outputs)

            total_diffs += nb_differences

    nb_differences_dyd_par = compare_dyd_and_par("")
    total_diffs += nb_differences_dyd_par

    reference_aggr_res = os.path.join(reference_root, "aggregatedResults.xml")
    results_aggr_res = os.path.join(results_root, "aggregatedResults.xml")
    if os.path.isfile(results_aggr_res):
        if not os.path.isfile(results_aggr_res):
            print("[ERROR] Aggregated results file " + results_aggr_res + " not found.")
            total_diffs += 1
        else:
            # compare line per line with universal newline, stop at first diff
            with open(reference_aggr_res) as f1, open(results_aggr_res) as f2:
                identical = all(l1 == l2 for l1, l2 in zip_longest(f1, f2))
            if not identical:
                print("[ERROR] Found differences when comparing result and reference aggregated results file.")
                total_diffs += 1

    for folder in os.listdir(reference_root):
        if os.path.isdir(os.path.join(reference_root, folder)):
            if not os.path.isdir(os.path.join(results_root, folder)):
                print("[ERROR] Result folder " + os.path.join(results_root, folder) + " not found.")
                total_diffs += 1
            elif buildType == "Debug" and os.path.isfile(os.path.join(reference_root, folder, "outputIIDM.xml")) and \
                not os.path.isfile(os.path.join(results_root, folder, "outputs", "finalState", "outputIIDM.xml")):
                print("[ERROR] Result file " + os.path.join(results_root, folder, "outputs", "finalState", "outputIIDM.xml") + " not found.")
                total_diffs += 1
            elif os.path.isfile(os.path.join(reference_root, folder, "constraints.xml")) and \
                not os.path.isfile(os.path.join(results_root, "constraints", "constraints_" + folder + ".xml")):
                print("[ERROR] Result file " + os.path.join(results_root, "constraints", "constraints_" + folder + ".xml") + " not found.")
                total_diffs += 1
            elif os.path.isfile(os.path.join(reference_root, folder, "lostEquipments.xml")) and \
                not os.path.isfile(os.path.join(results_root, "lostEquipments", "lostEquipments_" + folder + ".xml")):
                print("[ERROR] Result file " + os.path.join(results_root, "lostEquipments", "lostEquipments_" + folder + ".xml") + " not found.")
                total_diffs += 1

    sys.exit(total_diffs)
