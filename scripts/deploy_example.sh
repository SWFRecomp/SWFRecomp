#!/bin/bash
# Deploy a test's WASM build to docs examples
# Usage: ./scripts/deploy_example.sh <test_name> [docs_dir]

set -e

TEST_NAME=$1
DOCS_DIR=${2:-../SWFRecompDocs/docs/examples}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWFRECOMP_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${SWFRECOMP_ROOT}/tests/${TEST_NAME}/build/wasm"
DEPLOY_DIR="${DOCS_DIR}/${TEST_NAME}"

# Validate inputs
if [ -z "$TEST_NAME" ]; then
    echo "Error: Test name required"
    echo "Usage: $0 <test_name> [docs_dir]"
    exit 1
fi

if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found: $BUILD_DIR"
    echo "Run ./scripts/build_test.sh ${TEST_NAME} wasm first"
    exit 1
fi

# Create deployment directory
mkdir -p "${DEPLOY_DIR}"

# Copy WASM artifacts
echo "Deploying ${TEST_NAME} to ${DEPLOY_DIR}..."
cp "${BUILD_DIR}"/*.wasm "${DEPLOY_DIR}/" 2>/dev/null || true
cp "${BUILD_DIR}"/*.js "${DEPLOY_DIR}/" 2>/dev/null || true
cp "${BUILD_DIR}"/index.html "${DEPLOY_DIR}/" 2>/dev/null || true

echo "✅ Deployed ${TEST_NAME} to ${DEPLOY_DIR}"
echo ""
echo "Files deployed:"
ls -lh "${DEPLOY_DIR}"
