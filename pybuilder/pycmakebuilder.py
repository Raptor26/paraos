import os
import json
import subprocess

OK_GREEN = "\033[92m"
BLUE = "\033[94m"
WARNING = "\033[93m"
FAIL = "\033[91m"
END_COLOR = "\033[0m"
BOLD = "\033[1m"

BINARY_DIR_PRESET_FIELD = "binaryDir"
NAME_PRESET_FIELD = "name"

def find_base_preset(config_presets_list: str, preset_name: str):
    base_preset = None
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            # Actual preset found, check binary dir.
            if "binaryDir" in config_preset:
                base_preset = preset_name
                break
            else:
                # Try find base preset
                base_preset = config_preset["inherits"]
                break
    return base_preset


def find_binary_dir(config_presets_list: str, preset_name: str):
    binary_dir = None
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            # Try find binary dir path
            binary_dir = config_preset.get(BINARY_DIR_PRESET_FIELD)
            break
    return binary_dir


def compute_binary_dir_from_preset_name(config_presets_list: str, preset_name: str):
    binary_dir_path = None
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            binary_dir_path = config_preset.get(BINARY_DIR_PRESET_FIELD)
            break

    return binary_dir_path


def check_is_preset_contained_binary_dir(config_presets_list: str, preset_name: str):
    is_contained_binary_dir = False
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            if "binaryDir" in config_preset:
                is_contained_binary_dir = True
                break

    return is_contained_binary_dir


def compute_binary_dir(root_dir: str, config_presets_list: str, preset_name: str):
    binary_dir = None

    current_preset_name = preset_name

    while binary_dir is None:
        preset_name = find_base_preset(config_presets_list, preset_name)
        if (
            check_is_preset_contained_binary_dir(config_presets_list, preset_name)
            is True
        ):
            binary_dir = compute_binary_dir_from_preset_name(
                config_presets_list, preset_name
            )
            break

    if binary_dir is not None:
        # binary_dir = binary_dir.replace("/", "\\")
        binary_dir = binary_dir.replace("${sourceDir}", root_dir)
        binary_dir = binary_dir.replace("${presetName}", current_preset_name)

    return binary_dir


def is_preset_hidden(preset: str):
    is_hidden = True
    if (preset.get("hidden") is None) or (preset.get("hidden") is False):
        is_hidden = False
    return is_hidden

def configure_preset(args: str, preset_name: str):
    if preset_name is not None:
        configure_cmd = f"cmake --preset {preset_name}"
        if (
            subprocess.run(
                args=configure_cmd, cwd=args.cmake_project_dir, shell=True
            ).returncode
            == 0
        ):
            return True
        return False

    # Nothing to configure, it's not error.
    return True


def build_preset(args: str, binary_dir: str):
    if binary_dir is None:
        return True

    build_cmd = f"cmake --build {binary_dir}"
    if (
        subprocess.run(
            args=build_cmd, cwd=args.cmake_project_dir, shell=True
        ).returncode
        == 0
    ):
        return True
    return False


def test_preset(args: str, binary_dir: str):
    test_cmd = f"ctest --test-dir {binary_dir} {args.ctest_options}"
    if (
        subprocess.run(
            args=test_cmd, cwd=args.cmake_project_dir, shell=True
        ).returncode
        == 0
    ):
        return True
    return False


def load_configure_presets(path_to_cmake_json: str = "cpp"):
    path_to_cmake_json = f"{path_to_cmake_json}/CMakePresets.json"
    if os.path.exists(path_to_cmake_json):
        with open(path_to_cmake_json) as cmake_presets:
            return json.load(cmake_presets)["configurePresets"]
    return None


def compute_actual_configure_presets_subset(config_presets_lst: list, args: str):
    configure_name_set = set()

    for config_preset in config_presets_lst:
        configure_name_set.add(config_preset.get(NAME_PRESET_FIELD))

    excluded = set()
    if args.exclude is not None:
        excluded = args.exclude.split(" ")
        
    excluded_set = set()

    for excluded_name in excluded:
        excluded_set.add(excluded_name)

    configure_name_set = configure_name_set.difference(excluded_set)
    return configure_name_set


def reduced_if_hidden(config_presets_lst: list, configure_name_set: set):
    for config_preset in config_presets_lst:
        name = config_preset.get(NAME_PRESET_FIELD)
        if name in configure_name_set:
            if config_preset.get("hidden") is True:
                configure_name_set.remove(name)
    return configure_name_set


def reduced_configure_dictionary(config_presets_lst: list, configure_name_set: set):
    reduced_configure_lst = list()
    for idx, config_preset in enumerate(config_presets_lst):
        name = config_preset.get(NAME_PRESET_FIELD)
        if name in configure_name_set:
            reduced_configure_lst.append(config_preset)
    return reduced_configure_lst

def compute_configure_subset_lst(args: str, config_presets_list: list):
    actual_configure_subset = compute_actual_configure_presets_subset(
        config_presets_list, args
    )

    reduced_configure_presets_lst = reduced_if_hidden(
        config_presets_list, actual_configure_subset
    )

    reduced_configure_presets_lst = reduced_configure_dictionary(
        config_presets_list, actual_configure_subset
    )
    
    return reduced_configure_presets_lst

def parse_presets(args: str):
    config_presets_lst = load_configure_presets(args.cmake_project_dir)
    reduced_config_presets_lst = compute_configure_subset_lst(args, config_presets_lst)

    return_code = 0
    if reduced_config_presets_lst is None:
        return 1
    for config_preset in reduced_config_presets_lst:
        name = config_preset.get(NAME_PRESET_FIELD)
        if name is None:
            continue

        if configure_preset(args, name):
            binary_dir = compute_binary_dir(
                args.cmake_project_dir, config_presets_lst, name
            )
            if build_preset(args, binary_dir):
                if not test_preset(args, binary_dir):
                    return_code += 1
                    break
            else:
                return_code += 1
                break
        else:
            return_code += 1
            break

    if return_code == 0:
        print(f"{OK_GREEN}{BOLD}Tests succeed!{END_COLOR}")
    else:
        print(f"{FAIL}{BOLD}ERROR: See log above.{END_COLOR}")

    return return_code


if __name__ == "__main__":
    import argparse

    parse = argparse.ArgumentParser(
        prog="python cmake builder",
        description="Parse cmakepresets.json, build all configs and run tests for them",
        epilog="",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parse.add_argument(
        "-e",
        "--exclude",
        metavar="",
        required=False,
        type=str,
        help="excluding names of configurePresets in CMakePresets.json",
    )

    parse.add_argument(
        "-d",
        "--cmake-project-dir",
        metavar="",
        required=True,
        type=str,
        help="set folder which contained CMakePresets.json",
    )

    parse.add_argument(
        "-t",
        "--ctest-options",
        metavar="",
        required=False,
        type=str,
        help="set folder which contained CMakePresets.json",
        default="--output-on-failure --stop-on-failure",
    )

    args = parse.parse_args()

    parse_presets(args)
