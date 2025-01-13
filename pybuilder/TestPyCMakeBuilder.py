import unittest
from pycmakebuilder import find_base_preset, find_binary_dir, compute_binary_dir, is_preset_hidden

class TestComputeBinaryDir(unittest.TestCase):
    def setUp(self):
        self.cmake_presets_json = {
            "version": 6,
            "cmakeMinimumRequired": {
                "major": 3,
                "minor": 23,
                "patch": 0
            },
            "configurePresets": [
                {
                    "name": "stavpilot_f405_gcc_debug",
                    "displayName": "Stavpilot board STM32F405 debug with GCC",
                    "description": "Debug version for stavpilot with STM32F405RGT6",
                    "generator": "Ninja",
                    "binaryDir": "${sourceDir}/build/${presetName}",
                    "hidden": False,
                    "cacheVariables": {
                        "CMAKE_BUILD_TYPE": "Debug",
                        "NEED_ELF_OUTPUT": True,
                        "FREERTOS": True,
                        "STAVPILOT_STM32F405_BOARD": True,
                        "SCRIPT_BUILD_ONLY": True,
                        "HOST_TYPE": "MCU",
                        "PRESET_NAME": "${presetName}"
                    }
                },
                {
                    "name": "stavpilot_f405_gcc_release",
                    "displayName": "Stavpilot board STM32F405 release with GCC",
                    "description": "Release version for stavpilot with STM32F405RGT6",
                    "inherits": "stavpilot_f405_gcc_debug",
                    "cacheVariables": {
                        "CMAKE_BUILD_TYPE": "Release"
                    }
                },
                {
                    "name": "pc_gcc_with_simulation_cpp",
                    "displayName": "PC platform configuration with simulation from C++-model",
                    "description": "Version for communicating with model from stavpilot",
                    "inherits": "stavpilot_f405_gcc_release",
                    "cacheVariables": {
                        "SIM_MODE": "CPP_SITL",
                        "SCRIPT_BUILD_ONLY": True
                    }
                }
            ]
        }
        
    def test_find_base_preset_if_no_inheritance(self):
        self.assertEqual(find_base_preset(
            self.cmake_presets_json["configurePresets"], 
            "stavpilot_f405_gcc_debug"), "stavpilot_f405_gcc_debug")
        
    def test_find_base_preset_if_one_inheritance(self):
        self.assertEqual(find_base_preset(
            self.cmake_presets_json["configurePresets"], 
            "stavpilot_f405_gcc_release"), "stavpilot_f405_gcc_debug")
        
    def test_find_base_preset_if_two_inheritance(self):
        self.assertEqual(find_base_preset(
            self.cmake_presets_json["configurePresets"], 
            "pc_gcc_with_simulation_cpp"), "stavpilot_f405_gcc_release")
        
    def test_find_raw_binary_dir(self):
        compute_path = find_binary_dir(
            self.cmake_presets_json["configurePresets"], 
            "stavpilot_f405_gcc_debug")
        expected_path = "${sourceDir}/build/${presetName}"
        self.assertEqual(compute_path, expected_path)
        
    def test_compute_binary_dir(self):
        compute_path = compute_binary_dir("home", 
                           self.cmake_presets_json["configurePresets"], 
                           "stavpilot_f405_gcc_debug")
        expected_path = "home\\build\\stavpilot_f405_gcc_debug"
        self.assertEqual(compute_path, expected_path)
        
    def test_compute_binary_dir_if_one_inheritance(self):
        compute_path = compute_binary_dir("home", 
                        self.cmake_presets_json["configurePresets"], 
                        "stavpilot_f405_gcc_release")
        expected_path = "home\\build\\stavpilot_f405_gcc_release"
        self.assertEqual(compute_path, expected_path)
        
    def test_compute_binary_dir_if_two_inheritance(self):
        compute_path = compute_binary_dir("home", 
                        self.cmake_presets_json["configurePresets"], 
                        "pc_gcc_with_simulation_cpp")
        expected_path = "home\\build\\pc_gcc_with_simulation_cpp"
        self.assertEqual(compute_path, expected_path)

class TestCheckHidden(unittest.TestCase):
    def setUp(self):
        self.cmake_presets_json = {
            "version": 6,
            "cmakeMinimumRequired": {
                "major": 3,
                "minor": 23,
                "patch": 0
            },
            "configurePresets": [
                {
                    "name": "stavpilot_f405_gcc_debug_not_hidden",
                    "displayName": "Stavpilot board STM32F405 debug with GCC",
                    "description": "Debug version for stavpilot with STM32F405RGT6",
                    "generator": "Ninja",
                    "binaryDir": "${sourceDir}/build/${presetName}",
                    "hidden": False
                },
                {
                    "name": "stavpilot_f405_gcc_debug",
                    "displayName": "Stavpilot board STM32F405 debug with GCC",
                    "description": "Debug version for stavpilot with STM32F405RGT6",
                    "generator": "Ninja",
                    "binaryDir": "${sourceDir}/build/${presetName}"
                },
                {
                    "name": "stavpilot_f405_gcc_debug_hidden",
                    "displayName": "Stavpilot board STM32F405 debug with GCC",
                    "description": "Debug version for stavpilot with STM32F405RGT6",
                    "generator": "Ninja",
                    "binaryDir": "${sourceDir}/build/${presetName}",
                    "hidden": True
                },
            ]
        }
        
    def test_if_hidden_false(self):
         self.assertEqual(
             is_preset_hidden(
                 self.cmake_presets_json["configurePresets"][0]), 
             False)
         
    def test_if_no_hidden(self):
         self.assertEqual(
             is_preset_hidden(
                 self.cmake_presets_json["configurePresets"][1]), 
             False)
         
    def test_if_hidden(self):
         self.assertEqual(
             is_preset_hidden(
                 self.cmake_presets_json["configurePresets"][2]), 
             True)

# Executing the tests in the above test case class
if __name__ == "__main__":
    unittest.main()