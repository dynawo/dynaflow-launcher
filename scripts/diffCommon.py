import filecmp


def compare_input_files(result_input_file_path, reference_input_file_path, file_type, verbose, nb_differences):
    if verbose:
        print("comparing " + result_input_file_path + " and " + reference_input_file_path)

    files_are_equal = filecmp.cmp(result_input_file_path, reference_input_file_path)
    if (files_are_equal):
        print("No difference")
    else:
        print("[ERROR] " + file_type + " file " + result_input_file_path + " different from reference file " + reference_input_file_path)
        nb_differences += 1
