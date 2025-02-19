import os
import re
import json
import argparse
import subprocess

OK_GREEN = '\033[92m'
BLUE = '\033[94m'
WARNING = '\033[93m'
FAIL = '\033[91m'
END_COLOR = '\033[0m'
BOLD = '\033[1m'

BINARY_DIR_PRESET_FIELD = 'binaryDir'
NAME_PRESET_FIELD = 'name'


def find_base_preset(config_presets_list: list[dict], preset_name: str):
    base_preset = None
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            # Actual preset found, check binary dir.
            if 'binaryDir' in config_preset:
                base_preset = preset_name
                break
            else:
                # Try to find base preset.
                base_preset = config_preset['inherits']
                break
    return base_preset


def find_binary_dir(config_presets_list: list[dict], preset_name: str):
    binary_dir = None
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            # Try to find binary dir path.
            binary_dir = config_preset.get(BINARY_DIR_PRESET_FIELD)
            break
    return binary_dir


def compute_binary_dir_from_preset_name(
    config_presets_list: list[dict], preset_name: str
):
    binary_dir_path = None
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            binary_dir_path = config_preset.get(BINARY_DIR_PRESET_FIELD)
            break

    return binary_dir_path


def check_is_preset_contained_binary_dir(
    config_presets_list: list[dict], preset_name: str
):
    is_contained_binary_dir = False
    for config_preset in config_presets_list:
        if preset_name in config_preset[NAME_PRESET_FIELD]:
            if 'binaryDir' in config_preset:
                is_contained_binary_dir = True
                break

    return is_contained_binary_dir


def compute_binary_dir(
    root_dir: str, config_presets_list: list[dict], preset_name: str
):
    binary_dir = None

    current_preset_name = preset_name

    while binary_dir is None:
        preset_name = find_base_preset(config_presets_list, preset_name)
        if (
            check_is_preset_contained_binary_dir(
                config_presets_list, preset_name
            )
            is True
        ):
            binary_dir = compute_binary_dir_from_preset_name(
                config_presets_list, preset_name
            )
            break

    if binary_dir is not None:
        binary_dir = binary_dir.replace('${sourceDir}', root_dir)
        binary_dir = binary_dir.replace('${presetName}', current_preset_name)

    return binary_dir


def is_preset_hidden(preset: dict):
    is_hidden = True
    if (preset.get('hidden') is None) or (preset.get('hidden') is False):
        is_hidden = False
    return is_hidden


def configure_preset(args: argparse.Namespace, preset_name: str):
    if preset_name is not None:
        configure_cmd = f'cmake --preset {preset_name}'
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


def build_preset(args: argparse.Namespace, binary_dir: str):
    if binary_dir is None:
        return True

    build_cmd = f'cmake --build {binary_dir}'
    if (
        subprocess.run(
            args=build_cmd, cwd=args.cmake_project_dir, shell=True
        ).returncode
        == 0
    ):
        return True
    return False


def test_preset(args: argparse.Namespace, binary_dir: str):
    test_cmd = f'ctest --test-dir {binary_dir} {args.ctest_options}'
    if (
        subprocess.run(
            args=test_cmd, cwd=args.cmake_project_dir, shell=True
        ).returncode
        == 0
    ):
        return True
    return False


def load_configure_presets(path_to_cmake_json: str = 'cpp'):
    path_to_cmake_json = f'{path_to_cmake_json}/CMakePresets.json'
    if os.path.exists(path_to_cmake_json):
        with open(path_to_cmake_json) as cmake_presets:
            return json.load(cmake_presets)['configurePresets']
    return None


def reduced_if_hidden(config_presets_lst: list):
    return [
        preset
        for preset in config_presets_lst
        if is_preset_hidden(preset) is False
    ]


def get_regex_set(config_presets_list: list[dict], regex: str | None):
    if regex:
        return {
            preset[NAME_PRESET_FIELD]
            for preset in config_presets_list
            if re.search(regex, preset[NAME_PRESET_FIELD]) is not None
        }
    elif regex == '':
        return {preset[NAME_PRESET_FIELD] for preset in config_presets_list}
    else:
        return set()


