#!/usr/bin/env bash
set -uo pipefail

# Phase 3: Preset Validation runner
# Configures, builds, and tests every CMake preset on this macOS machine.
# Clang presets are forced to /usr/bin/clang and /usr/bin/clang++ because the
# default clang in PATH targets Linux (aarch64-unknown-linux-gnu).

PRESETS=$(cmake --list-presets 2>/dev/null | grep -oE '"[^"]+"' | tr -d '"')
RESULT_DIR=".planning/phases/03-preset-validation/results"
mkdir -p "$RESULT_DIR"
SUMMARY="$RESULT_DIR/summary.tsv"

echo -e "Preset\tConfigure\tBuild\tTest\tNotes" > "$SUMMARY"

for preset in $PRESETS; do
    echo "============================================"
    echo "Preset: $preset"
    echo "============================================"

    build_dir="build/$preset"
    log="$RESULT_DIR/$preset.log"
    rm -rf "$build_dir"

    configure_status=0
    build_status=0
    test_status=0
    notes=""

    # Clang presets: override compilers to AppleClang.
    if [[ "$preset" == *clang* ]]; then
        extra_args="-D CMAKE_C_COMPILER=/usr/bin/clang -D CMAKE_CXX_COMPILER=/usr/bin/clang++"
    else
        extra_args=""
    fi

    {
        echo "=== Preset: $preset ==="
        echo "=== Configure ==="
        cmake --preset "$preset" $extra_args 2>&1
        configure_status=$?
        echo "Configure exit: $configure_status"

        if [ $configure_status -eq 0 ]; then
            echo "=== Build ==="
            cmake --build "$build_dir" 2>&1
            build_status=$?
            echo "Build exit: $build_status"
        fi

        if [ $configure_status -eq 0 ] && [ $build_status -eq 0 ]; then
            echo "=== Test (timeout 20s per test) ==="
            # Hard-stop the whole ctest run if it takes longer than 5 minutes
            # to protect against hung stress tests.
            ctest --test-dir "$build_dir" \
                --output-on-failure \
                --stop-on-failure \
                --schedule-random \
                --timeout 20 \
                -j4 2>&1
            test_status=$?
            echo "Test exit: $test_status"
        fi
    } > "$log" 2>&1

    if [ $configure_status -ne 0 ]; then
        notes="configure failed"
    elif [ $build_status -ne 0 ]; then
        notes="build failed"
    elif [ $test_status -ne 0 ]; then
        notes="tests failed"
    else
        notes="ok"
    fi

    echo -e "$preset\t$configure_status\t$build_status\t$test_status\t$notes" >> "$SUMMARY"
    echo "Result: configure=$configure_status build=$build_status test=$test_status ($notes)"
    echo "Log: $log"
    echo ""
done

echo "=== Summary ==="
cat "$SUMMARY"
