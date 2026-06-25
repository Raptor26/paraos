# AGENTS.md — PARAOS

> Руководство для агентов, работающих с проектом PARAOS. Этот файл находится в корне репозитория. Ранее он писался на английском — языке README, большинства заголовочных документов и комментариев в исходном коде. В некоторых Python-скриптах сборки встречаются комментарии и строки на русском.

## Обзор проекта

**PARAOS** — это слой абстракции операционной системы (OSAL) на C++, поставляемый только в виде исходного кода и ориентированный на встраиваемые системы. Он оборачивает примитивы Windows, Linux и FreeRTOS так, чтобы один и тот же прикладной код мог нативно запускаться на ПК для тестирования и на микроконтроллере в продакшене.

- **Лицензия:** MIT (`LICENSE.txt`)
- **Текущая версия:** считывается CMake из `.cz.json` (на момент изучения — `0.13.0`)
- **Язык репозитория:** английский для документации и большинства комментариев в коде; русский встречается в некоторых скриптах сборки
- **Модель распространения:** нет шага установки — проект подключается как поддиректория CMake

Библиотека собирается как единая статическая CMake-цель `paraos` (псевдоним `paraos::paraos`). Платформенно-зависимые реализации находятся в директориях `port_*` и выбираются автоматически корневым `CMakeLists.txt`.

## Стек технологий

- **Языки:** C++17, C11
- **Система сборки:** CMake ≥ 3.20 (CMakePresets.json требует ≥ 3.28)
- **Генераторы:** предустановки рассчитаны на Ninja
- **Поддерживаемые компиляторы/тулчейны:** Clang, GCC, MSVC (Windows), кросс-компиляция для FreeRTOS
- **Фреймворки тестирования:** GoogleTest, CTest, опционально Google benchmark для бенчмарков контейнеров
- **Статический анализ:** clang-tidy, модули cppcheck в `cmake/`
- **Зависимости контейнеров/статического анализа:** Boost.LEAF, ETL, Microsoft GSL, LwRB, FreeRTOS-Kernel
- **Python-инструменты:** `builder.py` (интерактивный/неинтерактивный запуск тестов), `pybuilder/pycmakebuilder.py` (CI-драйвер)
- **Версионирование/стиль коммитов:** Commitizen / Conventional Commits (`.cz.json`)

## Структура репозитория

| Путь | Назначение |
| ------ | ------------ |
| `CMakeLists.txt` | Корневое определение проекта; выбирает порт, включает тесты, связывает зависимости |
| `setup.cmake` | Определяет интерфейсную цель `paraos_setup`, используемую для распространения флагов сборки и путей включения |
| `CMakePresets.json` | Предустановки CMake для PC и FreeRTOS сборок с Clang/GCC, Debug/Release, трассировкой, clang-tidy, полиморфным extra |
| `.cz.json` | Конфигурация Commitizen; версия CMake-проекта считывается отсюда |
| `paraos_*.hpp` / `paraos_*.h` | Кроссплатформенные core-заголовки (базовые классы, исключения, конфигурация, трассировка, ISR-хелперы, атомарный bool, профилировщик времени выполнения и т.д.) |
| `port_unix/` | Реализация POSIX/Linux: потоки, мьютексы, семафоры, таймеры, критические секции, UDP-сокеты, время, утилиты |
| `port_win/` | Реализация WinAPI: потоки, мьютексы, семафоры, таймеры, критические секции, UDP-сокеты, время |
| `port_freertos/` | Реализация FreeRTOS плюс встроенные исходники `FreeRTOS-Kernel/` |
| `containers/` | Буферы сообщений, блокирующие очереди, кольцевые буферы, мульти-кольцевые буферы |
| `extra/` | Высокоуровневые хелперы: статусный LED, one-shot executor, кооперативный планировщик, последовательность потоков |
| `port_tests/` | Тесты и примеры уровня OSAL (мьютекс, семафор, жизненный цикл потока, таймер, ISR, критическая секция, версия, UDP-сокет) |
| `containers/tests/` | Модульные и стресс-тесты контейнеров |
| `extra/tests/` | Тесты хелперов extra |
| `port_unix/tests/` | Порт-специфичные утилитарные тесты |
| `cmake/` | Общие модули CMake (покрытие кода, хелперы cppcheck, профилирование и т.д.) |
| `leaf/`, `etl/`, `GSL/`, `lwrb/` | Сторонние зависимости, импортированные как git-subrepos (у каждой есть файл `.gitrepo`) |
| `pybuilder/` | Python-автоматизация сборки/тестов и манифесты зависимостей |
| `docker/` | Хелперы для сборки и запуска Docker-образа |
| `Dockerfile` | Многостадийный образ для CI-стиля сборки и тестов |
| `.gitlab-ci.yml` | Описание пайплайнов GitLab CI |

