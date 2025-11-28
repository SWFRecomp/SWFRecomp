#!/bin/bash
# Build all tests as native executables
# Usage: ./scripts/build_all_native.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWFRECOMP_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Load exclude list from config file
EXCLUDE_CONFIG="${SCRIPT_DIR}/excluded_tests.conf"
EXCLUDE_TESTS=()

if [ -f "$EXCLUDE_CONFIG" ]; then
    # Read exclude list (skip comments and empty lines, extract test name before colon)
    while IFS=':' read -r test_name reason || [ -n "$test_name" ]; do
        # Skip comments and empty lines
        [[ "$test_name" =~ ^#.*$ ]] && continue
        [[ -z "$test_name" ]] && continue
        EXCLUDE_TESTS+=("$test_name")
    done < "$EXCLUDE_CONFIG"
else
    echo "Warning: Exclude config not found: $EXCLUDE_CONFIG"
fi

# Auto-discover all tests with config.toml
TESTS=()
for test_dir in "${SWFRECOMP_ROOT}/tests"/*/; do
    test_name=$(basename "$test_dir")
    if [ -f "${test_dir}/config.toml" ]; then
        # Check if test is in exclude list
        skip=0
        for exclude in "${EXCLUDE_TESTS[@]}"; do
            if [ "$test_name" = "$exclude" ]; then
                skip=1
                break
            fi
        done

        if [ $skip -eq 0 ]; then
            TESTS+=("$test_name")
        fi
    fi
done

# Sort tests alphabetically
IFS=$'\n' TESTS=($(sort <<<"${TESTS[*]}"))
unset IFS

echo "Auto-discovered ${#TESTS[@]} tests with config.toml"
echo "Excluded ${#EXCLUDE_TESTS[@]} tests: ${EXCLUDE_TESTS[*]}"
echo "Building all tests for native execution..."
echo ""

SUCCESS_COUNT=0
FAIL_COUNT=0
TIMEOUT_COUNT=0
FAILED_TESTS=()
TIMEOUT_TESTS=()

# Build timeout per test (in seconds)
BUILD_TIMEOUT=60

for test_name in "${TESTS[@]}"; do
    echo "========================================="
    echo "Building: $test_name ($((SUCCESS_COUNT + FAIL_COUNT + TIMEOUT_COUNT + 1))/${#TESTS[@]})"
    echo "========================================="

    # Build with timeout (allow failures and continue)
    if timeout "$BUILD_TIMEOUT" "${SCRIPT_DIR}/build_test.sh" "$test_name" native 2>&1 | grep -q "✅ Native build complete"; then
        echo "✅ $test_name - built successfully"
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        # Check if it was a timeout (exit code 124)
        if [ $? -eq 124 ]; then
            echo "⏱️  $test_name - build timeout (>${BUILD_TIMEOUT}s)"
            TIMEOUT_COUNT=$((TIMEOUT_COUNT + 1))
            TIMEOUT_TESTS+=("$test_name")
        else
            echo "❌ $test_name - build failed"
            FAIL_COUNT=$((FAIL_COUNT + 1))
            FAILED_TESTS+=("$test_name")
        fi
    fi

    echo ""
done

echo "========================================="
echo "Build Summary"
echo "========================================="
echo "✅ Successful: $SUCCESS_COUNT"
echo "❌ Failed: $FAIL_COUNT"
echo "⏱️  Timeout: $TIMEOUT_COUNT"
echo "Total: ${#TESTS[@]}"

if [ $FAIL_COUNT -gt 0 ]; then
    echo ""
    echo "Failed tests:"
    for failed in "${FAILED_TESTS[@]}"; do
        echo "  - $failed"
    done
fi

if [ $TIMEOUT_COUNT -gt 0 ]; then
    echo ""
    echo "Timed out tests (may be too complex for quick builds):"
    for timeout_test in "${TIMEOUT_TESTS[@]}"; do
        echo "  - $timeout_test"
    done
fi

echo ""
echo "Native executables are in: tests/<test_name>/build/native/"
