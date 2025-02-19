import unittest
from pycmakebuilder import reduced_if_hidden
from pycmakebuilder import find_base_preset, find_binary_dir
from pycmakebuilder import compute_binary_dir, is_preset_hidden
from pycmakebuilder import get_regex_set, get_resulting_include_and_build_sets


class TestComputeBinaryDir(unittest.TestCase):  # NOQA Found too many methods: 8 > 7
    def setUp(self):
        self.cmake_presets_json = {
            'version': 6,
            'cmakeMinimumRequired': {'major': 3, 'minor': 23, 'patch': 0},
            'configurePresets': [
                {
                    'name': 'stavpilot_f405_gcc_debug',
                    'displayName': 'Stavpilot board STM32F405 debug with GCC',
                    'description': 'Debug version for stavpilot with '
                    'STM32F405RGT6',
                    'generator': 'Ninja',
                    'binaryDir': '${sourceDir}/build/${presetName}',
                    'hidden': False,
                    'cacheVariables': {
                        'CMAKE_BUILD_TYPE': 'Debug',
                        'NEED_ELF_OUTPUT': True,
                        'FREERTOS': True,
                        'STAVPILOT_STM32F405_BOARD': True,
                        'SCRIPT_BUILD_ONLY': True,
                        'HOST_TYPE': 'MCU',
                        'PRESET_NAME': '${presetName}',
                    },
                },
                {
                    'name': 'stavpilot_f405_gcc_release',
                    'displayName': 'Stavpilot board STM32F405 '
                    'release with GCC',
                    'description': 'Release version for stavpilot with '
                    'STM32F405RGT6',
                    'inherits': 'stavpilot_f405_gcc_debug',
                    'cacheVariables': {'CMAKE_BUILD_TYPE': 'Release'},
                },
                {
                    'name': 'pc_gcc_with_simulation_cpp',
                    'displayName': 'PC platform configuration with '
                    'simulation from C++-model',
                    'description': 'Version for communicating with '
                    'model from stavpilot',
                    'inherits': 'stavpilot_f405_gcc_release',
                    'cacheVariables': {
                        'SIM_MODE': 'CPP_SITL',
                        'SCRIPT_BUILD_ONLY': True,
                    },
                },
            ],
        }

    def test_find_base_preset_if_no_inheritance(self):
        self.assertEqual(
            find_base_preset(
                self.cmake_presets_json['configurePresets'],
                'stavpilot_f405_gcc_debug',
            ),
            'stavpilot_f405_gcc_debug',
        )

    def test_find_base_preset_if_one_inheritance(self):
        self.assertEqual(
            find_base_preset(
                self.cmake_presets_json['configurePresets'],
                'stavpilot_f405_gcc_release',
            ),
            'stavpilot_f405_gcc_debug',
        )

    def test_find_base_preset_if_two_inheritance(self):
        self.assertEqual(
            find_base_preset(
                self.cmake_presets_json['configurePresets'],
                'pc_gcc_with_simulation_cpp',
            ),
            'stavpilot_f405_gcc_release',
        )

    def test_find_raw_binary_dir(self):
        compute_path = find_binary_dir(
            self.cmake_presets_json['configurePresets'],
            'stavpilot_f405_gcc_debug',
        )
        expected_path = '${sourceDir}/build/${presetName}'
        self.assertEqual(compute_path, expected_path)

    def test_compute_binary_dir(self):
        compute_path = compute_binary_dir(
            'home',
            self.cmake_presets_json['configurePresets'],
            'stavpilot_f405_gcc_debug',
        )
        expected_path = 'home/build/stavpilot_f405_gcc_debug'
        self.assertEqual(compute_path, expected_path)

    def test_compute_binary_dir_if_one_inheritance(self):
        compute_path = compute_binary_dir(
            'home',
            self.cmake_presets_json['configurePresets'],
            'stavpilot_f405_gcc_release',
        )
        expected_path = 'home/build/stavpilot_f405_gcc_release'
        self.assertEqual(compute_path, expected_path)

    def test_compute_binary_dir_if_two_inheritance(self):
        compute_path = compute_binary_dir(
            'home',
            self.cmake_presets_json['configurePresets'],
            'pc_gcc_with_simulation_cpp',
        )
        expected_path = 'home/build/pc_gcc_with_simulation_cpp'
        self.assertEqual(compute_path, expected_path)


