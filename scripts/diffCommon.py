import os

try:
    from itertools import zip_longest
except ImportError:
    from itertools import izip_longest as zip_longest


def compare_input_files(result_input_file_path, reference_input_file_path, file_type, verbose):
    nb_differences = 0

    if verbose:
        print("comparing " + result_input_file_path + " and " + reference_input_file_path)

    dfl_home = os.getenv("DYNAFLOW_LAUNCHER_HOME")
    with open(result_input_file_path) as result_file, open(reference_input_file_path) as reference_file:
        identical = all(result_line.replace(dfl_home, "") == reference_line for result_line, reference_line in zip_longest(result_file, reference_file))

    if (identical):
        print("No difference")
    else:
        print("[ERROR] " + file_type + " file " + result_input_file_path + " different from reference file " + reference_input_file_path)
        nb_differences += 1

    return nb_differences
