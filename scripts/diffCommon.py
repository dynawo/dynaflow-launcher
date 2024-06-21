import os

try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest

from scriptsException import MissingEnvironmentVariable


def compare_dyd_and_par_files(results_root, reference_root, verbose):
    dyd_par_nb_differences = 0

    for filename in os.listdir(reference_root):
        reference_dyd_par_file_path = os.path.join(reference_root, filename)
        if os.path.isfile(reference_dyd_par_file_path) and \
                (reference_dyd_par_file_path.endswith(".dyd") or reference_dyd_par_file_path.endswith(".par")):
            result_dyd_par_filename = os.path.basename(reference_dyd_par_file_path)
            result_dyd_par_file_path = os.path.realpath(os.path.join(results_root, result_dyd_par_filename))
            dyd_par_nb_differences += compare_files(result_dyd_par_file_path, reference_dyd_par_file_path, verbose)

    return dyd_par_nb_differences


def compare_files(result_input_file_path, reference_input_file_path, verbose):
    nb_differences = 0

    if verbose:
        print("comparing " + result_input_file_path + " and " + reference_input_file_path)

    dfl_home = os.getenv("DYNAFLOW_LAUNCHER_HOME")
    if not dfl_home:
        MissingEnvironmentVariable("DYNAFLOW_LAUNCHER_HOME")
    with open(result_input_file_path) as result_file, open(reference_input_file_path) as reference_file:
        identical = all(result_line.replace(dfl_home, "") == reference_line for result_line, reference_line in zip_longest(result_file, reference_file))

    if (identical):
        print("No difference")
    else:
        print("[ERROR] file " + result_input_file_path + " different from reference file " + reference_input_file_path)
        nb_differences += 1

    return nb_differences
