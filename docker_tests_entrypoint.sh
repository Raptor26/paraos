# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Vyhodcev Egor

#!/bin/bash

RED=$(tput setaf 1)
NC=$(tput sgr0)

pc_stress_test="ctest --test-dir ./build/pc_debug_clang_docker \
    -j8 \
    --timeout 15 \
    --repeat-until-fail 1000 \
    --schedule-random \
    --output-on-failure \
    --stop-on-failure"

freertos_stress_test="ctest --test-dir ./build/freertos_debug_clang \
    -j8 \
    --timeout 15 \
    --repeat-until-fail 1000 \
    --schedule-random \
    --output-on-failure \
    --stop-on-failure"

echo "Choose needed action:"

echo "  0 - Exit"
echo "  1 - Run normal tests + memcheck"
echo "  2 - Run stress tests (1000 repetitions) + memcheck"

read action # Чтение переменной из стандартного ввода

case $action in
0)
    exit 0;;
1)
    echo "NORMAL CTest with clang:"
    ctest --test-dir ./build/pc_debug_clang_docker -j8 --timeout 15 --repeat-until-fail 2 --schedule-random --output-on-failure

    echo "NORMAL freeRTOS test with clang:"
    ctest --test-dir ./build/freertos_debug_clang -j8 --timeout 15 --repeat-until-fail 2 --schedule-random --output-on-failure

    echo "Memory check tests:"
    sh ./memcheck.sh
    ;;

2)
    echo "STRESS CTest with clang:"
    if eval "$pc_stress_test"; then
        echo "STRESS freeRTOS test with clang:"
        if eval "$freertos_stress_test"; then
            echo "Memory check tests:"
            sh ./memcheck.sh
        else
            echo "${RED}ERROR: Стресс тест pc_debug_clang_docker завершился неуспешно!"
        fi
    else
        echo "${RED}ERROR: Стресс тест freertos_debug_clang завершился неуспешно!"
    fi;;

*) # Если с клавиатуры введено действие, не описанное конструкцией case, то будет выведено сообщение ниже
    echo "This action is not supported!"

esac # Конец оператора case.