def get_resulting_include_and_build_sets(
    config_presets_list: list[dict],
    include_regex: str | None,
    build_only_regex: str | None,
    exclude_regex: str | None,
):
    if include_regex and exclude_regex:
        print(  # NOQA
            'Warning! Include and Exclude regex-s are set, you should use '
            'only one of these parameters at a time!'
        )
        return set(), set()

    include_set = get_regex_set(config_presets_list, include_regex)

    build_only_set = get_regex_set(config_presets_list, build_only_regex)

    exclude_set = get_regex_set(config_presets_list, exclude_regex)

    resulting_build_set = build_only_set - exclude_set

    resulting_include_set = include_set - exclude_set - resulting_build_set

    return resulting_include_set, resulting_build_set


def get_build_only_and_test_presets(
    args: argparse.Namespace, config_presets_list: list
):
    not_hidden_presets_list = reduced_if_hidden(config_presets_list)

    test_presets, build_only_presets = get_resulting_include_and_build_sets(
        not_hidden_presets_list, args.include, args.build_only, args.exclude
    )

    return test_presets, build_only_presets


def config_build_test(
    args: argparse.Namespace, preset_name: str, binary_dir: str
):
    if configure_preset(args, preset_name):
        if build_preset(args, binary_dir) and test_preset(args, binary_dir):
            return 0
    return 1


def config_and_build_only(
    args: argparse.Namespace, preset_name: str, binary_dir: str
):
    if configure_preset(args, preset_name) and build_preset(args, binary_dir):
        return 0
    return 1


def parse_presets(args: argparse.Namespace):
    config_presets_lst = load_configure_presets(args.cmake_project_dir)

    test_presets, build_only_presets = get_build_only_and_test_presets(
        args, config_presets_lst
    )

    return_code = 1

    for preset in build_only_presets:
        return_code = config_and_build_only(
            args,
            preset,
            compute_binary_dir(
                args.cmake_project_dir, config_presets_lst, preset
            ),
        )

        if return_code == 1:
            print(f'{FAIL}{BOLD}ERROR: See log above.{END_COLOR}')  # NOQA
            return return_code

    for preset in test_presets:
        return_code = config_build_test(
            args,
            preset,
            compute_binary_dir(
                args.cmake_project_dir, config_presets_lst, preset
            ),
        )
        if return_code == 1:
            print(f'{FAIL}{BOLD}ERROR: See log above.{END_COLOR}')  # NOQA
            return return_code

    if return_code == 0:
        print(f'{OK_GREEN}{BOLD}Tests succeed!{END_COLOR}')  # NOQA Why print
        # has been banned?
    else:
        print(f'{FAIL}{BOLD}ERROR: See log above.{END_COLOR}')  # NOQA

    return return_code


if __name__ == '__main__':
    parse = argparse.ArgumentParser(
        prog='python cmake builder',
        description='Parse cmakepresets.json, '
        'build all configs and run tests for them',
        epilog='',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parse.add_argument(
        '-e',
        '--exclude',
        metavar='',
        required=False,
        type=str,
        help='excluding names of configurePresets in CMakePresets.json, '
        "you can set this parameter with '' to select all preset names",
    )
    parse.add_argument(
        '-i',
        '--include',
        metavar='',
        required=False,
        type=str,
        help='including names of configurePresets in CMakePresets.json,'
        " you can set this parameter with '' to select all preset names",
    )
    parse.add_argument(
        '-b',
        '--build-only',
        metavar='',
        required=False,
        type=str,
        help='Presets which need to be built only without testing, you can '
        "set this parameter with '' to select all preset names",
    )

    parse.add_argument(
        '-d',
        '--cmake-project-dir',
        metavar='',
        required=True,
        type=str,
        help='set folder which contained CMakePresets.json',
    )

    parse.add_argument(
        '-t',
        '--ctest-options',
        metavar='',
        required=False,
        type=str,
        help='set folder which contained CMakePresets.json',
        default='--output-on-failure --stop-on-failure',
    )

    terminal_args = parse.parse_args()

    # Return the result of the pycmakebuilder work into terminal
    exit(parse_presets(terminal_args))  # NOQA no other way to exit from
    # program with specified result