## Детали системы сборки

### Основные CMake-цели

- `paraos` — статическая библиотека, содержащая core + выбранный порт + containers + extra.
- `paraos::paraos` — псевдоним для указанной выше цели.
- `paraos_setup` — интерфейсная цель, определённая в `setup.cmake`; используется для распространения определений компиляции, таких как `paraosTRACE_ENABLE`, и директорий включения (например, `etl_profile.h`).
- `etl::etl`, `Microsoft.GSL::GSL`, `Boost::leaf`, `lwrb_ex` — псевдонимы зависимостей, подключаемые как поддиректории, если они ещё не определены родительским проектом.

### Выбор платформы

Корневой `CMakeLists.txt` выбирает порт на основе переменных CMake:

- `RTOS_NAME STREQUAL FREERTOS` → `port_freertos/`, определяет `PARAOS_LIKE_FREERTOS`
- `WIN32` → `port_win/`, определяет `PARAOS_LIKE_WINAPI`
- `UNIX` → `port_unix/`, определяет `PARAOS_LIKE_UNIX`

### Важные опции/флаги CMake

| Переменная | Эффект |
| ---------- | -------- |
| `CMAKE_BUILD_TYPE` | `Debug` включает `PARAOS_CHECK_LOOP_ENABLE` и автоматически включает тесты в standalone-режиме |
| `RTOS_NAME=FREERTOS` | Сборка FreeRTOS-порта |
| `FREERTOS_PORT` | Выбор порта FreeRTOS, например `GCC_ARM_CM4F` |
| `FREERTOS_HEAP` | Номер реализации кучи FreeRTOS |
| `FREERTOS_USER_CONFIG` | Использовать предоставленную пользователем цель `freertos_config` вместо встроенной |
| `TRACE=true` | Включает debug-макросы `paraosTRACE_ENABLE` |
| `PARAOS_USING_POLYMORPHIC_EXTRA=true` | Добавляет `virtual` к хелперам extra для возможности мокирования |
| `CLANG_TIDY_ENABLE=true` | Запускает clang-tidy для основной библиотеки и тестовых целей |
| `PARAOS_TESTS` | Включает тестовые поддиректории (по умолчанию ON в Debug standalone) |
| `CODE_COVERAGE` | Включает покрытие кода GCC через `cmake/CodeCoverage.cmake` |
| `IS_PARAOS_STAND_ALONE_PROJECT` | По умолчанию ON; при OFF проект ведёт себя как зависимость и не навязывает настройки тестов/debug |

### Генерируемые файлы

- `paraos_version.hpp` генерируется из `paraos_version.hpp.in` через `configure_file`. Он добавлен в `.gitignore` — не редактируйте его вручную.

## Команды сборки и тестирования

### Использование предустановок CMake (рекомендуется)

```bash
# Список предустановок
cmake --list-presets

# Конфигурация
cmake --preset pc_debug_clang

# Сборка
cmake --build build/pc_debug_clang/

# Запуск тестов
ctest --test-dir build/pc_debug_clang/
```

Полезные флаги `ctest`, используемые в CI:

```bash
ctest --test-dir build/<preset> \
  --output-on-failure \
  --stop-on-failure \
  --schedule-random \
  --timeout 20
```

Стресс-тесты выбираются по метке:

```bash
ctest --test-dir build/<preset> -L stress --repeat-until-fail 150 --timeout 20
```

### Использование Python-builder

