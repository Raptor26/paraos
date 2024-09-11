# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Vyhodcev Egor

#!/bin/bash

echo "CTest with clang:"
ctest --test-dir ./build/pc_debug_clang_docker -j8 --timeout 15 --repeat-until-fail 2 --schedule-random --output-on-failure

echo "freeRTOS test with clang:"
ctest --test-dir ./build/freertos_debug_clang -j8 --timeout 15 --repeat-until-fail 2 --schedule-random --output-on-failure

echo "Memory check tests:"
sh ./memcheck.sh
