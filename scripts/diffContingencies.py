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
import iidmDiff
import constraintsDiff
import filecmp


def get_argparser():
    parser = argparse.ArgumentParser()

    parser.add_argument("--verbose", "-v",
                        help="Print comparing result", action="store_true")
    parser.add_argument("root", type=str, help="Root directory to process")
    parser.add_argument("testdir", type=str, help="Test directory to process")

    return parser

def compare_file(options, contingency_folder):
    """ Will compare a reference file and a result file"""
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")

    nb_differences = 0
    #output IIDM
    if buildType == "Debug":
        result_path = full_path(
            options.root, "resultsTestsTmp", options.testdir, contingency_folder, "outputs","finalState","outputIIDM.xml")
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

    #constraints
    result_path = full_path(
        options.root, "resultsTestsTmp", options.testdir, contingency_folder, "outputs","constraints","constraints.xml")
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
            print(msg)
        elif options.verbose:
            print("No difference")
        nb_differences += nb_differences_local

    #lost equipments
    result_path = full_path(
        options.root, "resultsTestsTmp", options.testdir, contingency_folder, "outputs","lostEquipments","lostEquipments.xml")
    reference_path = full_path(
        options.root, "reference", options.testdir, contingency_folder, "lostEquipments.xml")

    if not os.path.exists(reference_path):
        if options.verbose:
            print("Reference path " + reference_path +
                    " does not exist : not checked")
    else:
        if options.verbose:
            print("comparing " + result_path + " and " + reference_path)

        identical = filecmp.cmp (result_path, reference_path, shallow=False)
        if identical:
            if options.verbose:
                print("No difference")
        else:
            print("[ERROR] lost equipments file " + result_path + " different from reference file " + reference_path)
            nb_differences += 1

    return nb_differences

def full_path(a, *paths):
    return os.path.realpath(os.path.join(a,*paths))

if __name__ == "__main__":
    parser = get_argparser()
    options = parser.parse_args()
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")
    total_diffs = 0

    results_root = full_path(options.root, "resultsTestsTmp", options.testdir)
    reference_root = full_path(options.root, "reference", options.testdir)

    print(results_root)
    for folder in os.listdir(results_root):

        if os.path.isdir(os.path.join(results_root, folder)):
            nb_differences = compare_file(options, folder)

            total_diffs += nb_differences

    reference_aggr_res = os.path.join(reference_root, "aggregatedResults.xml")
    results_aggr_res = os.path.join(results_root, "aggregatedResults.xml")
    if os.path.isfile(results_aggr_res):
        if not os.path.isfile(results_aggr_res):
            print("[ERROR] Aggregated results file " + results_aggr_res + " not found.")
            total_diffs += 1
        else:
            identical = filecmp.cmp (reference_aggr_res, results_aggr_res, shallow=False)
            if not identical:
                print("[ERROR] Found differences when comparing result and reference aggregated results file.")
                total_diffs += 1

    for folder in os.listdir(reference_root):
        if os.path.isdir(os.path.join(reference_root, folder)):
            if not os.path.isdir(os.path.join(results_root, folder)):
                print("[ERROR] Result folder" + os.path.join(results_root, folder) + " not found.")
                total_diffs += 1
            elif buildType == "Debug" and os.path.isfile(os.path.join(reference_root, folder, "outputIIDM.xml")) and \
                not os.path.isfile(os.path.join(results_root, folder, "outputs","finalState","outputIIDM.xml")):
                print("[ERROR] Result file" + os.path.join(results_root, folder, "outputs","finalState","outputIIDM.xml") + " not found.")
                total_diffs += 1
            elif os.path.isfile(os.path.join(reference_root, folder, "constraints.xml")) and \
                not os.path.isfile(os.path.join(results_root, folder, "outputs","constraints","constraints.xml")):
                print("[ERROR] Result file" + os.path.join(results_root, folder, "outputs","constraints","constraints.xml") + " not found.")
                total_diffs += 1
            elif os.path.isfile(os.path.join(reference_root, folder, "lostEquipments.xml")) and \
                not os.path.isfile(os.path.join(results_root, folder, "outputs","lostEquipments","lostEquipments.xml")):
                print("[ERROR] Result file" + os.path.join(results_root, folder, "outputs","lostEquipments","lostEquipments.xml") + " not found.")
                total_diffs += 1

    sys.exit(total_diffs)
