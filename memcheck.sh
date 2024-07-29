#!/bin/bash

cmake --preset pc_debug_clang \
&& cmake --build build/pc_debug_clang/ \
&& ctest -T memcheck --test-dir build/pc_debug_clang -j16 --timeout 15 --output-on-failure

