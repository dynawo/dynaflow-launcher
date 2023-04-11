#!/usr/bin/python3

import argparse
import json
import os
import sys
import re
import xml.etree.ElementTree as ET
from lxml import etree

### Regexes ####################################################################
contingency_event_time = 10
timeline_log_regex = re.compile("^" + str(contingency_event_time) + "(\.0*)?\s\|\s([\w-]+)\s\|\s\w+\s[:\s]*[\w\s]+$", re.MULTILINE)

### Code #######################################################################
def check_test_contingencies(tests_path, test_name, input_iidm_name):
    results_folder = os.path.join(tests_path, 'resultsTestsTmp', test_name)
    contingencies_file = os.path.join(tests_path, 'res/contingencies_' + test_name + '.json')

    return check_contingencies(contingencies_file, results_folder, input_iidm_name)

def check_element(element, contingency_id, dyd_file, par_file, timeline_file, constraints_file, finalState_file):
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")
    c_inputs_and_timeline = False
    c_finalState = True
    (element_id, element_type) = element
    # The dynawo input files DYD, PAR and the timeline has references to the three two-winding transformers
    # that dynawo uses to model the three-winding transformers
    # But the final state contains the original element as a three-winding transformer
    if element_type == 'THREE_WINDINGS_TRANSFORMER':
        c1 = check_element_inputs_and_timeline((element_id + '_1', 'TWO_WINDINGS_TRANSFORMER'), contingency_id, dyd_file, par_file, constraints_file, timeline_file)
        c2 = check_element_inputs_and_timeline((element_id + '_2', 'TWO_WINDINGS_TRANSFORMER'), contingency_id, dyd_file, par_file, constraints_file, timeline_file)
        c3 = check_element_inputs_and_timeline((element_id + '_3', 'TWO_WINDINGS_TRANSFORMER'), contingency_id, dyd_file, par_file, constraints_file, timeline_file)
        c_inputs_and_timeline = c1 and c2 and c3
    else:
        c_inputs_and_timeline = check_element_inputs_and_timeline(element, contingency_id, dyd_file, par_file, constraints_file, timeline_file)
    if buildType == "Debug":
        c_finalState = check_file_with(check_finalState, "Final State IIDM", finalState_file, element, contingency_id)
        print("  check final state         : {}".format(c_finalState))
    return c_inputs_and_timeline and c_finalState

def check_element_inputs_and_timeline(element, contingency_id, dyd_file, par_file, constraints_file, timeline_file):
    buildType = os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE")
    c_dyd = True
    c_par = True
    c_timeline = True
    c_constraints = True
    c_lost_equipments = True
    c_dyd      = check_file_with(check_dyd,           "Dyd", dyd_file, element, contingency_id)
    print("  check dyd         : {}".format(c_dyd))
    c_par      = check_file_with(check_par,           "Par", par_file, element, contingency_id)
    print("  check par         : {}".format(c_par))
    if buildType == "Debug":
        c_timeline = check_file_with(check_timeline, "Timeline", timeline_file, element, contingency_id)
        print("  check timeline         : {}".format(c_timeline))
    print("  check lost equipments         : {}".format(c_lost_equipments))
    return c_dyd and c_par and c_timeline and c_constraints and c_lost_equipments

def check_contingencies(contingencies_file, results_folder, input_iidm_name):
    all_ok = True

    for (contingency_id, contingency_elements) in load_contingencies(contingencies_file):
        dyd_file = os.path.join(results_folder, input_iidm_name + '-' + contingency_id + '.dyd')
        par_file = os.path.join(results_folder, input_iidm_name + '-' + contingency_id + '.par')
        timeline_file = os.path.join(results_folder, 'timeLine/timeline_' + contingency_id + '.xml')
        constraints_file = os.path.join(results_folder, 'constraints/constraints_' + contingency_id + '.xml')
        finalState_file = os.path.join(results_folder, contingency_id, 'outputs/finalState/outputIIDM.xml')

        print("Checking contingency " + contingency_id)
        if os.path.isdir(os.path.join(results_folder , contingency_id)):
            # This is a valid contingency, check that every file exists and
            # conforms to what we expect
            for element in contingency_elements:
                all_ok = all_ok and check_element(element, contingency_id, dyd_file, par_file, timeline_file, constraints_file, finalState_file)
        else:
            # This is an invalid contingency, check that actually no file
            # related to it exists, we only need to worry about DYD and PAR files
            all_ok = (all_ok and
                    not os.path.isfile(dyd_file) and
                    not os.path.isfile(par_file)
            )
            print("  declared invalid, check is ok : {}".format(all_ok))

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
    timeline = etree.parse(timeline_log_file)
    timeline_root = timeline.getroot()
    timeline_namespace = timeline_root.nsmap
    timeline_prefix_root = timeline_root.prefix
    if timeline_prefix_root is None:
        timeline_prefix_root_string = ''
    else:
        timeline_prefix_root_string = timeline_prefix_root + ':'
    for event in timeline_root.findall('.//' + timeline_prefix_root_string + 'event', timeline_namespace):
        element_id_from_timeline = event.attrib['modelName']
        # For busbar sections, if we find a disconnected calculated bus it is ok
        if element_type == 'BUSBAR_SECTION' and 'calculatedBus' in element_id_from_timeline:
            return True
        # For the rest of element types we must find exactly the element identifier in the timeline log
        if element_id_from_timeline == element_id:
            return True
    else:
        print("Check timeline '" + timeline_log_file + "' failed:")
        print("  Element: " + element_id)
        print("  Log: \n" + etree.tostring(timeline_root, pretty_print = True, xml_declaration = True, encoding='UTF-8'))
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
    p123_q123 = ['p1','p2','p3','q1','q2','q3']
    attrs_to_check = {
        'LOAD': p_q,
        'GENERATOR': p_q,
        'DANGLING_LINE': p_q,
        'BRANCH': p12_q12,
        'LINE': p12_q12,
        'TWO_WINDINGS_TRANSFORMER': p12_q12,
        'THREE_WINDINGS_TRANSFORMER': p123_q123,
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
    parser.add_argument("test", type=str, help="Test name")
    parser.add_argument("iidm_name", type=str, help="IIDM input file")

    return parser

### Main #######################################################################
if __name__ == '__main__':
    parser = get_argparser()
    options = parser.parse_args()
    if os.getenv("DYNAFLOW_LAUNCHER_BUILD_TYPE") is None:
        print("[ERROR] environment variable DYNAFLOW_LAUNCHER_BUILD_TYPE needs to be defined")
        exit(1)

    sys.exit(check_test_contingencies(options.root, options.test, options.iidm_name))