class TestCheckHidden(unittest.TestCase):
    def setUp(self):
        self.cmake_presets_json = {
            'version': 6,
            'cmakeMinimumRequired': {'major': 3, 'minor': 23, 'patch': 0},
            'configurePresets': [
                {
                    'name': 'stavpilot_f405_gcc_debug_not_hidden',
                    'displayName': 'Stavpilot board STM32F405 debug with GCC',
                    'description': 'Debug ver for '
                    'stavpilot with STM32F405RGT6',
                    'generator': 'Ninja',
                    'binaryDir': '${sourceDir}/build/${presetName}',
                    'hidden': False,
                },
                {
                    'name': 'stavpilot_f405_gcc_debug',
                    'displayName': 'Stavpilot board STM32F405 debug with GCC',
                    'description': 'Debug ver for '
                    'stavpilot with STM32F405RGT6',
                    'generator': 'Ninja',
                    'binaryDir': '${sourceDir}/build/${presetName}',
                },
                {
                    'name': 'stavpilot_f405_gcc_debug_hidden',
                    'displayName': 'Stavpilot board STM32F405 debug with GCC',
                    'description': 'Debug version for '
                    'stavpilot with STM32F405RGT6',
                    'generator': 'Ninja',
                    'binaryDir': '${sourceDir}/build/${presetName}',
                    'hidden': True,
                },
            ],
        }

    def test_if_hidden_false(self):
        self.assertFalse(
            is_preset_hidden(self.cmake_presets_json['configurePresets'][0])
        )

    def test_if_no_hidden(self):
        self.assertFalse(
            is_preset_hidden(self.cmake_presets_json['configurePresets'][1])
        )

    def test_if_hidden(self):
        self.assertTrue(
            is_preset_hidden(self.cmake_presets_json['configurePresets'][2])
        )

    def test_get_not_hidden_presets_list(self):
        test_list = [
            {
                'binaryDir': '${sourceDir}/build/${presetName}',
                'description': 'Debug ver for stavpilot with STM32F405RGT6',
                'displayName': 'Stavpilot board STM32F405 debug with GCC',
                'generator': 'Ninja',
                'hidden': False,
                'name': 'stavpilot_f405_gcc_debug_not_hidden',
            },
            {
                'binaryDir': '${sourceDir}/build/${presetName}',
                'description': 'Debug ver for stavpilot with STM32F405RGT6',
                'displayName': 'Stavpilot board STM32F405 debug with GCC',
                'generator': 'Ninja',
                'name': 'stavpilot_f405_gcc_debug',
            },
        ]
        self.assertEqual(
            reduced_if_hidden(self.cmake_presets_json['configurePresets']),
            test_list,
        )


