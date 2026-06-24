#!/usr/bin/env python3
"""
Скрипт сборки всех конфигураций PARAOS и запуска стресс-тестов.

Использует библиотеку pybuilder для запуска тестов.

Алгоритм работы:
  1. Собираются все неисключённые configure-пресеты из CMakePresets.json.
  2. Для каждой успешно собранной конфигурации один раз запускаются
     все тесты (timeout 20 с).
  3. Для каждой конфигурации, прошедшей обычное тестирование,
     запускаются тесты с меткой stress (timeout 20 с),
     каждый тест повторяется N раз (по умолчанию 50).

При первой же ошибке на любом этапе скрипт сразу выводит сообщение
и завершает работу с ненулевым кодом возврата.

Количество повторений стресс-тестов можно переопределить аргументом -r/--repetitions.
"""

import argparse
import subprocess
import sys
from pathlib import Path

# Добавляем pybuilder в PYTHONPATH, чтобы импортировать builder_functions.
SCRIPT_DIR = Path(__file__).resolve().parent
PYBUILDER_DIR = SCRIPT_DIR / 'pybuilder'
sys.path.insert(0, str(PYBUILDER_DIR))

import builder_functions  # noqa: E402

DEFAULT_STRESS_REPETITIONS = 50
DEFAULT_TEST_TIMEOUT_SEC = 20

# Пресеты, которые нужно исключить из обработки по умолчанию.
EXCLUDED_PRESETS = ('freertos_release_clang',)

OK_GREEN = builder_functions.OK_GREEN
BLUE = builder_functions.BLUE
WARNING = builder_functions.WARNING
FAIL = builder_functions.FAIL
END_COLOR = builder_functions.END_COLOR
BOLD = builder_functions.BOLD


def error_and_exit(message):
    """
    Выводит сообщение об ошибке и завершает работу скрипта с кодом 1.

    :param message: Текст сообщения об ошибке.
    """
    print(f'\n{FAIL}{BOLD}{message}{END_COLOR}')
    sys.exit(1)


def load_presets():
    """
    Загружает имена видимых configure-пресетов через pybuilder.

    :return: Кортеж имён пресетов.
    """
    if not builder_functions.check_presets_existence():
        error_and_exit('Не найден файл пресетов CMakePresets.json')

    # parse_presets возвращает кортеж всех не hidden и не исключённых пресетов.
    return builder_functions.parse_presets()


def filter_presets(presets, filter_keyword):
    """
    Фильтрует пресеты по ключевому слову и списку исключений.

    :param presets: Исходный кортеж имён пресетов.
    :param filter_keyword: Подстрока для фильтрации (None/пусто — без фильтра).
    :return: Список имён пресетов.
    """
    result = [
        name for name in presets
        if name not in EXCLUDED_PRESETS
    ]
    if filter_keyword:
        result = [name for name in result if filter_keyword in name]
    return result


def configure_and_build_preset(preset_name):
    """
    Конфигурирует и собирает выбранный CMake-пресет.

    :param preset_name: Имя пресета.
    :return: True при успехе, иначе False.
    """
    print(f'\n{BLUE}{BOLD}CONFIGURE: {preset_name}{END_COLOR}')
    builder_functions._make_preset(['cmake', '--preset', preset_name])

    print(f'{BLUE}{BOLD}BUILD: {preset_name}{END_COLOR}')
    build_result = builder_functions._build_preset(
        ['cmake', '--build', f'build/{preset_name}/']
    )

    return build_result.returncode == 0


def is_freertos_preset(preset_name):
    """
    Проверяет, относится ли пресет к конфигурации FreeRTOS.

    :param preset_name: Имя пресета.
    :return: True, если пресет относится к FreeRTOS, иначе False.
    """
    return 'freertos' in preset_name.lower()


def run_tests_once(preset_name, timeout_sec):
    """
    Запускает все тесты выбранной конфигурации один раз.

    Для FreeRTOS-пресетов тесты запускаются в один поток (-j1).

    :param preset_name: Имя пресета.
    :param timeout_sec: Таймаут каждого теста в секундах.
    :return: True при успехе, иначе False.
    """
    print(f'\n{BLUE}{BOLD}RUN ALL TESTS ONCE: {preset_name}{END_COLOR}')

    test_dir = f'build/{preset_name}'
    command = [
        'ctest',
        '--test-dir', test_dir,
        '--timeout', f'{timeout_sec}',
        '--output-on-failure',
        '--stop-on-failure',
        '--schedule-random',
    ]
    if is_freertos_preset(preset_name):
        command.extend(['-j', '1'])

    print(f'Команда: {" ".join(command)}')
    result = subprocess.run(command, cwd=SCRIPT_DIR)
    return result.returncode == 0


