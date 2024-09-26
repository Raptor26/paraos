#!/bin/bash

GREEN=$(tput setaf 2)
RED=$(tput setaf 1)
NC=$(tput sgr0)


pc_debug_clang="cmake --preset pc_debug_clang \
    && cmake --build build/pc_debug_clang/ \
    && ctest --test-dir build/pc_debug_clang \
        -j8 \
        --timeout 15 \
        --repeat-until-fail 2 \
        --schedule-random \
        --output-on-failure"

pc_release_clang="cmake --preset pc_release_clang \
    && cmake --build build/pc_release_clang/ \
    && ctest --test-dir build/pc_release_clang \
        -j8 \
        --timeout 15 \
        --repeat-until-fail 2 \
        --schedule-random \
        --output-on-failure"

pc_debug_clang_stress="cmake --preset pc_debug_clang \
    && cmake --build build/pc_debug_clang/ \
    && ctest --test-dir build/pc_debug_clang \
        -j16 \
        --timeout 15 \
        --repeat-until-fail 1000 \
        --schedule-random \
        --output-on-failure"

freertos_debug_clang="cmake --preset freertos_debug_clang \
    && cmake --build build/freertos_debug_clang/ \
    && ctest --test-dir build/freertos_debug_clang \
        -j8 \
        --timeout 15 \
        --repeat-until-fail 2 \
        --schedule-random \
        --output-on-failure"

freertos_release_clang="cmake --preset freertos_release_clang \
    && cmake --build build/freertos_release_clang/ \
    && ctest --test-dir build/freertos_release_clang \
        -j8 \
        --timeout 15 \
        --repeat-until-fail 2 \
        --schedule-random \
        --output-on-failure"

freertos_debug_clang_stress="cmake --preset freertos_debug_clang \
    && cmake --build build/freertos_debug_clang/ \
    && ctest --test-dir build/freertos_debug_clang \
        -j16 \
        --timeout 15 \
        --repeat-until-fail 1000 \
        --schedule-random \
        --output-on-failure"

echo "Выберите необходимое действие"

echo "  0 - Выход"
echo "  1 - Запустить все сборки и тесты"
echo "  2 - Запустить все сборки и тесты с большим количеством повторений"
echo "  3 - pc_debug_clang"
echo "  4 - freertos_debug_clang"
echo "  12 - run tests in docker"

read doing # Чтение переменной из стандартного ввода


case $doing in
0)
    exit 0;;
1)
    if eval "$pc_debug_clang" \
        && eval "$pc_release_clang" \
        && eval "$freertos_debug_clang"\
        && eval "$freertos_release_clang"; then
        echo "${GREEN}SUCCESS: Все тесты выполнены успешно${NC}"
    else
        echo "${RED}ERROR${NC}: Обнаружена ошибка в сборке или выполнении тестов"
    fi;;

2)
    if eval "$pc_debug_clang_stress" && eval "$freertos_debug_clang_stress"; then
        echo "${GREEN}SUCCESS: Все тесты выполнены успешно${NC}"
    else
        echo "${RED}ERROR${NC}: Обнаружена ошибка в сборке или выполнении тестов"
    fi;;
3)
    if eval $pc_debug_clang; then 
        echo "${GREEN}SUCCESS: Все тесты выполнены успешно${NC}"
    else
        echo "${RED}ERROR${NC}: Обнаружена ошибка в сборке или выполнении тестов"
    fi;;
4)
    if eval $freertos_debug_clang; then 
        echo "${GREEN}SUCCESS: Все тесты выполнены успешно${NC}"
    else
        echo "${RED}ERROR${NC}: Обнаружена ошибка в сборке или выполнении тестов"
    fi;;
12)
    cd docker
    ./run_docker_tests.sh;;

*) # Если с клавиатуры введено действие, не описанное конструкцией case, то будет выведено сообщение ниже
    echo "Указанное действие не поддерживается"

esac # Конец оператора case.
# ------------------------------------------------------------------------------

read  -p  "Нажмите клавишу [Enter] чтобы завершить выполнение скрипта"