class TestRegEx(unittest.TestCase):  # NOQA Found too many methods: 8 > 7
    def setUp(self):
        self.cmake_presets_json = {
            'version': 6,
            'cmakeMinimumRequired': {'major': 3, 'minor': 23, 'patch': 0},
            'configurePresets': [
                {
                    'name': 'stavpilot_f405_gcc_debug',
                    'displayName': 'Stavpilot board STM32F405 debug with GCC',
                    'description': 'Debug version for '
                    'stavpilot with STM32F405RGT6',
                    'generator': 'Ninja',
                    'binaryDir': '${sourceDir}/build/${presetName}',
                },
                {
                    'name': 'stavpilot_f405_gcc_release',
                    'displayName': 'Stavpilot STM32F405 release with GCC',
                    'description': 'Release version for stavpilot'
                    ' with STM32F405RGT6',
                    'inherits': 'stavpilot_f405_gcc_debug',
                    'cacheVariables': {
                        'CMAKE_BUILD_TYPE': 'Release',
                        'SCRIPT_BUILD_ONLY': True,
                    },
                },
                {
                    'name': 'pc_debug_clang_native',
                    'displayName': 'PC platform debug with clang and '
                    'winapi/posix',
                    'description': 'Debug version for stavpilot',
                    'inherits': 'pc_debug_clang',
                    'cacheVariables': {'UTEST': True},
                },
                {
                    'name': 'pc_debug_gcc_native_with_coverage',
                    'displayName': 'PC platform debug with GCC and '
                    'winapi/posix (with code coverage)',
                    'description': 'Debug version for stavpilot',
                    'inherits': 'pc_debug_gcc',
                    'cacheVariables': {
                        'UTEST': True,
                        'CODE_COVERAGE': True,
                        'SCRIPT_BUILD_ONLY': True,
                    },
                },
                {
                    'name': 'pc_debug_gcc_native_clang_tidy',
                    'displayName': 'PC platform debug with GCC and '
                    'winapi/posix with clang-tidy checks',
                    'description': 'Debug version for stavpilot',
                    'generator': 'Ninja',
                    'binaryDir': '${sourceDir}/build/${presetName}',
                    'inherits': 'pc_debug_clang',
                    'cacheVariables': {
                        'UTEST': True,
                        'CLANG_TIDY_ENABLE': True,
                        'SCRIPT_BUILD_ONLY': True,
                    },
                },
            ],
        }

    def test_get_regex_set_if_regex_is_none(self):
        self.assertEqual(
            get_regex_set(self.cmake_presets_json['configurePresets'], None),
            set(),
        )

    def test_get_regex_set_for_all_presets(self):
        test_set = {
            'stavpilot_f405_gcc_debug',
            'stavpilot_f405_gcc_release',
            'pc_debug_clang_native',
            'pc_debug_gcc_native_with_coverage',
            'pc_debug_gcc_native_clang_tidy',
        }
        self.assertEqual(
            get_regex_set(
                self.cmake_presets_json['configurePresets'],
                # Empty string means that we're going to use all presets
                # from the list.
                '',
            ),
            test_set,
        )

    def test_get_regex_set(self):
        test_set = {'pc_debug_clang_native', 'pc_debug_gcc_native_clang_tidy'}
        self.assertEqual(
            get_regex_set(
                self.cmake_presets_json['configurePresets'], 'clang*'
            ),
            test_set,
        )
        self.assertEqual(
            get_regex_set(
                self.cmake_presets_json['configurePresets'], 'release|coverage'
            ),
            {
                'stavpilot_f405_gcc_release',
                'pc_debug_gcc_native_with_coverage',
            },
        )

    def test_getting_resulting_build_and_include_sets(self):
        # Operator OR to match names which contain
        # either "clang_tidy" or "release"
        build_only_regex = 'clang_tidy|release'
        # Check exclude overlapping on the build set
        exclude_regex = 'coverage|release'

        test_incl_set = {'stavpilot_f405_gcc_debug', 'pc_debug_clang_native'}
        test_build_set = {'pc_debug_gcc_native_clang_tidy'}

        include_set, build_only_set = get_resulting_include_and_build_sets(
            self.cmake_presets_json['configurePresets'],
            # Empty string means that we're going to use all presets
            # from the list.
            '',
            build_only_regex,
            exclude_regex,
        )

        self.assertEqual(include_set, test_incl_set)
        self.assertEqual(build_only_set, test_build_set)

    def test_getting_include_set_only(self):
        test_incl_set = {
            'pc_debug_gcc_native_clang_tidy',
            'pc_debug_clang_native',
        }

        include_set, build_only_set = get_resulting_include_and_build_sets(
            self.cmake_presets_json['configurePresets'], 'clang*', None, None
        )

        self.assertEqual(include_set, test_incl_set)
        self.assertEqual(build_only_set, set())

    def test_getting_build_set_only(self):
        include_set, build_only_set = get_resulting_include_and_build_sets(
            self.cmake_presets_json['configurePresets'],
            None,
            'coverage*',
            None,
        )

        self.assertEqual(include_set, set())
        self.assertEqual(build_only_set, {'pc_debug_gcc_native_with_coverage'})

    def test_sets_when_include_and_exclude(self):
        include_set, build_only_set = get_resulting_include_and_build_sets(
            self.cmake_presets_json['configurePresets'], 'gcc', None, 'clang'
        )

        self.assertEqual(include_set, set())
        self.assertEqual(build_only_set, set())


# Executing the tests in the above test case class
if __name__ == '__main__':
    unittest.main()
