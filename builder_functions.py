import os
import sys
import subprocess

try:
    from python_on_whales import docker, DockerException
except (ImportError, DockerException) as e:
    print(
        f'Произошла ошибка ImportError: {e[0]}. '
        'Возможно, зависимости скрипта builder.py не установлены, '
        'попробуйте запустить файл-установщик: '
        '"python install_builder_dependencies.py"'
    )

tests_errors_table = {
    'pc_debug_clang': '',
    'pc_release_clang': '',
    'freertos_debug_clang': '',
    'freertos_release_clang': '',
    'docker_tests': ''
}

# Таблица с результатами memcheck:
# 'preset_name': {
#     'defects': '',
#     'memcheck_results': ''
# }
memcheck_results_table = {}

OK_GREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
END_COLOR = '\033[0m'


def _make_preset(make_command: list[str]):
    subprocess.run(
        make_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )


def _build_preset(build_command: list[str]):
    build_res = subprocess.run(
        build_command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    return build_res


def test_preset(
        make_command: list[str],
        build_command: list[str],
        test_dir: str
):
    preset_name = make_command[2]
    print(f'Фаза Make {preset_name}')
    _make_preset(make_command)

    print(f'Фаза сборки {preset_name}')
    build_res = _build_preset(build_command)
    if not build_res:
        print(
            f'{FAIL}Произошла ошибка при сборке '
            f'{preset_name}! {build_res.stderr}{END_COLOR}'
        )
        return False

    print(f'Фаза тестирования {preset_name}')
    test_res = subprocess.run(
        [
            'ctest',
            '--test-dir',
            test_dir,
            '-j8',
            '--timeout', '15',
            '--repeat-until-fail', '2',
            '--schedule-random',
            '--output-on-failure'
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    print(test_res.stdout)

    if test_res != 0 and test_res.stderr:
        failed_test_output = test_res.stdout.split('FAILED:')
        tests_errors_table[preset_name] = failed_test_output[1]

        return False

    return True


def stress_test_preset(
        make_command: list[str],
        build_command: list[str],
        test_dir: str
):
    out_string = ''
    preset_name = make_command[2]
    print(f'Фаза Make {preset_name}')
    _make_preset(make_command)

    print(f'Фаза сборки {preset_name}')
    build_res = _build_preset(build_command)
    if not build_res:
        print(
            f'{FAIL}Произошла ошибка при сборке '
            f'{preset_name}! {build_res.stderr}{END_COLOR}'
        )
        return False

    print(f'Фаза стресс-тестирования {preset_name}')
    test_process = subprocess.Popen(
        [
            'ctest',
            '--test-dir',
            test_dir,
            '-j16',
            '--timeout', '15',
            '--repeat-until-fail', '555',
            '--stop-on-failure',
            '--output-on-failure'
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    found_failed_tests = False
    while True:
        out = test_process.stdout.read(10)
        if test_process.poll() is not None:
            break
        if out != '':
            if out.find('FAILED:'):
                found_failed_tests = True
            sys.stdout.write(out)
            sys.stdout.flush()

            if found_failed_tests:
                out_string += out

    if test_process.returncode != 0:
        print(test_process.stderr.readline())
        failed_test_output = out_string.split('FAILED:')
        tests_errors_table[preset_name] = failed_test_output[1]

        return False

    return True


def _build_docker_image():
    failed_flag = False
    fail_output = ''
    try:
        build_output_generator = docker.build(
            context_path='.',
            tags='docker_test_paraos:1.0',
            stream_logs=True
        )

        for stream_content in build_output_generator:
            print({stream_content})
            if stream_content.find('FAILED:') != -1:
                failed_flag = True
            if failed_flag:
                fail_output += stream_content

        return True

    except DockerException as e:
        tests_errors_table['docker_tests'] = fail_output
        return False


def _run_docker_container():
    result = True
    memcheck_flag = False
    tests_fail_flag = False
    tests_fail_out = b''
    memcheck_output = ''
    memcheck_results_flag = False
    preset_name = 'preset'
    try:
        memcheck_results_table.clear()
        run_output_generator = docker.run(
            name='docker_test_paraos',
            image='docker_test_paraos:1.0',
            stream=True,
            remove=True,
            privileged=True
        )

        for _, stream_content in run_output_generator:
            print(stream_content)
            if (stream_content.find(b'The following tests FAILED:') != -1
                    and not memcheck_flag):
                tests_fail_flag = True
            if stream_content.find(b'Errors while running CTest') != -1:
                tests_fail_flag = False
                stream_content = b''
            if stream_content.find(
                    b'-- Processing memory checking output:') != -1:
                tests_fail_flag = False
            if stream_content.find(b'[TEST START]') != -1:
                tests_fail_flag = False
                tests_fail_out += b'\n' + stream_content + b'\n'

            if tests_fail_flag:
                tests_fail_out += stream_content

            if stream_content.find(
                    b'-- Processing memory checking output:') != -1:
                memcheck_flag = True
            if stream_content.find(
                    b'Memory checking results:') != -1:
                memcheck_flag = True
                memcheck_results_table[preset_name][
                    'defects'] = memcheck_output
                memcheck_output = ''
                memcheck_results_flag = True
                stream_content = b''

            elif stream_content.find(
                    b'[MEMCHECK START]') != -1:
                if preset_name != 'preset':
                    memcheck_results_table[preset_name][
                        'memcheck_results'] = memcheck_output
                    memcheck_results_flag = False
                    memcheck_output = ''

                preset_name = stream_content.split(b'[MEMCHECK START] ')[1]
                preset_name = preset_name.decode('utf-8')
                preset_name = preset_name.strip()
                memcheck_results_table.update(
                    {
                        preset_name: {
                            'memcheck_results': '',
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
                if memcheck_results_flag:
                    if line.find('Potential') == -1:
                        memcheck_output += FAIL + line + WARNING
                    else:
                        memcheck_output += line
                else:
                    memcheck_output += line

        if memcheck_output != '':
            memcheck_results_table[
                preset_name]['memcheck_results'] = memcheck_output

    except DockerException as e:
        tests_errors_table['docker_tests'] = e
        if tests_fail_out != '':
            tests_errors_table[
                'docker_tests'
            ] = tests_fail_out.decode("utf-8")
            result = False

        if memcheck_output != '':
            memcheck_results_table[
                preset_name]['memcheck_results'] = memcheck_output

            result = False

    return result


def run_docker_test():
    print('Удаление "Dangling" образов...')
    result = True
    dangling_remove_res = subprocess.run(
        [os.path.abspath('./docker/remove_dangling_images.sh')],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        shell=True
    )
    if dangling_remove_res.returncode == 0:
        print('Сборка Docker образа, процесс может занять длительное время...')

        result = _build_docker_image()
        if result:
            result = _run_docker_container()

    return result
