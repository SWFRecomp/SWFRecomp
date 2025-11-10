#!/bin/bash
set -e

BUILD_DIR="build/wasm"
RUNTIME_DIR="runtime/wasm"

echo "=== Building trace_swf_4 to WASM ==="
echo ""

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "❌ Error: Emscripten (emcc) not found!"
    echo "Run: source ~/tools/emsdk/emsdk_env.sh"
    exit 1
fi

echo "✓ Emscripten found: $(emcc --version | head -n1)"
echo ""

# Setup build directory
echo "Setting up build directory..."
mkdir -p "$BUILD_DIR"

# Copy runtime files
cp "$RUNTIME_DIR/main.c" "$BUILD_DIR/"
cp "$RUNTIME_DIR/runtime.c" "$BUILD_DIR/"
cp "$RUNTIME_DIR/recomp.h" "$BUILD_DIR/"
cp "$RUNTIME_DIR/stackvalue.h" "$BUILD_DIR/"
cp "$RUNTIME_DIR/index.html" "$BUILD_DIR/"

# Copy generated files
cp RecompiledScripts/*.c "$BUILD_DIR/" 2>/dev/null || true
cp RecompiledScripts/*.h "$BUILD_DIR/" 2>/dev/null || true
cp RecompiledTags/*.c "$BUILD_DIR/" 2>/dev/null || true
cp RecompiledTags/*.h "$BUILD_DIR/" 2>/dev/null || true

echo "✓ Build directory prepared"
echo ""

# Compile all C files
echo "Compiling SWF recompiled code to WASM..."
cd "$BUILD_DIR"
emcc \
    main.c \
    runtime.c \
    tagMain.c \
    constants.c \
    draws.c \
    script_0.c \
    script_defs.c \
    -I. \
    -o trace_swf.js \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_main","_runSWF"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -O2

cd ../..

echo ""
echo "✅ Build complete!"
echo ""
echo "Generated files:"
ls -lh "$BUILD_DIR/trace_swf.js" "$BUILD_DIR/trace_swf.wasm" 2>/dev/null
echo ""
echo "To test:"
echo "  1. cd $BUILD_DIR"
echo "  2. python3 -m http.server 8000"
echo "  3. Open http://localhost:8000/index.html"
echo ""
