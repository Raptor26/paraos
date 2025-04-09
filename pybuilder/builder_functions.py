import os
import sys
import json
import subprocess

try:
    from python_on_whales import docker, DockerException
except (ImportError, DockerException) as exception:
    print(
        f'ImportError happened: {exception[0]}. '
        'Try to install builder.py dependencies: '
        '"python install_builder_dependencies.py"'
    )

# Список ключевых слов для исключения при поиске пресетов, пресеты,
# содержащие данные слова не будут участвовать в тестировании.
exclude_keywords_list = ['_trace', '_docker']

# Список ключевых слов, используемых при поиске пресетов, для которых нет
# необходимости запускать тестирование (выполняется только сборка).
no_test_keywords_list = []

tests_errors_table = {}

test_out_file_name = 'pybuilder_test_output.txt'

# Значение ключа для доступа к результатам команды memcheck.
memcheck_results_key = 'memcheck_results'

# Значение ключа для доступа к имени пресета.
preset_name_key = 'name'

# Таблица с результатами memcheck:
# 'preset_name': {
#     'defects': '',
#     'memcheck_results': ''
# }
memcheck_results_table: dict = {}

OK_GREEN = '\033[92m'
BLUE = '\033[94m'
WARNING = '\033[93m'
FAIL = '\033[91m'
END_COLOR = '\033[0m'
BOLD = '\033[1m'

build_failed_flag = False


def check_presets_existence():
    """
    Функция проверяет наличие файла CMakePresets.json в одной директории со
    скриптом builder.
    :return: Возвращает True, если файл CMakePresets.json существует,
        иначе - False.
    """
    if os.path.isfile('CMakePresets.json'):
        return True
    else:
        return False


def parse_presets(presets_filter: str = '', stress_test_flag: bool = False):
    """
    Функция выполняет парсинг доступных пресетов из файла CMakePresets.json.
    :param presets_filter: Фильтр пресетов - строка, используемая для
        отбора только тех пресетов, которые содержат данную строку.
    :param stress_test_flag: Флаг, который равен true на этапе
        стресс-тестирования.
    :return: Возвращает кортеж пресетов, полученных в результате парсинга.
    """
    if check_presets_existence():
        with open('CMakePresets.json') as presets_file:
            presets_dict = json.load(presets_file)
            config_presets_list = presets_dict['configurePresets']

            # Добавление пресетов в список исключения, если в их cache
            # variables содержится флаг "SCRIPT_BUILD_ONLY"
            no_test_keywords_list.extend(
                (
                    preset_data[preset_name_key] for
                    preset_data in config_presets_list
                    if 'SCRIPT_BUILD_ONLY' in preset_data['cacheVariables']
                )
            )

            # В случае стресс тестирования нет необходимости собирать
            # повторно пресеты, которые, затем, не запускаются в тестировании.
            if stress_test_flag:
                exclude_keywords_list.extend(no_test_keywords_list)

            presets_tuple = tuple(
                preset_data[preset_name_key]
                for preset_data in config_presets_list
                if not any(
                    string in preset_data[preset_name_key]
                    for string in exclude_keywords_list
                )
                and preset_data[preset_name_key].find(presets_filter) != -1
                and 'hidden' not in preset_data
            )

            tests_errors_table.update(
                {
                    preset_name: '' for preset_name in presets_tuple
                }
            )
            return presets_tuple

    else:
        print(FAIL, 'NO CMakePresets.json was found!', END_COLOR)