```bash
# Сначала установите Python-зависимости (опционально, если нужен полный builder)
cd pybuilder
python3 install_builder_dependencies.py

# Интерактивное меню
cd ..
python3 builder.py

# Примеры неинтерактивного запуска
python3 builder.py -a 1   # все сборки и тесты
python3 builder.py -a 2   # все сборки и стресс-тесты
python3 builder.py -a 3   # только пресет pc_debug_clang
python3 builder.py -a 5   # стресс-тесты GCC
python3 builder.py -a 11  # запуск Docker entrypoint для одиночных тестов
```

`pybuilder/pycmakebuilder.py` — низкоуровневый драйвер, используемый GitLab CI; он фильтрует пресеты по include/exclude регулярным выражениям и пробрасывает опции `ctest`.

### Сборка и тесты в Docker

`Dockerfile` собирает несколько пресетов в многостадийном образе. Хелперы находятся в `docker/`:

- `docker/dbuild.sh` — сборка образа
- `docker/drunner.sh` — интерактивный запуск образа
- `docker/run_docker_tests.sh` — сборка + запуск

Корневые entrypoint-скрипты копируются `builder.py` во временный `docker_tests_entrypoint.sh` перед запуском контейнера:

- `docker_tests_entrypoint_single.sh`
- `docker_tests_entrypoint_stress.sh`
- `docker_tests_entrypoint_memcheck.sh`

> **Примечание:** Docker entrypoint-скрипты ссылаются на `build/pc_debug_clang_docker/`, но `CMakePresets.json` сейчас определяет только `pc_debug_clang` (без суффикса `_docker`). Убедитесь, что существует подходящий пресет, прежде чем полагаться на эти Docker-пути в неизменном виде.

### Проверка памяти

Valgrind memcheck запускается через CTest:

```bash
ctest --test-dir build/pc_debug_gcc -T memcheck -j4
```

`memcheck.sh` автоматизирует это для пресетов FreeRTOS и `pc_debug_clang_docker`.

## Сводка по предустановкам CMake

Проект определяет пресеты, сгруппированные следующим образом:

- **PC Clang:** `pc_debug_clang`, `pc_debug_clang_trace`, `pc_debug_clang_polymorphic`, `pc_release_clang`
- **PC GCC:** `pc_debug_gcc`, `pc_debug_gcc_trace`, `pc_debug_gcc_clang_tidy`
- **FreeRTOS Clang:** `freertos_debug_clang`, `freertos_debug_clang_trace`, `freertos_release_clang`
- **FreeRTOS GCC:** `freertos_debug_gcc`, `freertos_debug_gcc_trace`, `freertos_debug_gcc_clang_tidy`

`pc_debug_clang` и `pc_debug_gcc` — стандартные отправные точки для разработки под Linux/PC.

## Стратегия тестирования

- **Модульные тесты:** исполняемые файлы на базе GoogleTest в `port_tests/`, `containers/tests/`, `extra/tests/` и `port_unix/tests/`.
- **Автономные исполняемые файлы:** Некоторые файлы в `port_tests/` компилируются как запускаемые примеры, но не регистрируются как тесты CTest (например, `example_timer`, `example_thread_check_timeout`, `example_socket_udp`).
- **Стресс-тесты:** Многопоточные тесты контейнеров и extra помечены меткой CTest `stress` и многократно запускаются в CI.
- **Статический анализ:** clang-tidy запускается в пресетах `*_clang_tidy` и трактует предупреждения как ошибки.
- **Корректность работы с памятью:** Valgrind memcheck выполняется в CI для PC-пресета GCC и через Docker/FreeRTOS.
- **Покрытие кода:** Покрытие GCC можно включить флагом `CODE_COVERAGE=true` и получить отчёт через CTest (`-T Test -T Coverage`).

## Стиль кода и соглашения

### Форматирование

- C/C++: `clang-format` с `.clang-format`:
  - `BasedOnStyle: Google`
  - `AlignAfterOpenBracket: AlwaysBreak`
- Python: `ruff format` (настроен в `pybuilder/.pre-commit-config.yaml`)
- Markdown: `markdownlint` с отключённым правилом `MD013`

### Статический анализ

`.clang-tidy` включает широкий набор проверок:

