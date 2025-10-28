# WASM Hello World Test

This is a simple test to verify Emscripten is set up correctly before starting the SWFRecomp WASM port.

## What This Tests

- ✅ C code compiles to WebAssembly
- ✅ WASM module loads in browser
- ✅ JavaScript can call C functions
- ✅ C can print to browser console
- ✅ Functions can accept parameters and return values

## Prerequisites

Install Emscripten (if not already installed):

```bash
# Clone Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git ~/emsdk
cd ~/emsdk

# Install and activate latest version
./emsdk install latest
./emsdk activate latest

# Activate for current shell (add to ~/.bashrc to make permanent)
source ./emsdk_env.sh
```

## Build Instructions

```bash
cd wasm-hello-world

# Run the build script
./build.sh
```

This will generate:
- `hello.js` - JavaScript glue code (~100KB)
- `hello.wasm` - WebAssembly binary (~20KB)

## Test Instructions

1. Start a local web server:
```bash
python3 -m http.server 8000
```

2. Open in browser:
```
http://localhost:8000/index.html
```

3. You should see:
   - ✅ Green "WASM module loaded and ready!" status
   - Buttons enabled
   - Console output showing "WASM module loaded successfully!"

4. Test the buttons:
   - **Call sayHello()** - Should print "Hello from WebAssembly!"
   - **Call add()** - Should add two numbers and print result

## Expected Output

In the browser console and on the page, you should see:

```
WASM module loaded successfully!
Call sayHello() or add(a, b) from JavaScript.
=================================================================
Module ready! Try clicking the buttons above.

> Calling sayHello()...
Hello from WebAssembly!

> Calling add(5, 7)...
C function called: 5 + 7 = 12
JavaScript received return value: 12
```

## Troubleshooting

### Error: "emcc: command not found"

Make sure you activated Emscripten:
```bash
source ~/emsdk/emsdk_env.sh
```

### Browser shows blank page

Check browser console for errors. Make sure you're accessing via HTTP server, not `file://`

### Functions not working

Check that:
1. Build completed without errors
2. `hello.js` and `hello.wasm` exist in the directory
3. Browser console shows no errors

## What's Next

Once this works, you're ready to start Phase 1 of the SWFRecomp WASM port!

The next step will be to:
1. Create the rendering abstraction layer
2. Compile the actual SWFModernRuntime to WASM
3. Test with a real SWF (starting with `mess` test)
