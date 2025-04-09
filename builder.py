import os
import shutil
import argparse
import pybuilder.builder_functions as builder_functions

# Количество повторений каждого теста в режиме стресс-тестирования.
stress_test_repetitions_count = 150
# Тайм-аут ожидания завершения каждого теста в секундах.
test_timeout_sec = 30

TEST_SUCCESS = 0
TEST_FAIL = -1

available_commands = (
    'Доступные команды:\n'
    ' 0 - Выход\n'
    ' 1 - Запустить все сборки и тесты\n'
    ' 2 - Запустить все сборки и стресс тест\n'
    ' 3 - pc_debug_clang\n'
    ' 4 - freertos_debug_clang\n'
    ' 5 - Стресс тест всех сборок для компилятора gcc\n'
    ' 6 - Стресс тест всех сборок для компилятора clang\n'
    ' 7 - Memcheck only\n'
    ' 11 - Запуск тестов в Docker\n'
    ' 12 - Запуск стресс-тестов в Docker\n'
)


def show_result_output(result: bool):
    """
    Функция выполняет вывод общего результата по всем выбранным тестам.
    :param result: Результат выполнения одного или нескольких тестов.
    """
    if not result and not builder_functions.build_failed_flag:
        print(
            f'{builder_functions.FAIL}TESTS FAILED '
            'in the following presets:'

        )
        for preset, errors in builder_functions.tests_errors_table.items():
            if errors:
                print(f'{preset}:\n {errors}')
        print(f'{builder_functions.END_COLOR}')

    if builder_functions.memcheck_results_table:
        print(f'{builder_functions.WARNING}'
              '\n--------- MEMCHECK observations detected:\n')
        for preset, res in builder_functions.memcheck_results_table.items():
            print(preset)
            print(f'Defects:\n {res["defects"]}', end='')
            print(f'Memcheck results:\n {res["memcheck_results"]}')

        print('--------- MEMCHECK SUMMARY:\n')
        for preset, res in builder_functions.memcheck_results_table.items():
            if res["defects"] != '':
                print(
                    f'{builder_functions.BOLD}'
                    f'{preset}'
                    f'{builder_functions.BOLD}'
                )
                print(res["memcheck_results"])
        print('-------------------------------\n')

    if not builder_functions.build_failed_flag:
        if not result:
            print(
                f'{builder_functions.FAIL}'
                'Testing ended with FAIL, '
                'more information shown above in the terminal!.'
                f'{builder_functions.END_COLOR}\n'
            )
        else:
            print(
                f'{builder_functions.OK_GREEN}'
                'All tests SUCCEEDED!!\n'
                f'{builder_functions.END_COLOR}'
            )


def remove_tmp_docker_entrypoint():
    if os.path.isfile('docker_tests_entrypoint.sh'):
        os.remove('docker_tests_entrypoint.sh')


def replace_docker_entrypoint(entrypoint_name: str):
    remove_tmp_docker_entrypoint()

    shutil.copy(
        entrypoint_name,
        'docker_tests_entrypoint.sh'
    )


def action_matching(action: str):
    result = False
    match action:
        case '0':
            result = True

        case '1':
            test_result = builder_functions.test_multiple_presets(
                test_timeout_sec=test_timeout_sec
            )
            show_result_output(test_result)

            result = test_result

        case '2':
            test_result = builder_functions.test_multiple_presets(
                repetitions_count=stress_test_repetitions_count,
                test_timeout_sec=test_timeout_sec,
                stress_test_flag=True
            )
            show_result_output(test_result)

            result = test_result

        case '3':
            pc_debug_res = builder_functions.test_preset(
                ['cmake', '--preset', 'pc_debug_clang'],
                ['cmake', '--build', 'build/pc_debug_clang/'],
                'build/pc_debug_clang',
                test_timeout_sec=test_timeout_sec
            )

            show_result_output(pc_debug_res)

            result = pc_debug_res

        case '4':
            rtos_debug_res = builder_functions.test_preset(
                ['cmake', '--preset', 'freertos_debug_clang'],
                [
                    'cmake', '--build', 'build/freertos_debug_clang/'
                ],
                'build/freertos_debug_clang',
                test_timeout_sec=test_timeout_sec
            )

            show_result_output(rtos_debug_res)

            result = rtos_debug_res

        case '5':
            test_result = builder_functions.test_multiple_presets(
                'gcc',
                stress_test_repetitions_count,
                test_timeout_sec,
                stress_test_flag=True
            )
            show_result_output(test_result)

            result = test_result

        case '6':
            test_result = builder_functions.test_multiple_presets(
                'clang',
                stress_test_repetitions_count,
                test_timeout_sec,
                stress_test_flag=True
            )
            show_result_output(test_result)

            result = test_result

        case '7':
            replace_docker_entrypoint('docker_tests_entrypoint_memcheck.sh')

            docker_test_result = builder_functions.run_docker_test(
                'docker_test_paraos', 'docker_test_paraos:1.0'
            )
            show_result_output(docker_test_result)

            remove_tmp_docker_entrypoint()

            result = docker_test_result

        case '11':
            replace_docker_entrypoint('docker_tests_entrypoint_single.sh')

            docker_test_result = builder_functions.run_docker_test(
                'docker_test_paraos', 'docker_test_paraos:1.0'
            )
            show_result_output(docker_test_result)

            remove_tmp_docker_entrypoint()

            result = docker_test_result

        case '12':
            replace_docker_entrypoint('docker_tests_entrypoint_stress.sh')

            docker_test_result = builder_functions.run_docker_test(
                'docker_test_paraos', 'docker_test_paraos:1.0'
            )
            show_result_output(docker_test_result)

            remove_tmp_docker_entrypoint()

            result = docker_test_result

        case _:
            print(
                f'{builder_functions.WARNING}'
                'Chosen action is NOT SUPPORTED!'
                f'{builder_functions.END_COLOR}'
            )
            print(available_commands)

    return result


if __name__ == '__main__':
    # Создание парсера аргументов
    arg_parser = argparse.ArgumentParser(description='Builder console args')
    arg_parser.add_argument(
        '-a', '--action',
        type=str,
        help='Действие, которое необходимо выполнить.'
             f' {available_commands}'
    )
    args = arg_parser.parse_args()

    # Если скрипт запущен без аргументов (скорее всего в интерактивном режиме)
    if args.action is None:
        print(available_commands)

        action = input()
    else:
        action = args.action

    if action_matching(action):
        exit(TEST_SUCCESS)
    else:
        exit(TEST_FAIL)
