# SWFRecomp

Static recompiler for Adobe Flash SWF files - translates Flash bytecode into portable C code that can be compiled to native executables or WebAssembly.

**Live Demos:** https://peerinfinity.github.io/SWFRecomp/

## What is This?

SWFRecomp is a **static recompiler** (not an emulator) that converts Flash SWF files into C code. Inspired by [N64Recomp](https://github.com/N64Recomp/N64Recomp) by Wiseguy, this project applies the same approach to preserve Flash content.

This fork adds:
- **Improved build system** - Automated native and WASM builds with no manual file copying
- **Better project structure** - Clean separation of source, generated, and build files
- **Enhanced documentation** - Complete guides for the entire recompilation process
- **WebAssembly examples** - Working demos that run in your browser

## Documentation

### This Repository

- **[TRACE_SWF_4_WASM_GENERATION_GUIDE.md](TRACE_SWF_4_WASM_GENERATION_GUIDE.md)** - Complete guide to the entire SWF → WASM pipeline
- **[WASM_PROJECT_PLAN.md](WASM_PROJECT_PLAN.md)** - Detailed WASM development plan and roadmap
- **[PROJECT_STATUS.md](PROJECT_STATUS.md)** - Current project status and progress
- **[AS3_IMPLEMENTATION_PLAN.md](AS3_IMPLEMENTATION_PLAN.md)** - Implementation plan for adding AS3 support
- **Build System:** Each test directory has a `Makefile` and `build_wasm.sh` for automated builds
- **Runtime:** Runtime files are in `tests/*/runtime/` directories

### SWFModernRuntime

- **[README.md](https://github.com/PeerInfinity/SWFModernRuntime/tree/wasm-support/README.md)** - Main README file

### Upstream

- **[Upstream README](https://github.com/SWFRecomp/SWFRecomp/blob/master/README.md)** - Original SWFRecomp documentation

## Quick Start

### Prerequisites

**For native builds:**
```bash
# Standard C++ compiler
sudo apt install build-essential cmake
```

**For WASM builds:**
```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git ~/tools/emsdk
cd ~/tools/emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### Build SWFRecomp

```bash
git clone https://github.com/PeerInfinity/SWFRecomp.git
cd SWFRecomp

# Build the recompiler
mkdir -p build && cd build
cmake ..
make
cd ..
```

### Run an Example

```bash
cd tests/trace_swf_4

# Step 1: Recompile SWF to C
../../build/SWFRecomp config.toml

# Step 2: Build natively (for testing)
make
./build/native/TestSWFRecompiled
# Output: sup from SWF 4

# Step 3: Build for WASM (for web)
source ~/tools/emsdk/emsdk_env.sh
./build_wasm.sh

# Test in browser
cd build/wasm
python3 -m http.server 8000
# Open http://localhost:8000/index.html
```

## Project Structure

```
SWFRecomp/
├── src/                    # Recompiler source code
│   ├── swf.cpp            # SWF parsing
│   ├── tag.cpp            # Tag processing
│   ├── action/            # ActionScript translation
│   └── recompilation.cpp  # C code generation
├── tests/                  # Test cases with complete build setup
│   ├── trace_swf_4/       # Console output test (working!)
│   │   ├── test.swf       # Source SWF file
│   │   ├── config.toml    # Recompiler config
│   │   ├── runtime/       # Runtime implementations
│   │   │   ├── native/    # For native builds
│   │   │   └── wasm/      # For WASM builds
│   │   ├── Makefile       # Automated native build
│   │   └── build_wasm.sh  # Automated WASM build
│   └── graphics/          # Graphics tests
├── docs/                   # GitHub Pages with live demos
└── wasm-hello-world/      # Basic WASM example
```

## How It Works

```
┌─────────────┐
│  test.swf   │  Original Flash SWF file
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  SWFRecomp  │  Static recompiler (this project)
└──────┬──────┘
       │
       │  Generates C files:
       │  ├── script_0.c      (ActionScript → C)
       │  ├── tagMain.c       (Frame logic)
       │  └── constants.c     (String literals)
       │
       ▼
┌─────────────┐
│   C Code    │  Portable C17 code
└──────┬──────┘
       │
       ├──────────────┬─────────────┐
       ▼              ▼             ▼
   ┌──────┐      ┌────────┐   ┌────────┐
   │ gcc  │      │ clang  │   │ emcc   │
   └──┬───┘      └───┬────┘   └───┬────┘
      │              │            │
      ▼              ▼            ▼
  ┌────────┐    ┌────────┐   ┌────────┐
  │ Native │    │ Native │   │  WASM  │
  │  x86   │    │  ARM   │   │Browser │
  └────────┘    └────────┘   └────────┘
```

## Current Features

| Feature | Status | Notes |
|---------|--------|-------|
| **SWF Parsing** | ✅ Working | All compression formats |
| **ActionScript 1.0** | ✅ Partial | SWF v4 actions working |
| **Shape Rendering** | ✅ Partial | DefineShape support |
| **Native Builds** | ✅ Working | gcc/clang compilation |
| **WASM Builds** | ✅ Working | Emscripten compilation |
| **Automated Build** | ✅ Working | No manual file copying |

## What Can This Do Right Now?

Currently, SWFRecomp successfully:
- Decompresses all forms of SWF compression (none, zlib, LZMA)
- Reads SWF file headers and metadata
- Recompiles ActionScript actions from SWF 4 into equivalent C code
- Handles many graphics operations defined in `DefineShape` tags
- Generates portable C code that compiles to both native and WASM

Check out the [live demo](https://peerinfinity.github.io/SWFRecomp/examples/trace-swf-test/) of the `trace_swf_4` test!

## Related Projects

- **[SWFModernRuntime](https://github.com/PeerInfinity/SWFModernRuntime)** - Runtime library for executing recompiled Flash with graphics
- **[N64Recomp](https://github.com/N64Recomp/N64Recomp)** - The inspiration for this project

## Legal Note

Adobe released most control over Flash as part of the [Open Screen Project](https://web.archive.org/web/20080506095459/http://www.adobe.com/aboutadobe/pressroom/pressreleases/200804/050108AdobeOSP.html), removing many restrictions from the SWF format:

> Removing restrictions on use of the SWF and FLV/F4V specifications

Adobe also [donated the Flex 3 SDK to Apache](https://www.pcworld.com/article/478324/adobe_donates_flex_to_apache-2.html) in 2011, licensed under the Apache License (a permissive open-source license).

## License

Same as upstream SWFRecomp (check LICENSE file).

## Credits

- **Upstream:** [SWFRecomp](https://github.com/SWFRecomp/SWFRecomp) by LittleCube
- **Inspiration:** [N64Recomp](https://github.com/N64Recomp/N64Recomp) by Wiseguy
- **Related:** [SWFModernRuntime](https://github.com/PeerInfinity/SWFModernRuntime) - Runtime for recompiled SWFs

---

**Note:** This is a community fork. The upstream project (by LittleCube) provides the core recompilation engine. This fork adds improved build tooling, WASM support, and documentation.
