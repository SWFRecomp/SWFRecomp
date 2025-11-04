#!/bin/bash
# Build all tests and deploy to docs
# Usage: ./scripts/build_all_examples.sh [docs_dir]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DOCS_DIR=${1:-../SWFRecompDocs/docs/examples}

# List of tests to build (can be auto-discovered or manually maintained)
TESTS=(
    "trace_swf_4"
    "dyna_string_vars_swf_4"
    # Add more tests here
)

echo "Building ${#TESTS[@]} tests for WASM deployment..."
echo ""

for test_name in "${TESTS[@]}"; do
    echo "========================================="
    echo "Building: $test_name"
    echo "========================================="

    # Build
    "${SCRIPT_DIR}/build_test.sh" "$test_name" wasm

    # Deploy
    "${SCRIPT_DIR}/deploy_example.sh" "$test_name" "$DOCS_DIR"

    echo ""
done

echo "✅ All tests built and deployed!"
echo "Documentation examples location: $DOCS_DIR"
