# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Mickle Isaev

#!/bin/bash

echo "[MEMCHECK START] Memcheck freeRTOS"
cmake --preset freertos_debug_clang \
    && cmake --build build/freertos_debug_clang/ \
    && ctest -T memcheck --test-dir build/freertos_debug_clang -j16 --timeout 15 --output-on-failure

echo "[MEMCHECK START] Memcheck unix"
cmake --preset pc_debug_clang_docker \
    && cmake --build build/pc_debug_clang_docker/ \
    && ctest -T memcheck --test-dir build/pc_debug_clang_docker -j16 --timeout 15 --output-on-failure
