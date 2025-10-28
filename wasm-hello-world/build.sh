#!/bin/bash
set -e

BUILD_DIR="build"
SRC_DIR="src"

echo "=== Building Hello World WASM ==="
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
cp "$SRC_DIR/hello.c" "$BUILD_DIR/"
cp index.html "$BUILD_DIR/"
cp favicon.svg "$BUILD_DIR/" 2>/dev/null || true
echo "✓ Build directory prepared"
echo ""

# Compile to WASM
echo "Compiling hello.c to WASM..."
cd "$BUILD_DIR"
emcc hello.c \
    -o hello.js \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_main","_sayHello","_add"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -O2
cd ..

echo ""
echo "✅ Build complete!"
echo ""
echo "Generated files:"
ls -lh "$BUILD_DIR/hello.js" "$BUILD_DIR/hello.wasm" 2>/dev/null
echo ""
echo "To test:"
echo "  1. cd $BUILD_DIR"
echo "  2. python3 -m http.server 8000"
echo "  3. Open http://localhost:8000/index.html"
echo ""
