# Copyright (c) 2024 Stilsoft
# Distributed under the MIT License
# Author Mickle Isaev

#!/bin/bash

cmake --preset pc_debug_clang_docker \
&& cmake --build build/pc_debug_clang_docker/ \
&& ctest -T memcheck --test-dir build/pc_debug_clang_docker -j16 --timeout 15 --output-on-failure

