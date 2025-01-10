import os
import json
import subprocess

OK_GREEN = '\033[92m'
BLUE = '\033[94m'
WARNING = '\033[93m'
FAIL = '\033[91m'
END_COLOR = '\033[0m'
BOLD = '\033[1m'

def find_base_preset(config_presets_list: str, preset_name: str):
    base_preset = None
    for config_preset in config_presets_list:
        if preset_name in config_preset["name"]:
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
        if preset_name in config_preset["name"]:
            # Try find binary dir path
            if "binaryDir" in config_preset:
                # binary directory founded!
                binary_dir = config_preset["binaryDir"]
                break
    return binary_dir

def compute_binary_dir_from_preset_name(
    config_presets_list : str, 
    preset_name: str):
    binary_dir_path = None
    for config_preset in config_presets_list:
        if preset_name in config_preset["name"]:
            if "binaryDir" in config_preset:
                binary_dir_path = config_preset["binaryDir"]
                break

    return binary_dir_path
    
def check_is_preset_contained_binary_dir(
    config_presets_list : str, 
    preset_name: str):
    is_contained_binary_dir = False
    for config_preset in config_presets_list:
        if preset_name in config_preset["name"]:
            if "binaryDir" in config_preset:
                is_contained_binary_dir = True
                break

    return is_contained_binary_dir

def compute_binary_dir_v2(
    root_dir : str,
    config_presets_list : str, 
    preset_name: str):
    binary_dir = None

    current_preset_name = preset_name

    while binary_dir is None:
        preset_name = find_base_preset(config_presets_list, preset_name)
        if check_is_preset_contained_binary_dir(config_presets_list, preset_name) is True:
            binary_dir = compute_binary_dir_from_preset_name(
                config_presets_list, preset_name)
            break

    if binary_dir is not None:
        binary_dir = binary_dir.replace("/", "\\")
        binary_dir = binary_dir.replace("${sourceDir}", root_dir)
        binary_dir = binary_dir.replace("${presetName}", current_preset_name)
        
    return binary_dir

def is_preset_hidden(preset: str):
    is_hidden = True
    if (preset.get("hidden") is None) or (preset.get("hidden") is False):
        is_hidden = False
    return is_hidden

def if_need_build_preset(preset: str, exclude: str = ""):
    preset_name = preset["name"]
    
    if preset_name in exclude:
        preset_name = None
    
    if preset_name is not None:
        if is_preset_hidden(preset):
            preset_name = None

    return preset_name

def configure_preset(args: str, preset_name: str):
    if preset_name is not None:
        configure_cmd = f"cmake --preset {preset_name}"
        if subprocess.run(args = configure_cmd, cwd = args.cmake_project_dir, shell = False).returncode == 0:
            return True
        return False
    
    # Nothing to configure, it's not error.
    return True

def build_preset(args: str, binary_dir: str):
    build_cmd = f"cmake --build {binary_dir}"
    if subprocess.run(args = build_cmd, cwd = args.cmake_project_dir, shell = False).returncode == 0:
        return True
    return False

def test_preset(args: str, 
                binary_dir: str):
    test_cmd = f"ctest --test-dir {binary_dir} {args.ctest_options}"
    if subprocess.run(args = test_cmd, cwd = args.cmake_project_dir, shell = False).returncode == 0:
        return True
    return False

def load_configure_presets(path_to_cmake_json: str = "cpp"):
    path_to_cmake_json = path_to_cmake_json + "\\CMakePresets.json"
    if os.path.exists(path_to_cmake_json):
        with open(path_to_cmake_json) as cmake_presets:
            return json.load(cmake_presets)["configurePresets"]
    return None

def parse_presets(args: str):
    config_presets_list = load_configure_presets(args.cmake_project_dir)
    return_code = 0
    if config_presets_list:
        for config_preset in config_presets_list:
            name = if_need_build_preset(config_preset, args.exclude)
            if name is not None:
                if configure_preset(args, name):
                    binary_dir = compute_binary_dir_v2(
                        args.cmake_project_dir, 
                        config_presets_list, 
                        name)

                    if (binary_dir is not None) and build_preset(args, binary_dir):
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
        print (f'{OK_GREEN}{BOLD}Tests succeed!{END_COLOR}')
    else:
        print(f'{FAIL}{BOLD}ERROR: See log above.{END_COLOR}')
    
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
        default="all building, nothing exclude",
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

    msg = f'Build and tests all: except "{args.exclude}"'
    print(msg)
    print(f'C++ root directory is "{args.cmake_project_dir}"')

    parse_presets(args)
