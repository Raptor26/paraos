echo "CTest with clang:"
ctest --test-dir ./build/pc_debug_clang -j8 --timeout 15 --repeat-until-fail 2 --schedule-random --output-on-failure

echo "Memory check tests:"
sh ./memcheck.sh
