# SWFRecomp

This is a stupid idea.

# Let's do it anyway.

## What is this?

SWFRecomp is a **static recompiler** (not an emulator) that converts Flash SWF files into C code. Inspired by [N64Recomp](https://github.com/N64Recomp/N64Recomp) by Wiseguy, a static recompiler (not emulator) for N64 games that translates N64 MIPS instructions into C code that can be compiled into a native port. You should probably check that out, because it's epic. So is he. :D

This project applies the same approach to preserve Flash content.

**Live Demos:** https://swfrecomp.github.io/SWFRecompDocs/

This fork adds:
- **Improved build system** - Automated native and WASM builds with no manual file copying
- **Better project structure** - Clean separation of source, generated, and build files
- **Enhanced documentation** - Complete guides for the entire recompilation process
- **WebAssembly examples** - Working demos that run in your browser

## Documentation

### This Repository

**Core Documentation:**
- **[TRACE_SWF_4_WASM_GENERATION_GUIDE.md](TRACE_SWF_4_WASM_GENERATION_GUIDE.md)** - Complete guide to the entire SWF → WASM pipeline
- **[WASM_PROJECT_PLAN.md](WASM_PROJECT_PLAN.md)** - Detailed WASM development plan and roadmap
- **[PROJECT_STATUS.md](PROJECT_STATUS.md)** - Current project status and progress

**ActionScript 3 Support (Pure C):**
- **[AS3_IMPLEMENTATION_GUIDE.md](AS3_IMPLEMENTATION_GUIDE.md)** - Complete AS3 implementation guide in pure C
- **[SEEDLING_IMPLEMENTATION_GUIDE.md](SEEDLING_IMPLEMENTATION_GUIDE.md)** - Targeted AS3 implementation guide for Seedling game
- **[ABC_PARSER_GUIDE.md](ABC_PARSER_GUIDE.md)** - Detailed guide for Phase 1: ABC Parser implementation
- **[ABC_PARSER_RESEARCH.md](ABC_PARSER_RESEARCH.md)** - ABC format specification verification and license analysis
- **[ABC_IMPLEMENTATION_INFO.md](ABC_IMPLEMENTATION_INFO.md)** - Complete reference for implementing ABC parser (codebase integration, opcodes, roadmap)
- **[AS3_TEST_SWF_GENERATION_GUIDE.md](AS3_TEST_SWF_GENERATION_GUIDE.md)** - Guide for creating AS3 test SWF files using Flex SDK and AIR SDK

**Font and Text Support:**
- **[FONT_IMPLEMENTATION_ANALYSIS.md](FONT_IMPLEMENTATION_ANALYSIS.md)** - Complete analysis of font implementation status and requirements
- **[FONT_PHASE1_IMPLEMENTATION.md](FONT_PHASE1_IMPLEMENTATION.md)** - Step-by-step guide for implementing Phase 1: Basic font support with DefineFontInfo parsing

**Deprecated Documentation (Moved to deprecated/):**
- **[deprecated/AS3_C_IMPLEMENTATION_PLAN.md](deprecated/AS3_C_IMPLEMENTATION_PLAN.md)** - Original AS3 plan with time estimates
- **[deprecated/SEEDLING_C_IMPLEMENTATION_PLAN.md](deprecated/SEEDLING_C_IMPLEMENTATION_PLAN.md)** - Original Seedling plan with time estimates
- **[deprecated/C_VS_CPP_ARCHITECTURE.md](deprecated/C_VS_CPP_ARCHITECTURE.md)** - Why SWFRecomp uses C++ for build tools and C for runtime
- **[deprecated/AS3_IMPLEMENTATION_PLAN.md](deprecated/AS3_IMPLEMENTATION_PLAN.md)** - Full AS3 with C++
- **[deprecated/SEEDLING_IMPLEMENTATION_PLAN.md](deprecated/SEEDLING_IMPLEMENTATION_PLAN.md)** - Seedling with C++
- **[deprecated/SEEDLING_MANUAL_C_CONVERSION.md](deprecated/SEEDLING_MANUAL_C_CONVERSION.md)** - Manual AS3→C conversion for Seedling
- **[deprecated/SEEDLING_MANUAL_CPP_CONVERSION.md](deprecated/SEEDLING_MANUAL_CPP_CONVERSION.md)** - Manual AS3→C++ conversion
- **[deprecated/SYNERGY_ANALYSIS_C.md](deprecated/SYNERGY_ANALYSIS_C.md)** - How manual C conversion and SWFRecomp can work together
- **[deprecated/SYNERGY_ANALYSIS.md](deprecated/SYNERGY_ANALYSIS.md)** - Synergy analysis with C++

**Build System:**
- Each test directory has a `Makefile` and `build_wasm.sh` for automated builds
- Runtime files are in `tests/*/runtime/` directories

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

Check out the [live demo](https://swfrecomp.github.io/SWFRecompDocs/examples/trace-swf-test/) of the `trace_swf_4` test!

## Related Projects

- **[SWFModernRuntime](https://github.com/PeerInfinity/SWFModernRuntime)** - Runtime library for executing recompiled Flash with graphics
- **[N64Recomp](https://github.com/N64Recomp/N64Recomp)** - The inspiration for this project

## Legal Note

Fortunately, Adobe released most of their control over Flash as part of the [Open Screen Project](https://web.archive.org/web/20080506095459/http://www.adobe.com/aboutadobe/pressroom/pressreleases/200804/050108AdobeOSP.html), Adobe removed many restrictions from their SWF format, including restrictions on creating software that _plays and renders SWF files_:

> To support this mission, and as part of Adobe’s ongoing commitment to enable Web innovation, Adobe will continue to open access to Adobe Flash technology, accelerating the deployment of content and rich Internet applications (RIAs). This work will include:
> 
> - Removing restrictions on use of the SWF and FLV/F4V specifications
> - Publishing the device porting layer APIs for Adobe Flash Player
> - Publishing the Adobe Flash® Cast™ protocol and the AMF protocol for robust data services
> - Removing licensing fees - making next major releases of Adobe Flash Player and Adobe AIR for devices free

# Special Thanks

All the people that wildly inspire me. 😋

My very dear friend Stave.

From RecompRando:
- ThatHypedPerson
- PixelShake92
- Muervo_

From N64Recomp:

- Wiseguy
- Darío
- dcvz
- Reonu
- thecozies
- danielryb
- kentonm

From Archipelago:

- seto10987
- Rogue
- ArsonAssassin
- CelestialKitsune
- Vincent'sSin
- LegendaryLinux
- zakwiz
- jjjj12212

From RotMG:

- HuskyJew
- Nequ
- snowden
- MoonMan
- Auru