def run_stress_tests(preset_name, repetitions, timeout_sec):
    """
    Запускает стресс-тесты выбранной конфигурации.

    Для FreeRTOS-пресетов тесты запускаются в один поток (-j1).

    :param preset_name: Имя пресета.
    :param repetitions: Количество повторений каждого стресс-теста.
    :param timeout_sec: Таймаут каждого теста в секундах.
    :return: True при успехе, иначе False.
    """
    print(
        f'\n{BLUE}{BOLD}RUN STRESS TESTS ({repetitions} repetitions): '
        f'{preset_name}{END_COLOR}'
    )

    test_dir = f'build/{preset_name}'
    command = [
        'ctest',
        '--test-dir', test_dir,
        '-L', 'stress',
        '--timeout', f'{timeout_sec}',
        '--repeat-until-fail', f'{repetitions}',
        '--output-on-failure',
        '--stop-on-failure',
        '--schedule-random',
    ]
    if is_freertos_preset(preset_name):
        command.extend(['-j', '1'])

    print(f'Команда: {" ".join(command)}')
    result = subprocess.run(command, cwd=SCRIPT_DIR)
    return result.returncode == 0


def main():
    """
    Главная функция скрипта.
    """
    parser = argparse.ArgumentParser(
        description='Сборка всех конфигураций PARAOS и запуск стресс-тестов.',
    )
    parser.add_argument(
        '-r', '--repetitions',
        type=int,
        default=DEFAULT_STRESS_REPETITIONS,
        help=f'Количество повторений каждого стресс-теста (по умолчанию {DEFAULT_STRESS_REPETITIONS}).',
    )
    parser.add_argument(
        '-t', '--timeout',
        type=int,
        default=DEFAULT_TEST_TIMEOUT_SEC,
        help=f'Таймаут каждого теста в секундах (по умолчанию {DEFAULT_TEST_TIMEOUT_SEC}).',
    )
    parser.add_argument(
        '-f', '--filter',
        type=str,
        default='',
        help='Фильтр имён пресетов (подстрока). По умолчанию обрабатываются все пресеты.',
    )
    parser.add_argument(
        '--no-stress',
        action='store_true',
        help='Выполнить только сборку и обычное тестирование, не запускать стресс-тесты.',
    )
    args = parser.parse_args()

    if args.repetitions < 1:
        error_and_exit('Количество повторений должно быть >= 1')
    if args.timeout < 1:
        error_and_exit('Таймаут должен быть >= 1 секунды')

    all_presets = load_presets()
    presets = filter_presets(all_presets, args.filter)

    if not presets:
        print(f'{WARNING}Не найдено пресетов для обработки.{END_COLOR}')
        sys.exit(0)

    print(f'Найдены пресеты для обработки: {presets}')

    # Этап 1: configure + build для всех пресетов.
    # При первой ошибке сборки сразу завершаем работу.
    for preset in presets:
        if not configure_and_build_preset(preset):
            error_and_exit(f'Сборка пресета {preset} завершилась с ошибкой')

    # Этап 2: запуск всех тестов по одному разу.
    # При первой ошибке тестирования сразу завершаем работу.
    for preset in presets:
        if not run_tests_once(preset, args.timeout):
            error_and_exit(
                f'Обычное тестирование пресета {preset} завершилось с ошибкой'
            )

    if args.no_stress:
        print(
            f'\n{OK_GREEN}{BOLD}Обычное тестирование завершено. '
            f'Стресс-тесты пропущены по флагу --no-stress.{END_COLOR}'
        )
        sys.exit(0)

    # Этап 3: запуск стресс-тестов.
    # При первой ошибке стресс-тестирования сразу завершаем работу.
    for preset in presets:
        if not run_stress_tests(preset, args.repetitions, args.timeout):
            error_and_exit(
                f'Стресс-тестирование пресета {preset} завершилось с ошибкой'
            )

    print(
        f'\n{OK_GREEN}{BOLD}Все сборки и тесты прошли успешно!{END_COLOR}'
    )
    sys.exit(0)


if __name__ == '__main__':
    main()
