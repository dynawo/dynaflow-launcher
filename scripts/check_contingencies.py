#!/usr/bin/python3

import argparse
import json
import os
import sys
import re
import xml.etree.ElementTree as ET

### Regexes ####################################################################
element_log_reg = re.compile("^80\s\|\s([\w-]+)\s\|\s\w+\s:\s[\w\s]+$", re.MULTILINE)

### Code #######################################################################
def check_test_contingencies(tests_path, test_name):
    results_folder = os.path.join(tests_path, 'results', test_name)
    contingencies_file = os.path.join(tests_path, 'res/contingencies_' + test_name + '.json')

    return check_contingencies(contingencies_file, results_folder)

def check_contingencies(cont_file, results_folder):
    all_ok = True

    for (id, elms) in load_contingencies(cont_file):
        dyd_file  = os.path.join(results_folder ,'TestIIDM_launch-' + id + '.dyd')
        par_file  = os.path.join(results_folder ,'TestIIDM_launch-' + id + '.par')
        log_file  = os.path.join(results_folder , id, 'outputs/timeLine/timeline.log')
        iidm_file = os.path.join(results_folder , id, 'outputs/finalState/outputIIDM.xml')

        if os.path.isdir(os.path.join(results_folder , id)):
            # This is a valid contingency, check that every file exists and
            # conforms to what we expect
            for elm in elms:
                elm_id = elm[0]
                # Let's check them ahead, so that 'and' doesn't shortcut
                c_dyd  = check_file_with(check_dyd,   "Dyd",  dyd_file, elm_id, id)
                c_par  = check_file_with(check_par,   "Par",  par_file, elm_id, id)
                c_log  = check_file_with(check_log,   "Log",  log_file, elm_id, id)
                c_iidm = check_file_with(check_iidm, "IIDM", iidm_file, elm_id, id)

                all_ok = all_ok and c_dyd and c_par and c_log and c_iidm
        else:
            # This is an invalid contingency, check that actually no file
            # related to it exists, we only need to worry about DYD and PAR files
            all_ok = (all_ok and
                    not os.path.isfile(dyd_file) and
                    not os.path.isfile(par_file)
            )

    # Returns for the OS
    if all_ok:
        return 0 # All went OK
    else:
        return 1 # We had some problems

def load_contingencies(cont_file):
    """Returns a list like: list(id, list(elements))  with one item
       per contingency"""
    with open(cont_file) as f:
        j_data = json.load(f)
        def extract_id_types(cont):
            return (cont['id'], list(map(lambda el: (el['id'], el['type']), cont['elements'])))
        return list(map(extract_id_types, j_data['contingencies']))


def check_dyd(dyd_file, elm_id):
    el = xml_find(dyd_file,
        "{http://www.rte-france.com/dynawo}blackBoxModel[@id=\"" +
        "Disconnect_" + elm_id +
        "\"]")

    return el is not None


def check_par(par_file, elm_id):
    el = xml_find(par_file,
        "{http://www.rte-france.com/dynawo}set[@id=\"" +
        "Disconnect_" + elm_id +
        "\"]")

    return el is not None

def check_log(log_file, elm_id):
    with open(log_file) as f:
        content = f.read()
        for find in element_log_reg.finditer(content):
            if find.group(1) == elm_id:
                return True
        else:
            print("Check log '" + log_file + "' failed:")
            print("  Element: " + elm_id)
            print("  Log: \n" + content)
            return False

def check_iidm(iidm_file, elm_id):
    el = xml_find(iidm_file,
        # Look for an element with our id at any level
        ".//*[@id=\"" + elm_id + "\"]")

    if el is None:
        return False

    def check_attrs(el, a_n, a_v):
        """Checks all of the attribs in a_n are in the array a_v"""
        for n in a_n:
            check =((n in el.attrib) and el.attrib[n] in a_v)
            if not check:
                return False
        return True

    def has_any_attrs(el, a_n):
        """Checks all el has any of the attributes in a_n"""
        for n in a_n:
            if n in el.attrib:
                return True
        return False

    # There are cases where they don't have p nor q (in busbar_section), cases
    # where instead of p and q, we have p1, p2, q1 and q2, and cases where
    # instead of '0' we have '-0', for some reason
    return ((not has_any_attrs(el,['p','q','p1','p2','q1','q2'])) or
            check_attrs(el, ['p','q'], ['0','-0']) or
            check_attrs(el, ['p1','p2', 'q1', 'q2'], ['0','-0'])
        )

def check_file_with(checker, name, file, elm_id, id):
    if os.path.isfile(file):
        res = checker(file, elm_id)
        if not res:
            print(name + " check failed for " + id)
        return res

    return False

def xml_find(file, path):
    tree = ET.parse(file)
    root = tree.getroot()
    return root.find(path)

def get_argparser():
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=str, help="Root directory to process")
    parser.add_argument("test", type=str, help="Root directory to process")

    return parser

### Main #######################################################################
if __name__ == '__main__':
    parser = get_argparser()
    options = parser.parse_args()

    sys.exit(check_test_contingencies(options.root, options.test))
