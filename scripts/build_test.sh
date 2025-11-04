#!/bin/bash
# Usage: ./scripts/build_test.sh <test_name> [native|wasm]
# Example: ./scripts/build_test.sh trace_swf_4 wasm

set -e

TEST_NAME=$1
TARGET=${2:-wasm}              # Default: wasm

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SWFRECOMP_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
TEST_DIR="${SWFRECOMP_ROOT}/tests/${TEST_NAME}"
BUILD_DIR="${TEST_DIR}/build/${TARGET}"

# Validate inputs
if [ -z "$TEST_NAME" ]; then
    echo "Error: Test name required"
    echo "Usage: $0 <test_name> [native|wasm]"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo "Error: Test directory not found: $TEST_DIR"
    exit 1
fi

# Run SWFRecomp if needed
if [ ! -d "${TEST_DIR}/RecompiledScripts" ]; then
    echo "Running SWFRecomp..."
    cd "${TEST_DIR}"
    "${SWFRECOMP_ROOT}/build/SWFRecomp" config.toml
fi

# Setup build directory
echo "Setting up build directory..."
mkdir -p "${BUILD_DIR}"

# Setup paths to SWFModernRuntime (sibling directory)
SWFMODERN_ROOT="${SWFRECOMP_ROOT}/../SWFModernRuntime"
SWFMODERN_SRC="${SWFMODERN_ROOT}/src"
SWFMODERN_INC="${SWFMODERN_ROOT}/include"

# Verify SWFModernRuntime exists
if [ ! -d "$SWFMODERN_ROOT" ]; then
    echo "Error: SWFModernRuntime not found at: $SWFMODERN_ROOT"
    echo "Expected directory structure:"
    echo "  /path/to/projects/SWFRecomp/"
    echo "  /path/to/projects/SWFModernRuntime/"
    exit 1
fi

# Copy or use main.c
if [ "$TARGET" == "wasm" ]; then
    # Use WASM wrapper main.c (has Emscripten exports)
    cp "${SWFRECOMP_ROOT}/wasm_wrappers/main.c" "${BUILD_DIR}/"
    cp "${SWFRECOMP_ROOT}/wasm_wrappers/index_template.html" "${BUILD_DIR}/index.html"

    # Customize HTML with test name
    sed -i "s/{{TEST_NAME}}/${TEST_NAME}/g" "${BUILD_DIR}/index.html"
else
    # Use test's own main.c for native builds
    if [ -f "${TEST_DIR}/main.c" ]; then
        cp "${TEST_DIR}/main.c" "${BUILD_DIR}/"
    else
        # Fallback to wrapper main.c
        cp "${SWFRECOMP_ROOT}/wasm_wrappers/main.c" "${BUILD_DIR}/"
    fi
fi

# Copy SWFModernRuntime source files
echo "Copying SWFModernRuntime sources..."
cp "${SWFMODERN_SRC}/actionmodern/action.c" "${BUILD_DIR}/"
cp "${SWFMODERN_SRC}/actionmodern/variables.c" "${BUILD_DIR}/"
cp "${SWFMODERN_SRC}/utils.c" "${BUILD_DIR}/"

# For WASM builds, use NO_GRAPHICS mode (console-only)
# For native builds, use full graphics mode
if [ "$TARGET" == "wasm" ]; then
    echo "Using NO_GRAPHICS mode for WASM build..."
    cp "${SWFMODERN_SRC}/libswf/swf_core.c" "${BUILD_DIR}/"
    cp "${SWFMODERN_SRC}/libswf/tag_stubs.c" "${BUILD_DIR}/"
else
    echo "Using full graphics mode for native build..."
    cp "${SWFMODERN_SRC}/libswf/swf.c" "${BUILD_DIR}/"
    cp "${SWFMODERN_SRC}/libswf/tag.c" "${BUILD_DIR}/"
    cp "${SWFMODERN_SRC}/flashbang/flashbang.c" "${BUILD_DIR}/"
fi

# Copy hashmap library (required for variable storage)
cp "${SWFMODERN_ROOT}/lib/c-hashmap/map.c" "${BUILD_DIR}/"

# Copy generated files from SWFRecomp
echo "Copying generated files..."
cp "${TEST_DIR}/RecompiledScripts"/*.c "${BUILD_DIR}/" 2>/dev/null || true
cp "${TEST_DIR}/RecompiledScripts"/*.h "${BUILD_DIR}/" 2>/dev/null || true
cp "${TEST_DIR}/RecompiledTags"/*.c "${BUILD_DIR}/" 2>/dev/null || true
cp "${TEST_DIR}/RecompiledTags"/*.h "${BUILD_DIR}/" 2>/dev/null || true

# Build
if [ "$TARGET" == "wasm" ]; then
    echo "Building WASM with SWFModernRuntime..."

    # Check if emcc is available
    if ! command -v emcc &> /dev/null; then
        echo "Error: Emscripten (emcc) not found!"
        echo "Run: source ~/tools/emsdk/emsdk_env.sh"
        exit 1
    fi

    cd "${BUILD_DIR}"
    emcc \
        *.c \
        -DNO_GRAPHICS \
        -I. \
        -I"${SWFMODERN_INC}" \
        -I"${SWFMODERN_INC}/actionmodern" \
        -I"${SWFMODERN_INC}/libswf" \
        -I"${SWFMODERN_ROOT}/lib/c-hashmap" \
        -o "${TEST_NAME}.js" \
        -s WASM=1 \
        -s EXPORTED_FUNCTIONS='["_main","_runSWF"]' \
        -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s INITIAL_MEMORY=16MB \
        -O2

    echo ""
    echo "✅ WASM build complete!"
    echo "Output: ${BUILD_DIR}/${TEST_NAME}.wasm"
    echo ""
    echo "To test:"
    echo "  cd ${BUILD_DIR}"
    echo "  python3 -m http.server 8000"
    echo "  Open http://localhost:8000/index.html"

else
    echo "Building native with SWFModernRuntime..."
    cd "${BUILD_DIR}"

    # Compile SWFModernRuntime from source
    gcc \
        *.c \
        -I. \
        -I"${SWFMODERN_INC}" \
        -I"${SWFMODERN_INC}/actionmodern" \
        -I"${SWFMODERN_INC}/libswf" \
        -I"${SWFMODERN_ROOT}/lib/c-hashmap" \
        -Wall \
        -Wno-unused-variable \
        -std=c17 \
        -o "${TEST_NAME}"

    echo ""
    echo "✅ Native build complete!"
    echo "Output: ${BUILD_DIR}/${TEST_NAME}"
    echo ""
    echo "To run:"
    echo "  ${BUILD_DIR}/${TEST_NAME}"
fi
