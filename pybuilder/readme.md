# Запуск

```bash
python pybuilder/pycmakebuilder.py -d="${PWD}" --ctest-options="--output-on-failure --stop-on-failure -j16" -e="pc_gcc_debug_native_trace"
```

# Выбор всех доступных пресетов

Чтобы указать в качестве одного из аргументов, что необходимо использовать 
все пресеты, нужно передать в качестве значения аргумента `""`, пример 
представлен ниже:
```shell
python -m pycmakebuilder -d="./cpp" --build-only="clang*" --include="" --exclude="trace|simulation"
```

# Pre-commit hooks

Для использования pre-commit hooks необходимо установить `pre-commit`:
```shell
pip install pre-commit
```

Далее необходимо создать файл конфигурации `.pre-commit-config.yaml`

Затем, необходимо установить описанные в конфигурационном файле скрипты:
```shell
pre-commit install
```

После установки скрипты будут запускаться каждый раз при совершении коммита.