```yaml
clang-analyzer-*, google-*, hicpp-*, llvm-*, misc-*, modernize-*,
performance-*, portability-*, readability-*
```

с рядом подавлений (например, `modernize-avoid-c-arrays`, `llvm-header-guard`, `readability-implicit-bool-conversion`) и `WarningsAsErrors: '*'`. Фильтр заголовков применяется к `*.hpp`.

### Соглашения по коду

- Все публичные символы находятся в namespace `paraos`.
- Публичные типы, классы, методы и свободные функции используют `snake_case`.
- Абстрактные интерфейсы именуются с суффиксом `_base` (например, `profiler_base`, `serial_base`).
- Значения `enum class` записываются в нижнем регистре без префикса `k` (например, `thread_priority::normal`, `status_led_mode::blink`).
- Публичные заголовки используют префикс `paraos_`.
- Защитные макросы включения используют формат `PARAOS_<NAME>_HPP`/`_H`.
- Файлы используют Doxygen-комментарии `///` с `@file`, `@brief`, `@param`, `@return`.
- Предпочитается RAII (например, `std::scoped_lock<paraos::mutex>`, `paraos::critical_section`).
- Старые публичные имена сохраняются как устаревшие алиасы с помощью `PARAOS_DEPRECATED` в течение одного релиза; виртуальные методы переименовываются без форвардеров.
- «Правило пяти» явно документируется в классах; copy/move удаляются, если не нужны.
- Делегаты — `etl::delegate<void()>`.
- Платформенно-специфичные макросы: `PARAOS_LIKE_UNIX`, `PARAOS_LIKE_WINAPI`, `PARAOS_LIKE_FREERTOS`.
- Debug-проверки: `PARAOS_CHECK_ASSERT` / `PARAOS_CHECK_LOOP` из `paraos_check.h` включаются в standalone Debug-сборках.
- Макросы трассировки: `paraosTRACE_MESSAGE`, `paraosOUT` и др., управляемые `paraosTRACE_ENABLE`.
- Макросы `PARAOS_ATTR_*` в `paraos_attr.h` абстрагируют атрибуты GCC/Clang/MSVC.
- Обработка ошибок использует `paraos::exception`, производный от `std::exception` и `etl::exception`.

### Соглашения по версиям и CHANGELOG

- Версия хранится в `.cz.json` по пути `commitizen.version`.
- Коммиты следуют Conventional Commits (feat, fix, refactor, perf и т.д.).
- `CHANGELOG.md` поддерживается при бампах Commitizen.

## Инструкции по подготовке коммитов

Перед каждым коммитом агент должен следовать приведённой ниже инструкции.

1. Выполни анализ полученных изменений, ответь для себя на вопрос зачем эти изменения были сделаны.
2. Проверь, не нарушена ли обратная совместимость, если не уверен, спроси у человека. Если совместимость не нарушена, не отражай это в сообщениях. Если нарушена — используй BREAKING CHANGE.
3. Все фиксации в git оформляются на русском языке. Следуй стилю Commitizen, используя безличные краткие причастия (страдательный залог) в прошедшем времени. Пример правильного формата: `refactor(scope): заменены X на Y`. Неправильно: `refactor(scope): заменил X на Y`. Сделай commit с помощью команды ниже:

```bash
git commit -m "ваше сообщение"
```

## Дополнительные требования

1. В ответе человеку напиши что работа выполнена, не дублируй информацию которую ты уже сделал.
2. Форматирование списка изменений: если в описании коммита (или теле сообщения) перечисляется несколько независимых изменений, каждый пункт должен начинаться с маркера списка (дефис `-` или звёздочка `*`) и отделяться от заголовка пустой строкой. Запрещено писать перечисление сплошным абзацем без маркеров.

**Пример:**

```bash
fix(radio_message_parser): исправлено имя функции RPM_WriteCrcInMessageTail → RMP_WriteCrcInMessageTail

- Исправлена опечатка в имени функции записи CRC во всех местах использования.
- Добавлены проверки валидности параметров в конструкторе RMP_Ctor.
- Исправлено сравнение счётчика прочитанных байт в RMP_FindFirstByte.
- Удалены неиспользуемые переменные и платформенно-зависимые заголовки.
- Расширено покрытие unit-тестами: граничные случаи конструктора,
  обработка пустого буфера, разделённые сообщения, мусор между сообщениями,
  переполнение буфера, порог чтения, ложный старт.
```

