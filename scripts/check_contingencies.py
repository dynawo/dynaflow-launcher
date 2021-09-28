#!/usr/bin/python3

import argparse
import json
import os
import sys
import re
import xml.etree.ElementTree as ET

### Regexes ####################################################################
contingency_event_time = 80
timeline_log_regex = re.compile("^" + str(contingency_event_time) + "\s\|\s([\w-]+)\s\|\s\w+\s[:\s]*[\w\s]+$", re.MULTILINE)

### Code #######################################################################
def check_test_contingencies(tests_path, test_name):
    results_folder = os.path.join(tests_path, 'results', test_name)
    contingencies_file = os.path.join(tests_path, 'res/contingencies_' + test_name + '.json')

    return check_contingencies(contingencies_file, results_folder)

def check_contingencies(contingencies_file, results_folder):
    all_ok = True

    for (contingency_id, contingency_elements) in load_contingencies(contingencies_file):
        dyd_file  = os.path.join(results_folder ,'TestIIDM_launch-' + contingency_id + '.dyd')
        par_file  = os.path.join(results_folder ,'TestIIDM_launch-' + contingency_id + '.par')
        timeline_file  = os.path.join(results_folder , contingency_id, 'outputs/timeLine/timeline.log')
        finalState_file = os.path.join(results_folder , contingency_id, 'outputs/finalState/outputIIDM.xml')

        if os.path.isdir(os.path.join(results_folder , contingency_id)):
            # This is a valid contingency, check that every file exists and
            # conforms to what we expect
            for element in contingency_elements:
                # Let's check them ahead, so that 'and' doesn't shortcut
                c_dyd        = check_file_with(check_dyd,                     "Dyd",        dyd_file, element, contingency_id)
                c_par        = check_file_with(check_par,                     "Par",        par_file, element, contingency_id)
                c_timeline   = check_file_with(check_timeline,           "Timeline",   timeline_file, element, contingency_id)
                c_finalState = check_file_with(check_finalState, "Final State IIDM", finalState_file, element, contingency_id)
                all_ok = all_ok and c_dyd and c_par and c_timeline and c_finalState
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

def load_contingencies(contingencies_file):
    """Returns a list like: list(id, list(elements))  with one item
       per contingency"""
    with open(contingencies_file) as f:
        j_data = json.load(f)
        def extract_id_types(contingency):
            return (contingency['id'], list(map(lambda element: (element['id'], element['type']), contingency['elements'])))
        return list(map(extract_id_types, j_data['contingencies']))

def check_dyd(dyd_file, element):
    element_id = element[0]
    e = xml_find(dyd_file,
        "{http://www.rte-france.com/dynawo}blackBoxModel[@id=\"" +
        "Disconnect_" + element_id +
        "\"]")
    return e is not None

def check_par(par_file, element):
    element_id = element[0]
    e = xml_find(par_file,
        "{http://www.rte-france.com/dynawo}set[@id=\"" +
        "Disconnect_" + element_id +
        "\"]")
    return e is not None

def check_timeline(timeline_log_file, element):
    (element_id, element_type) = element
    with open(timeline_log_file) as f:
        content = f.read()
        print("luma timeline " + timeline_log_file);
        for find in timeline_log_regex.finditer(content):
            print("luma find.group(1) == " + find.group(1));
            # For busbar sections, if we find a disconnected calculated bus it is ok
            if element_type == 'BUSBAR_SECTION' and 'calculatedBus' in find.group(1):
                return True
            # For the rest of element types we must find exactly the element identifier in the timeline log
            if find.group(1) == element_id:
                return True
        else:
            print("Check timeline '" + timeline_log_file + "' failed:")
            print("  Element: " + element_id)
            print("  Log: \n" + content)
            return False

def check_finalState(iidm_file, element):
    """Check that in the final state output IIDM file the element of the contingency
       has no injection (p,q == 0) or flows if it is a branch (p1, q1, p2, q2 == 0)
    """
    (element_id, element_type) = element
    e = xml_find(iidm_file,
        # Look for an element with our id at any level
        ".//*[@id=\"" + element_id + "\"]")

    if e is None:
        return False

    def check_attrs(el, a_n, a_v):
        """Checks all of the attribs in a_n are in the array a_v"""
        for n in a_n:
            check =((n in el.attrib) and el.attrib[n] in a_v)
            if not check:
                print("Check fails for attr " + n + ", value = " + el.attrib[n])
                return False
        return True

    def has_any_attrs(el, a_n):
        """Checks all el has any of the attributes in a_n"""
        for n in a_n:
            if n in el.attrib:
                return True
        return False

    if element_type in ['BUSBAR_SECTION', 'HVDC_LINE']:
        return not has_any_attrs(e, ['p','q','p1','p2','q1','q2'])

    p_q = ['p', 'q']
    p12_q12 = ['p1','p2','q1','q2']
    attrs_to_check = {
        'LOAD': p_q,
        'GENERATOR': p_q,
        'DANGLING_LINE': p_q,
        'BRANCH': p12_q12,
        'LINE': p12_q12,
        'TWO_WINDINGS_TRANSFORMER': p12_q12,
        'SHUNT_COMPENSATOR': ['q'],
        'STATIC_VAR_COMPENSATOR': ['q']}
    allowed_values = ['0', '-0']
    return check_attrs(e, attrs_to_check[element_type], allowed_values)

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