def _make_preset(make_command: list[str]):
    """
    Функция выполняет команду make с заданными аргументами.
    :param make_command: Команда make, которую необходимо запустить.
    """
    make_result = subprocess.run(
        make_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    print(make_result.stdout)
    print(make_result.stderr)


def _build_preset(build_command: list[str]):
    """
    Функция выполняет сборку проекта, выполняя запуск соответствующей команды с
    заданными аргументами.
    :param build_command: Команда для сборки проекта,
        которую необходимо запустить.
    :return: Возвращает объект завершённого процесса для доступа к его
        результатам.
    """
    preset_name = build_command[2].split('/')[1]
    build_process = subprocess.Popen(
        build_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    global build_failed_flag
    build_output = ''
    while True:
        out = build_process.stdout.readline()
        if build_process.poll() is not None:
            break
        if out != '':

            if out.find('FAILED:') != -1:
                build_failed_flag = True

            sys.stdout.write(out)
            sys.stdout.flush()

            if build_failed_flag:
                build_output += out

    if build_failed_flag:
        tests_errors_table[preset_name] += build_output

    return build_process


def _parse_cmake_line(line):
    found_fail_test_out = False
    output_str = ''

    if line != '':
        line = line.replace('  ', ' ')
        if line.find('***') != -1:
            found_fail_test_out = True

        elif line.find('FAILED TEST') != -1 or line.find(
                'Test #') != -1:
            found_fail_test_out = False

        if found_fail_test_out:
            output_str += line

    return output_str


def _parse_cmake_test_output(preset_name):
    failed_test_output = ''

    with (open(test_out_file_name, 'r')) as cmake_output_file:
        for line in cmake_output_file.readlines():
            failed_test_output += _parse_cmake_line(line)

    if os.path.isfile(test_out_file_name):
        os.remove(test_out_file_name)

    if len(failed_test_output) > 0:
        tests_errors_table[preset_name] = failed_test_output

        return False
    else:
        return True


def test_preset(
        make_command: list[str],
        build_command: list[str],
        test_dir: str,
        repetitions_count: int = 2,
        test_timeout_sec: int = 30
):
    """
    Функция выполняет тестирование выбранного пресета.
    :param make_command: Команда make с заданными аргументами,
        которую необходимо запустить.
    :param build_command: Команда build с заданными аргументами,
        которую необходимо запустить.
    :param test_dir: Путь к директории для тестов ctest.
    :param repetitions_count: Количество повторений каждого теста.
    :param test_timeout_sec: Тайм-аут ожидания завершения каждого теста в
        секундах.
    :return: Возвращает результат тестирования пресета.
    """
    preset_name = make_command[2]
    print(f'{BLUE}{BOLD}MAKE phase of the {preset_name}{END_COLOR}')
    _make_preset(make_command)

    print(f'{BLUE}{BOLD}BUILD phase of the {preset_name}{END_COLOR}')
    build_res = _build_preset(build_command)
    if build_res.returncode != 0:
        print(
            f'{FAIL}BUILD of the {preset_name} FAILED!\n'
            f'{tests_errors_table[preset_name]}{END_COLOR}'
        )
        return False

    # Если имя текущего пресета не содержит подстрок, добавленных в список,
    # исключающий тестирование.
    if not any(string in preset_name for string in no_test_keywords_list):
        print(f'{BLUE}{BOLD}TEST phase of the {preset_name}{END_COLOR}')

        subprocess.run(
            [
                'ctest',
                '--test-dir',
                test_dir,
                # Вывод ctest дублируется в текстовый документ для его
                # дальнейшего анализа.
                '--output-log', test_out_file_name,
                '--timeout', f'{test_timeout_sec}',
                '--repeat-until-fail', f'{repetitions_count}',
                '--stop-on-failure',
                '--output-on-failure',
                '--schedule-random'
            ],
            text=True
        )

        return _parse_cmake_test_output(preset_name)

    print(
        f'\n{WARNING}{BOLD}Preset {preset_name} has been SKIPPED from '
        f'testing due to EXCLUSION LIST!{END_COLOR}\n'
    )

    return True


def test_multiple_presets(
        presets_filter: str = '', repetitions_count: int = 1,
        test_timeout_sec: int = 30, stress_test_flag: bool = False):
    """
    Метод выполняет поиск и тестирование нескольких выбранных пресетов.
    :param presets_filter: Ключевое слово-фильтр, которое позволяет отбирать
        только пресеты, содержащие данное слово.
    :param repetitions_count: Количество повторений каждого теста.
    :param test_timeout_sec: Тайм-аут ожидания завершения каждого теста в
        секундах.
    :return: Возвращает True, если тесты всех пресетов завершились успешно,
    иначе - False.
    """
    presets_tuple = parse_presets(presets_filter, stress_test_flag)
    final_res = True
    if presets_tuple:
        if presets_filter != '':
            print(f'For the following FILTER {presets_filter}:')

        print(f'Found following presets:\n{presets_tuple}')

        for preset in presets_tuple:
            preset_res = test_preset(
                ['cmake', '--preset', preset],
                ['cmake', '--build', f'build/{preset}/'],
                f'build/{preset}',
                repetitions_count,
                test_timeout_sec
            )

            if not preset_res:
                final_res = False
                break

    else:
        print(f'{WARNING}No presets was found!{END_COLOR}')

    return final_res


def _build_docker_image(image_tags: str):
    """
    Функция выполняет сборку Docker образа.
    :param image_tags: Тег, который необходимо присвоить создаваемому образу.
    :return: Возвращает результат выполнения операции.
    """
    failed_flag = False
    fail_output = ''
    try:
        build_output_generator = docker.build(
            context_path='.',
            tags=image_tags,
            stream_logs=True
        )

        for stream_content in build_output_generator:
            print({stream_content})
            if stream_content.find('FAILED:') != -1:
                failed_flag = True
            if failed_flag:
                fail_output += stream_content

        return True

    except DockerException:
        tests_errors_table['docker_tests'] = fail_output
        return False


def _run_docker_container(image_name: str, image_tags: str):
    """
    Функция выполняет запуск Docker контейнера и отслеживание его вывода для
    сохранения ошибок.
    :param image_name: Название Docker контейнера,
        присваиваемое ему после запуска.
    :param image_tags: Тег образа, на основе которого
        необходимо запустить контейнер.
    :return: Возвращает результат выполнения операции.
    """
    container_result = True
    memcheck_flag = False
    tests_fail_flag = False
    tests_fail_out = b''
    memcheck_output = ''
    memcheck_results_flag = False
    preset_name = 'preset'
    try:
        memcheck_results_table.clear()
        run_output_generator = docker.run(
            name=image_name,
            image=image_tags,
            stream=True,
            remove=True,
            privileged=True
        )

        for _, stream_content in run_output_generator:
            print(stream_content)
            if stream_content.find(b'Running main()') != -1:
                tests_fail_flag = True
            elif stream_content.find(b'[TEST START]') != -1:
                tests_fail_flag = False
                tests_fail_out += b'\n' + stream_content + b'\n'

            elif stream_content.find(b'FAILED TEST') != -1:
                tests_fail_flag = False

            if tests_fail_flag:
                tests_fail_out += stream_content

            if stream_content.find(
                    b'-- Processing memory checking output:') != -1:
                memcheck_flag = True
                stream_content = b''
            if stream_content.find(
                    b'Memory checking results:') != -1:
                memcheck_flag = True
                if len(memcheck_output) > 0:
                    memcheck_results_table[preset_name][
                        'defects'] = memcheck_output
                else:
                    memcheck_results_table.pop(preset_name, None)
                memcheck_output = ''
                memcheck_results_flag = True
                stream_content = b''

            elif stream_content.find(
                    b'[MEMCHECK START]') != -1:
                if (preset_name != 'preset'
                        and preset_name in memcheck_results_table):
                    memcheck_results_table[preset_name][
                        memcheck_results_key] = memcheck_output
                    memcheck_results_flag = False
                    memcheck_output = ''

                preset_name = stream_content.split(b'[MEMCHECK START] ')[1]
                preset_name = preset_name.decode('utf-8')
                preset_name = preset_name.strip()
                memcheck_results_table.update(
                    {
                        preset_name: {
                            memcheck_results_key: '',
                            'defects': ''
                        }
                    }
                )
                memcheck_flag = False

            elif stream_content.find(
                    b'MemCheck log files can be found here:') != -1:
                memcheck_flag = False

            if memcheck_flag:
                line = stream_content.decode('utf-8')
                if line.find('Errors while running CTest') != -1:
                    continue
                if memcheck_results_flag:
                    if line.find('Potential') == -1:
                        memcheck_output += FAIL + line + WARNING
                    else:
                        memcheck_output += line
                else:
                    memcheck_output += line

        if preset_name in memcheck_results_table:
            if len(memcheck_output) > 0:
                # implicit `.get()` dict usage - Здесь не происходит
                # получение значения из словаря, поэтому использование get()
                # здесь неоправданно.
                memcheck_results_table[ # NOQA WPS529
                    preset_name][memcheck_results_key] = memcheck_output
            else:
                memcheck_results_table.pop(preset_name, None)

    except DockerException as exc:
        tests_errors_table['docker_tests'] = exc
        if tests_fail_out != b'':
            tests_errors_table[
                'docker_tests'
            ] = tests_fail_out.decode("utf-8")
            container_result = False

        if memcheck_output != '' and preset_name in memcheck_results_table:
            memcheck_results_table[
                preset_name][memcheck_results_key] = memcheck_output

            container_result = False

    return container_result


def run_docker_test(image_name: str, image_tags: str):
    """
    Функция выполняет сборку и запуск docker контейнеров,
    вызывается из внешних скриптов.
    :param image_name: Название образа для сборки и запуска.
    :param image_tags: Тег образа для сборки и запуска.
    :return: Возвращает результат выполнения операций сборки и запуска.
    """
    print(f'{BLUE}{BOLD}Removing "Dangling" images...{END_COLOR}')
    test_result = True
    dangling_remove_res = subprocess.run(
        [os.path.abspath('./docker/remove_dangling_images.sh')],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        shell=True
    )
    if dangling_remove_res.returncode == 0:
        print(f'{BLUE}{BOLD}BUILDING docker image, process may take a long '
              f'time...{END_COLOR}')

        test_result = _build_docker_image(image_tags)
        if test_result:
            test_result = _run_docker_container(image_name, image_tags)

    return test_result