**Контр-индикатор (что делать НЕ надо):**

```bash
...заголовок...

Исправлена опечатка... Добавлены проверки... Исправлено сравнение...
Удалены неиспользуемые... Расширено покрытие...
```

3. При наличии двух и более логически самостоятельных изменений в одном коммите, оформляй их в виде маркированного списка. Каждый пункт — одно законченное изменение, начинающееся с краткого страдательного причастия. Между заголовком и списком обязательна пустая строка. Не объединяй пункты в сплошной текст.

## CI/CD и развёртывание

Проект тестируется в GitLab CI на раннерах Windows и Linux. Этапы включают:

- `test_windows` / `test_linux` — сборка и запуск всех не-FreeRTOS пресетов
- `test_windows_freertos` / `test_linux_freertos` — FreeRTOS-пресеты, запуск в один поток (`-j1`)
- `test_windows_stress` / `test_linux_stress` — стресс-тесты, повторяемые 120–300 раз
- `test_windows_stress_freertos` / `test_linux_stress_freertos` — стресс-тесты FreeRTOS
- `test_linux_valgrind` — Valgrind memcheck для `pc_debug_gcc`

CI использует `pybuilder/pycmakebuilder.py` с include/exclude регулярными выражениями для выбора подмножеств пресетов и передаёт опции `ctest`, такие как `--output-on-failure`, `--stop-on-failure`, `--schedule-random`, `--timeout 20` и `--repeat-until-fail`.

В традиционном смысле артефакта развёртывания нет: PARAOS поставляется как исходный код. Потребители подключают его через `add_subdirectory(paraos)` и линкуют `paraos::paraos`.

## Аспекты безопасности

- **Поставка только исходников:** нет пресобранных бинарников; флаги сборки видны в CMake.
- **Строгие предупреждения:** все цели компилируются с `-Wall -Wextra -Wpedantic -Werror`.
- **Статический анализ:** интегрированы clang-tidy и модули cppcheck; CI падает на предупреждениях tidy.
- **Runtime-проверки:** standalone Debug-сборки включают `PARAOS_CHECK_LOOP_ENABLE`, предоставляя `PARAOS_CHECK_ASSERT`/`PARAOS_CHECK_LOOP` для обнаружения некорректных состояний.
- **Безопасность памяти:** Valgrind memcheck запускается в CI; RAII-обёртки снижают риск утечек ресурсов.
- **ISR-безопасность:** `paraos::isr_bool` и `paraos::var_atomic` спроектированы для контекстов прерываний/сервисных вызовов; где уместно, используются ISR-варианты примитивов FreeRTOS.
- **Отсутствие требования динамического выделения памяти в core-примитивах:** большинство абстракций принимают предоставленные пользователем стеки/делегаты, хотя некоторый тестовый/примерный код использует стандартные контейнеры.

## Платформенные замечания и типичные подводные камни

- **macOS:** `port_unix` опирается на POSIX-таймеры (`timer_create`, `timer_delete`, `itimerspec`, `timer_t`), которые недоступны на macOS. Для нативных сборок используйте Linux или Windows.
- **FreeRTOS на PC:** FreeRTOS-порт использует `GCC_POSIX` на Linux и `MSVC_MINGW` на Windows, если не задан `FREERTOS_USER_CONFIG`.
- **ETL-профиль:** каждый порт предоставляет свой `config/etl_profile.h`; в standalone-проекте директория конфигурации активного порта добавляется в пути включения цели `etl`.
- **Не редактируйте:** `paraos_version.hpp` генерируется; `etl/`, `leaf/`, `GSL/`, `lwrb/` содержат файлы `.gitrepo`, управляемые git-subrepo.
- **Таймауты тестов:** CI и Docker-скрипты используют таймаут 15–20 секунд на тест; тяжёлые стресс-прогоны могут требовать больше времени на более медленных машинах.
