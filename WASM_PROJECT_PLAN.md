# SWFRecomp WASM Port - Project Plan

**Document Version:** 1.1
**Created:** October 27, 2025
**Last Updated:** October 27, 2025
**Upstream Project:** LittleCube (SWFRecomp + SWFModernRuntime)
**Status:** Phase 1 - Implementation In Progress

---

## Table of Contents

1. [Project Overview](#project-overview)
2. [Fork Strategy](#fork-strategy)
3. [Technical Architecture](#technical-architecture)
4. [Phase 1: Canvas2D Prototype](#phase-1-canvas2d-prototype)
5. [Phase 2: WebGL2 Backend (Optional)](#phase-2-webgl2-backend-optional)
6. [Phase 3: SDL3 WebGPU Migration](#phase-3-sdl3-webgpu-migration)
7. [Maintaining Native Runtime Compatibility](#maintaining-native-runtime-compatibility)
8. [Build System Design](#build-system-design)
9. [Testing Strategy](#testing-strategy)
10. [Upstream Sync Strategy](#upstream-sync-strategy)
11. [Timeline & Milestones](#timeline--milestones)
12. [Risk Management](#risk-management)

---

## Project Overview

### Goals

**Primary Goal:** Enable SWFRecomp-generated C code to run in web browsers via WebAssembly (WASM) while maintaining full native runtime compatibility.

**Secondary Goals:**
- Preserve all existing native runtime functionality
- Maintain compatibility with upstream development
- Create a path for future SDL3 WebGPU integration
- Enable Flash games to run in modern browsers without plugins

### Non-Goals

- ❌ Replacing or modifying upstream native runtime
- ❌ Competing with LittleCube's development direction
- ❌ Comprehensive WebGL2 optimization (unless needed)
- ❌ Supporting browsers without WASM support

### Success Criteria

1. ✅ Generated C code compiles to WASM with Emscripten - **ACHIEVED**
2. ⏳ Basic graphics rendering works in browser (Canvas2D) - **IN PROGRESS**
3. ✅ ActionScript execution works in WASM - **ACHIEVED** (trace_swf_4 example working)
4. ✅ Native runtime builds remain unaffected - **MAINTAINED** (on wasm-support branch)
5. ⏳ Test suite passes in both native and WASM builds - **IN PROGRESS**
6. ✅ Easy to sync with upstream changes - **ACHIEVED** (clean separation via wasm/ directory)

### Upstream Status Update (October 27, 2025)

**Message from LittleCube:**

> "ah sorry i haven't refixed the tests to work with the new stuff lol
> the test that should work right now is the graphics test mess
> and i believe wild shadow and the two gradient tests should work too
> and i don't have any plans to, i know like nothing about web dev 🥴
> but if you can find a way to run a C runtime on the web and have the recompiler emit WASM then be my guest
> or however you want to do it"

**Key Takeaways:**
- ✅ LittleCube is **supportive** of WASM efforts
- ✅ No plans to implement WASM upstream (no conflict of interest)
- ⚠️ Test suite currently in flux (recent refactoring)
- ✅ Working tests: `mess`, `wild_shadow`, gradient tests
- ⚠️ ActionScript tests not yet updated for recent runtime changes

**Impact on Plan:**
- Focus initial testing on working graphics tests (`mess`, `wild_shadow`, gradients)
- Accept that ActionScript tests may need updates
- Proceed with plan as designed - upstream is open to community WASM work
- No need for extensive coordination (LittleCube has no web dev plans)

### Recent Upstream Progress (October 2025)

**Major Rendering Updates:**
- ✅ **Gradients implemented** - Linear and radial gradients with compute shader support
- ✅ **Bitmaps implemented** - Texture rendering with style indices
- ✅ **MSAA support** - Multi-sample anti-aliasing added
- ✅ **Refactored rendering** - color_info → texture_info, static shapes optimized
- ✅ **Display list architecture** - Character and DisplayObject structs, proper transforms

**Impact on WASM Fork:**
- ⚠️ Rendering backend has significantly evolved since planning phase
- ⚠️ Canvas2D backend will need to support gradients and bitmaps
- ✅ Good news: These features improve visual quality targets
- ⚠️ Complexity: WebGL2 backend may now be more essential than "optional"
- ⚠️ Plan adjustment: Consider prioritizing WebGL2 over Canvas2D for feature parity

---

## Fork Strategy

### Repository Structure

**Recommended Approach:** Fork with clear upstream tracking

```
Your Fork: SWFRecomp
├── Upstream: SWFRecomp (LittleCube)
└── Upstream: SWFModernRuntime (LittleCube)
```

### Git Workflow

```bash
# Initial setup
git clone https://github.com/SWFRecomp/SWFRecomp.git SWFRecomp
cd SWFRecomp
git remote rename origin upstream
git remote add origin <your-fork-url>

# Track upstream
git checkout -b wasm-support
git push -u origin wasm-support

# Periodic upstream sync
git fetch upstream
git merge upstream/master
```

### Branch Strategy

- **`master`** - Tracks upstream, unmodified
- **`wasm-support`** - Your WASM modifications
- **`wasm-canvas2d`** - Phase 1 work
- **`wasm-webgl2`** - Phase 2 work (optional)
- **`wasm-webgpu`** - Phase 3 work (future)

### Contribution Philosophy

**Upstream Contributions:**
- Bug fixes → Submit to LittleCube
- Native improvements → Submit to LittleCube
- WASM-specific code → Keep in fork (unless requested)

**Fork Maintenance:**
- Clearly mark WASM-specific code
- Use preprocessor guards (`#ifdef __EMSCRIPTEN__`)
- Document all deviations from upstream
- Make merging upstream changes easy

---

## Technical Architecture

### Current Architecture (Native)

```
SWF File
    ↓
SWFRecomp (unchanged - pure C++ tool)
    ↓
Generated C Code
    ├─ RecompiledTags/*.c
    └─ RecompiledScripts/*.c
    ↓
SWFModernRuntime
    ├─ libswf/ (frame management, SWF execution)
    ├─ actionmodern/ (ActionScript VM)
    └─ flashbang/ (SDL_GPU rendering)
    ↓
Native Executable (Linux/Windows/macOS)
```

### Target Architecture (WASM-Compatible)

```
SWF File
    ↓
SWFRecomp (unchanged - still native tool)
    ↓
Generated C Code (100% portable)
    ├─ RecompiledTags/*.c
    └─ RecompiledScripts/*.c
    ↓
SWFModernRuntime Core (WASM-compatible)
    ├─ libswf/ ✅ Portable
    ├─ actionmodern/ ✅ Portable
    └─ rendering/ (ABSTRACTED)
        ├─ render_api.h (interface)
        ├─ render_native.c (SDL_GPU) #ifndef __EMSCRIPTEN__
        ├─ render_canvas2d.c (Canvas2D) #ifdef __EMSCRIPTEN__
        ├─ render_webgl2.c (WebGL2) #ifdef __EMSCRIPTEN__ && WEBGL2
        └─ render_webgpu.c (Future) #ifdef __EMSCRIPTEN__ && WEBGPU
    ↓
Build Target Selection
    ├─→ Native Executable (Linux/Windows/macOS)
    └─→ WASM Binary + HTML/JS (Browser)
```

### Key Design Principles

1. **Minimal Native Impact:** Native builds should compile exactly as before
2. **Clean Abstraction:** Rendering backend swappable via compile-time flags
3. **Upstream Friendly:** Changes isolated, easy to merge
4. **Progressive Enhancement:** Start simple (Canvas2D), upgrade later (WebGPU)

---

## Phase 1: Canvas2D Prototype

**Status:** In Progress
**Priority:** High
**Goal:** Prove WASM compilation works end-to-end

### Progress Update

**✅ Completed:**
- Emscripten compilation working (trace_swf_4 example)
- ActionScript VM execution in WASM
- Frame management and execution
- Basic runtime infrastructure (runtime.c)
- Build scripts and HTML templates
- GitHub Pages deployment
- Live demo at https://peerinfinity.github.io/SWFModernRuntime/

**⏳ In Progress:**
- Canvas2D rendering backend implementation
- Shape rendering support
- Gradient support (required due to upstream changes)
- Bitmap support (required due to upstream changes)

**📋 Remaining:**
- Complete Canvas2D backend
- Test graphics examples (mess, wild_shadow)
- Performance profiling

### Objectives

1. ✅ Compile generated C code with Emscripten - **DONE**
2. ⏳ Create Canvas2D rendering backend - **IN PROGRESS**
3. ⏳ Render basic shapes in browser - **IN PROGRESS**
4. ✅ Verify ActionScript execution in WASM - **DONE** (trace_swf_4)
5. ⏳ Run graphics tests in browser - **NEXT**

### Dependencies

**Tools:**
- Emscripten SDK (latest)
- Python 3.x (for Emscripten)
- Node.js (for testing)
- Web browser with WASM support

**Knowledge:**
- C/C++ (already have)
- JavaScript basics
- HTML5 Canvas API
- Emscripten build system

### Architecture

#### Rendering Abstraction Layer

```c
// src/rendering/render_api.h
#ifndef RENDER_API_H
#define RENDER_API_H

#include <common.h>

typedef struct RenderContext {
    int width;
    int height;
    void* backend_data;
} RenderContext;

// Core rendering interface
RenderContext* render_init(int width, int height);
void render_begin_frame(RenderContext* ctx);
void render_end_frame(RenderContext* ctx);
void render_cleanup(RenderContext* ctx);

// Data upload (one-time initialization)
void render_upload_shapes(RenderContext* ctx, void* data, size_t size);
void render_upload_transforms(RenderContext* ctx, void* data, size_t size);
void render_upload_colors(RenderContext* ctx, void* data, size_t size);
void render_upload_gradients(RenderContext* ctx, void* data, size_t size);

// Drawing operations (per-frame)
void render_draw_shape(RenderContext* ctx,
                       int shape_id,
                       int transform_id,
                       int color_id);

// Input handling
int render_poll_events(RenderContext* ctx);

#endif
```

#### Canvas2D Implementation

```c
// src/rendering/render_canvas2d.c
#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <emscripten/html5.h>
#include "render_api.h"

typedef struct Canvas2DContext {
    int width;
    int height;

    // Cached data
    u32* shape_data;
    size_t shape_data_count;
    float* transform_data;
    size_t transform_count;
    float* color_data;
    size_t color_count;
} Canvas2DContext;

RenderContext* render_init(int width, int height) {
    RenderContext* ctx = malloc(sizeof(RenderContext));
    ctx->width = width;
    ctx->height = height;

    Canvas2DContext* canvas_ctx = malloc(sizeof(Canvas2DContext));
    canvas_ctx->width = width;
    canvas_ctx->height = height;
    ctx->backend_data = canvas_ctx;

    // Setup HTML canvas
    EM_ASM_({
        var canvas = document.getElementById('canvas');
        if (!canvas) {
            canvas = document.createElement('canvas');
            canvas.id = 'canvas';
            canvas.width = $0;
            canvas.height = $1;
            document.body.appendChild(canvas);
        }
    }, width, height);

    return ctx;
}

void render_upload_shapes(RenderContext* ctx, void* data, size_t size) {
    Canvas2DContext* canvas_ctx = (Canvas2DContext*)ctx->backend_data;

    // Copy shape data to WASM heap
    canvas_ctx->shape_data = malloc(size);
    memcpy(canvas_ctx->shape_data, data, size);
    canvas_ctx->shape_data_count = size / (4 * sizeof(u32)); // [x,y,z,w] per vertex
}

void render_upload_transforms(RenderContext* ctx, void* data, size_t size) {
    Canvas2DContext* canvas_ctx = (Canvas2DContext*)ctx->backend_data;

    canvas_ctx->transform_data = malloc(size);
    memcpy(canvas_ctx->transform_data, data, size);
    canvas_ctx->transform_count = size / (16 * sizeof(float)); // 4x4 matrix
}

void render_upload_colors(RenderContext* ctx, void* data, size_t size) {
    Canvas2DContext* canvas_ctx = (Canvas2DContext*)ctx->backend_data;

    canvas_ctx->color_data = malloc(size);
    memcpy(canvas_ctx->color_data, data, size);
    canvas_ctx->color_count = size / (4 * sizeof(float)); // RGBA
}

void render_begin_frame(RenderContext* ctx) {
    // Clear canvas
    EM_ASM({
        var canvas = document.getElementById('canvas');
        var context = canvas.getContext('2d');
        context.clearRect(0, 0, canvas.width, canvas.height);
    });
}

void render_draw_shape(RenderContext* ctx,
                       int shape_id,
                       int transform_id,
                       int color_id) {
    Canvas2DContext* canvas_ctx = (Canvas2DContext*)ctx->backend_data;

    // Get color
    float* color = &canvas_ctx->color_data[color_id * 4];
    int r = (int)(color[0] * 255);
    int g = (int)(color[1] * 255);
    int b = (int)(color[2] * 255);
    float a = color[3];

    // Get transform matrix (simplified - just translation for now)
    float* matrix = &canvas_ctx->transform_data[transform_id * 16];
    float tx = matrix[12];
    float ty = matrix[13];

    // Draw shape vertices as triangles
    // (simplified - assumes triangulated data)
    u32* vertices = &canvas_ctx->shape_data[shape_id * 4];

    EM_ASM_({
        var canvas = document.getElementById('canvas');
        var ctx = canvas.getContext('2d');

        ctx.fillStyle = 'rgba(' + $0 + ',' + $1 + ',' + $2 + ',' + $3 + ')';
        ctx.save();
        ctx.translate($4, $5);

        // Draw triangle (placeholder - would loop through all vertices)
        ctx.beginPath();
        ctx.moveTo(0, 0);
        ctx.lineTo(100, 0);
        ctx.lineTo(50, 100);
        ctx.closePath();
        ctx.fill();

        ctx.restore();
    }, r, g, b, a, tx, ty);
}

void render_end_frame(RenderContext* ctx) {
    // Canvas2D auto-presents
}

int render_poll_events(RenderContext* ctx) {
    // Return 0 to continue, 1 to quit
    // TODO: Hook up keyboard/mouse events
    return 0;
}

void render_cleanup(RenderContext* ctx) {
    Canvas2DContext* canvas_ctx = (Canvas2DContext*)ctx->backend_data;

    free(canvas_ctx->shape_data);
    free(canvas_ctx->transform_data);
    free(canvas_ctx->color_data);
    free(canvas_ctx);
    free(ctx);
}

#endif // __EMSCRIPTEN__
```

#### Native Adapter (Wrapper around existing flashbang)

```c
// src/rendering/render_native.c
#ifndef __EMSCRIPTEN__

#include "render_api.h"
#include <flashbang.h>

// This is just a thin wrapper around the existing flashbang code
// to match the new API

RenderContext* render_init(int width, int height) {
    RenderContext* ctx = malloc(sizeof(RenderContext));
    ctx->width = width;
    ctx->height = height;

    FlashbangContext* fb_ctx = flashbang_new();
    fb_ctx->width = width;
    fb_ctx->height = height;

    ctx->backend_data = fb_ctx;
    return ctx;
}

void render_upload_shapes(RenderContext* ctx, void* data, size_t size) {
    FlashbangContext* fb_ctx = (FlashbangContext*)ctx->backend_data;
    fb_ctx->shape_data = data;
    fb_ctx->shape_data_size = size;
}

// ... similar wrappers for other functions ...
// This allows native builds to continue using flashbang unchanged

#endif // !__EMSCRIPTEN__
```

### Integration with Runtime

Modify `src/libswf/swf.c` to use abstraction layer:

```c
// OLD CODE (native only):
#include <flashbang.h>
FlashbangContext* fb_ctx = flashbang_new();
flashbang_init(fb_ctx);

// NEW CODE (platform-agnostic):
#include <render_api.h>
RenderContext* render_ctx = render_init(width, height);
render_upload_shapes(render_ctx, shape_data, shape_data_size);
```

### Build Configuration

```cmake
# CMakeLists.txt additions
if(EMSCRIPTEN)
    message(STATUS "Building for WASM with Canvas2D")

    set(RENDER_SOURCES
        ${PROJECT_SOURCE_DIR}/src/rendering/render_canvas2d.c
    )

    set(CMAKE_EXECUTABLE_SUFFIX ".html")

    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "\
            -s WASM=1 \
            -s USE_SDL=0 \
            -s ALLOW_MEMORY_GROWTH=1 \
            -s EXPORTED_FUNCTIONS='[\"_main\"]' \
            -s EXPORTED_RUNTIME_METHODS='[\"cwrap\",\"ccall\"]' \
            --shell-file ${PROJECT_SOURCE_DIR}/src/rendering/shell.html \
        "
    )
else()
    message(STATUS "Building for native with SDL_GPU")

    set(RENDER_SOURCES
        ${PROJECT_SOURCE_DIR}/src/rendering/render_native.c
        ${PROJECT_SOURCE_DIR}/src/flashbang/flashbang.c
    )

    add_subdirectory(${PROJECT_SOURCE_DIR}/lib/SDL3)
    target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3)
endif()

target_sources(${PROJECT_NAME} PRIVATE
    ${RENDER_SOURCES}
    ${PROJECT_SOURCE_DIR}/src/rendering/render_api.h
)
```

### Build Commands

```bash
# Native build (unchanged)
mkdir build-native
cd build-native
cmake ..
make

# WASM build (new)
mkdir build-wasm
cd build-wasm
emcmake cmake ..
emmake make

# Output: TestSWFRecompiled.html, .js, .wasm
```

### Testing

**Priority Test Cases (Currently Working Upstream):**
1. ✅ `mess` - Graphics test (confirmed working by LittleCube)
2. ✅ `wild_shadow` - Complex graphics (confirmed working)
3. ✅ `awful_gradient` - Linear gradient test (confirmed working)
4. ✅ `awful_radial_gradient` - Radial gradient test (confirmed working)

**Secondary Test Cases (May Need Updates):**
5. ⚠️ `trace_swf_4` - ActionScript test (may need runtime updates)
6. ⚠️ `two_squares` - Basic shapes (status unknown)
7. ⚠️ Other ActionScript tests (not yet updated for new runtime)

**Success Metrics (Updated):**
- Graphics tests: `mess`, `wild_shadow`, gradients render correctly in WASM
- Basic rendering visible for at least 3-4 working tests
- No regressions in native builds
- ⚠️ ActionScript test count TBD (test suite in flux)

### Deliverables

- [x] `wasm/examples/trace-swf-test/` - Working ActionScript example
- [x] `wasm/examples/trace-swf-test/runtime.c` - Basic runtime implementation
- [x] `wasm/examples/trace-swf-test/build.sh` - Build script
- [x] `wasm/shell-templates/` - HTML templates for hosting
- [x] `docs/` - GitHub Pages site with live demos
- [x] `README.md` - WASM build instructions
- [ ] `render_api.h` - Platform-agnostic rendering interface (NEXT)
- [ ] `render_canvas2d.c` - Canvas2D backend implementation (NEXT)
- [ ] Graphics test examples (mess, wild_shadow) (NEXT)
- [ ] Performance benchmarks document

### Known Limitations (Phase 1)

- ❌ Performance will be poor (CPU rendering)
- ❌ Gradients may not work
- ❌ Complex shapes may render incorrectly
- ❌ No GPU acceleration
- ⚠️ This is a **proof of concept**, not production-ready

---

## Phase 2: WebGL2 Backend (Optional → Recommended)

**Status:** Planned (Priority Increased)
**Priority:** Medium → High
**Goal:** GPU-accelerated rendering in browser

### Status Update

**Priority has increased due to:**
- Upstream now supports gradients, bitmaps, and MSAA
- Canvas2D will struggle with complex graphics features
- Performance requirements higher than originally anticipated
- WebGL2 better matches upstream SDL_GPU feature set

### Decision Point

**Evaluate before starting Phase 2:**
- ✅ Is Phase 1 working? → **Partially (ActionScript done, rendering next)**
- ✅ Is SDL3 WebGPU support still 6+ months away? → **Yes, still future**
- ✅ Do you need GPU performance now? → **Yes, for feature parity**
- ✅ Are you willing to maintain shader code? → **Required for gradients/bitmaps**

**Decision:** Phase 2 now recommended after Phase 1 basics are working

### Objectives (if proceeding)

1. Implement WebGL2 rendering backend
2. Port shaders from SDL_GPU to GLSL ES 3.0
3. Achieve native-like rendering quality
4. Support gradients and bitmaps
5. Performance optimization

### Architecture

```c
// src/rendering/render_webgl2.c
#ifdef __EMSCRIPTEN__ && USE_WEBGL2

#include <emscripten.h>
#include <GLES3/gl3.h>
#include "render_api.h"

typedef struct WebGL2Context {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE gl_context;

    GLuint vertex_buffer;
    GLuint transform_buffer;
    GLuint color_buffer;

    GLuint shader_program;
    GLuint vao;

    // Cached data
    void* shape_data;
    size_t shape_data_size;
} WebGL2Context;

RenderContext* render_init(int width, int height) {
    // Create WebGL2 context
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx =
        emscripten_webgl_create_context("#canvas", &attrs);
    emscripten_webgl_make_context_current(ctx);

    // Initialize WebGL state...
    // Compile shaders...
    // Create buffers...

    // ... (implementation details)
}

// ... rest of WebGL2 implementation
#endif
```

### Shader Porting

**Vertex Shader (GLSL ES 3.0):**
```glsl
// shaders/webgl2/vertex.glsl
#version 300 es
precision highp float;

layout(location = 0) in vec4 a_position;

uniform mat4 u_transform;
uniform mat4 u_stage_to_ndc;

void main() {
    gl_Position = u_stage_to_ndc * u_transform * a_position;
}
```

**Fragment Shader (GLSL ES 3.0):**
```glsl
// shaders/webgl2/fragment.glsl
#version 300 es
precision highp float;

uniform vec4 u_color;
out vec4 fragColor;

void main() {
    fragColor = u_color;
}
```

### Build Configuration

```cmake
if(EMSCRIPTEN)
    option(USE_WEBGL2 "Use WebGL2 backend instead of Canvas2D" OFF)

    if(USE_WEBGL2)
        message(STATUS "Building for WASM with WebGL2")
        set(RENDER_SOURCES ${PROJECT_SOURCE_DIR}/src/rendering/render_webgl2.c)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS "-s USE_WEBGL2=1 -s FULL_ES3=1"
        )
    else()
        message(STATUS "Building for WASM with Canvas2D")
        set(RENDER_SOURCES ${PROJECT_SOURCE_DIR}/src/rendering/render_canvas2d.c)
    endif()
endif()
```

### Deliverables

- [ ] `render_webgl2.c` - WebGL2 backend
- [ ] Vertex/fragment shaders (GLSL ES 3.0)
- [ ] Shader compilation system
- [ ] Buffer management
- [ ] Performance benchmarks
- [ ] Documentation

### Performance Targets

- 60 FPS for simple games
- 30 FPS for complex games
- Sub-1MB WASM binary size

---

## Phase 3: SDL3 WebGPU Migration

**Timeline:** TBD (waiting on SDL3)
**Status:** Future
**Priority:** High (when available)
**Goal:** Use official SDL3 WebGPU backend

### Monitoring SDL3 Progress

**Resources to watch:**
- SDL3 GitHub: https://github.com/libsdl-org/SDL/projects
- SDL_GPU documentation
- Emscripten WebGPU support status
- Browser WebGPU implementation status

**Key milestones to track:**
- ✅ SDL_GPU API stabilization
- ⏳ Emscripten SDL3 support
- ⏳ SDL_GPU WebGPU backend implementation
- ⏳ Browser WebGPU availability (Chrome stable, Firefox, Safari)

### Migration Strategy

**When SDL3 WebGPU is ready:**

1. **Test upstream runtime with Emscripten**
   ```bash
   emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
   emmake make
   ```

2. **Verify compatibility**
   - Does flashbang.c compile with Emscripten?
   - Does SDL_GPU map to WebGPU?
   - Do shaders compile (SPIR-V → WGSL)?

3. **Deprecate custom backends**
   - Keep Canvas2D as fallback
   - Remove WebGL2 (if implemented)
   - Use native flashbang.c for WASM

4. **Build configuration**
   ```cmake
   if(EMSCRIPTEN)
       # Same sources as native!
       set(RENDER_SOURCES
           ${PROJECT_SOURCE_DIR}/src/flashbang/flashbang.c
       )
       set_target_properties(${PROJECT_NAME} PROPERTIES
           LINK_FLAGS "-s USE_SDL=3 -s USE_WEBGPU=1"
       )
   endif()
   ```

### Expected Benefits

- ✅ **Zero maintenance:** Same code as upstream
- ✅ **Automatic updates:** Benefit from LittleCube's improvements
- ✅ **Full features:** All native features available in WASM
- ✅ **Performance:** Native-like GPU acceleration

### Deliverables

- [ ] SDL3 WebGPU compatibility test
- [ ] Migration guide
- [ ] Updated build system
- [ ] Deprecation plan for custom backends
- [ ] Performance comparison (Canvas2D vs WebGL2 vs WebGPU)

---

## Maintaining Native Runtime Compatibility

### Core Principle

**Native builds must remain 100% unaffected by WASM changes.**

### Implementation Strategy

#### 1. Preprocessor Guards

```c
// GOOD: Platform-specific code clearly marked
#ifdef __EMSCRIPTEN__
    // WASM-specific code
    #include <emscripten.h>
    void wasm_function() { ... }
#else
    // Native code
    void native_function() { ... }
#endif

// BAD: Mixing platform code
void mixed_function() {
    if (is_wasm) { ... }  // Runtime check - adds overhead to native
}
```

#### 2. File Separation

```
src/rendering/
├── render_api.h          # Shared interface
├── render_native.c       # Native only (#ifndef __EMSCRIPTEN__)
├── render_canvas2d.c     # WASM only (#ifdef __EMSCRIPTEN__)
└── render_webgl2.c       # WASM only (#ifdef __EMSCRIPTEN__)
```

#### 3. Build System Isolation

```cmake
# Native build MUST NOT be affected by WASM options
if(NOT EMSCRIPTEN)
    # Native build - unchanged from upstream
    add_subdirectory(lib/SDL3)
    target_link_libraries(${PROJECT_NAME} SDL3::SDL3)
    # ... existing configuration ...
endif()

# WASM build - completely separate
if(EMSCRIPTEN)
    # WASM-specific configuration
    # ... WASM options ...
endif()
```

#### 4. Test Coverage

**Before every commit:**

```bash
# Test native build
mkdir build-native && cd build-native
cmake .. && make
./TestSWFRecompiled
cd ..

# Test WASM build
mkdir build-wasm && cd build-wasm
emcmake cmake .. && emmake make
# Open .html in browser
cd ..
```

### Continuous Integration

**GitHub Actions workflow:**

```yaml
# .github/workflows/ci.yml
name: CI

on: [push, pull_request]

jobs:
  native-build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive
      - name: Build native
        run: |
          mkdir build && cd build
          cmake ..
          make -j$(nproc)
      - name: Run tests
        run: |
          cd build
          ctest --output-on-failure

  wasm-build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive
      - uses: mymindstorm/setup-emsdk@v11
      - name: Build WASM
        run: |
          mkdir build-wasm && cd build-wasm
          emcmake cmake ..
          emmake make
      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: wasm-build
          path: build-wasm/*.{html,js,wasm}
```

---

## Build System Design

### Directory Structure

```
SWFRecomp/
├── SWFRecomp/                  # Unchanged - recompiler tool (native only)
│   ├── src/
│   ├── CMakeLists.txt
│   └── build/
│
└── SWFModernRuntime/           # Modified - add WASM support
    ├── src/
    │   ├── libswf/             ✅ WASM-compatible (no changes)
    │   ├── actionmodern/       ✅ WASM-compatible (no changes)
    │   ├── flashbang/          ⚠️ Native only
    │   │   └── flashbang.c
    │   └── rendering/          🆕 NEW - abstraction layer
    │       ├── render_api.h
    │       ├── render_native.c
    │       ├── render_canvas2d.c
    │       └── render_webgl2.c
    ├── shaders/
    │   ├── spirv/              # Native shaders
    │   └── glsl/               # WASM shaders (WebGL2)
    ├── web/                    🆕 NEW
    │   ├── shell.html          # HTML template
    │   └── style.css           # Styling
    ├── CMakeLists.txt          ⚠️ Modified
    ├── build-native/           # Native builds
    └── build-wasm/             # WASM builds
```

### CMake Organization

```cmake
# SWFModernRuntime/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(SWFModernRuntime)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ============================================================================
# Core sources (platform-agnostic)
# ============================================================================
set(CORE_SOURCES
    ${PROJECT_SOURCE_DIR}/src/libswf/swf.c
    ${PROJECT_SOURCE_DIR}/src/libswf/tag.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/action.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/variables.c
    ${PROJECT_SOURCE_DIR}/src/utils.c
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap/map.c
)

# ============================================================================
# Platform-specific rendering backend
# ============================================================================
if(EMSCRIPTEN)
    message(STATUS "=== Building for WebAssembly ===")

    # WASM rendering options
    option(USE_WEBGL2 "Use WebGL2 instead of Canvas2D" OFF)

    if(USE_WEBGL2)
        message(STATUS "Rendering backend: WebGL2")
        set(RENDER_SOURCES ${PROJECT_SOURCE_DIR}/src/rendering/render_webgl2.c)
        set(RENDER_FLAGS "-s USE_WEBGL2=1 -s FULL_ES3=1")
    else()
        message(STATUS "Rendering backend: Canvas2D")
        set(RENDER_SOURCES ${PROJECT_SOURCE_DIR}/src/rendering/render_canvas2d.c)
        set(RENDER_FLAGS "")
    endif()

    set(SOURCES ${CORE_SOURCES} ${RENDER_SOURCES})

    # WASM-specific build flags
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set(EMSCRIPTEN_LINK_FLAGS "\
        -s WASM=1 \
        -s ALLOW_MEMORY_GROWTH=1 \
        -s EXPORTED_FUNCTIONS='[\"_main\"]' \
        -s EXPORTED_RUNTIME_METHODS='[\"cwrap\",\"ccall\"]' \
        -s MODULARIZE=1 \
        -s EXPORT_NAME='SWFRecompiledModule' \
        --shell-file ${PROJECT_SOURCE_DIR}/web/shell.html \
        ${RENDER_FLAGS} \
    ")

else()
    message(STATUS "=== Building for Native ===")
    message(STATUS "Rendering backend: SDL_GPU (Vulkan/Metal/D3D12)")

    # Native rendering (unchanged from upstream)
    set(RENDER_SOURCES
        ${PROJECT_SOURCE_DIR}/src/rendering/render_native.c
        ${PROJECT_SOURCE_DIR}/src/flashbang/flashbang.c
    )

    set(SOURCES ${CORE_SOURCES} ${RENDER_SOURCES})

    # Native dependencies (unchanged)
    add_subdirectory(${PROJECT_SOURCE_DIR}/lib/zlib)
    add_subdirectory(${PROJECT_SOURCE_DIR}/lib/lzma)
    add_subdirectory(${PROJECT_SOURCE_DIR}/lib/SDL3)

endif()

# ============================================================================
# Target configuration
# ============================================================================
add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/actionmodern
    ${PROJECT_SOURCE_DIR}/include/libswf
    ${PROJECT_SOURCE_DIR}/include/flashbang
    ${PROJECT_SOURCE_DIR}/include/rendering
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap
)

if(NOT EMSCRIPTEN)
    # Native-only includes
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/lib/SDL3/include
        zlib
        lzma/liblzma/api
    )

    # Native-only linking
    target_link_libraries(${PROJECT_NAME} PUBLIC
        zlibstatic
        lzma
        SDL3::SDL3
    )
endif()

# Platform-specific compiler options
if(EMSCRIPTEN)
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wno-format-truncation
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}"
    )
else()
    if(WIN32)
        target_compile_options(${PROJECT_NAME} PRIVATE)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-format-truncation)
    endif()
endif()

# ============================================================================
# Installation (native only)
# ============================================================================
if(NOT EMSCRIPTEN)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "$\{ORIGIN\}")
endif()
```

### Build Scripts

Create helper scripts for common build tasks:

```bash
# scripts/build-native.sh
#!/bin/bash
set -e

echo "=== Building Native Runtime ==="

mkdir -p build-native
cd build-native

cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

echo "✓ Native build complete: build-native/libSWFModernRuntime.a"
```

```bash
# scripts/build-wasm.sh
#!/bin/bash
set -e

echo "=== Building WASM Runtime ==="

# Check if Emscripten is available
if ! command -v emcc &> /dev/null; then
    echo "Error: Emscripten not found. Please install and activate emsdk."
    exit 1
fi

mkdir -p build-wasm
cd build-wasm

# Option: USE_WEBGL2=ON for WebGL2, OFF for Canvas2D
emcmake cmake -DCMAKE_BUILD_TYPE=Release -DUSE_WEBGL2=OFF ..
emmake make

echo "✓ WASM build complete:"
echo "  - build-wasm/TestSWFRecompiled.html"
echo "  - build-wasm/TestSWFRecompiled.js"
echo "  - build-wasm/TestSWFRecompiled.wasm"
echo ""
echo "To test: python3 -m http.server -d build-wasm 8000"
echo "Then open: http://localhost:8000/TestSWFRecompiled.html"
```

```bash
# scripts/test-both.sh
#!/bin/bash
set -e

echo "=== Testing Both Platforms ==="

# Build native
./scripts/build-native.sh

# Build WASM
./scripts/build-wasm.sh

# Run native tests
echo ""
echo "=== Running Native Tests ==="
cd build-native
ctest --output-on-failure
cd ..

# Instructions for WASM tests
echo ""
echo "=== WASM Tests (Manual) ==="
echo "To test WASM build:"
echo "  1. cd build-wasm"
echo "  2. python3 -m http.server 8000"
echo "  3. Open http://localhost:8000/TestSWFRecompiled.html"
echo "  4. Check browser console for output"
```

### Test Infrastructure

```cmake
# tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Common for both platforms
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledTags/tagMain.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledTags/constants.c
    ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledTags/draws.c
)

# Link against runtime
add_executable(TestSWFRecompiled ${TEST_SOURCES})
target_link_libraries(TestSWFRecompiled SWFModernRuntime)
target_include_directories(TestSWFRecompiled PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/RecompiledTags
)

if(NOT EMSCRIPTEN)
    # Native: Add to CTest
    enable_testing()
    add_test(NAME two_squares COMMAND TestSWFRecompiled)
endif()
```

---

## Testing Strategy

### Test Matrix

| Test Type | Native Build | WASM Canvas2D | WASM WebGL2 | WASM WebGPU |
|-----------|--------------|---------------|-------------|-------------|
| **Compilation** | ✅ Required | ✅ Required | ⏳ Phase 2 | ⏳ Phase 3 |
| **ActionScript Tests (50)** | ✅ All pass | ✅ All pass | ✅ All pass | ✅ All pass |
| **Graphics Tests (14)** | ✅ All pass | ⚠️ Basic | ✅ All pass | ✅ All pass |
| **Performance** | ✅ Baseline | ⚠️ Slow | ✅ Good | ✅ Excellent |

### Test Cases

#### 1. Compilation Tests

```bash
# Native
cd build-native && cmake .. && make
# Expected: Clean compile, no warnings

# WASM Canvas2D
cd build-wasm && emcmake cmake -DUSE_WEBGL2=OFF .. && emmake make
# Expected: Clean compile, WASM binary generated

# WASM WebGL2 (Phase 2)
cd build-wasm && emcmake cmake -DUSE_WEBGL2=ON .. && emmake make
# Expected: Clean compile, shaders embedded
```

#### 2. ActionScript Tests

**Test:** `trace_swf_4`

```bash
# Native
./build-native/TestSWFRecompiled
# Expected stdout: "sup from SWF 4"

# WASM
# Open TestSWFRecompiled.html in browser
# Check console: "sup from SWF 4"
```

**Coverage:** All 50 ActionScript tests must produce identical output in native and WASM.

#### 3. Graphics Tests

**Test:** `two_squares`

**Native:**
- Window opens
- Two colored squares visible
- Correct colors (red, green)
- Correct positions

**WASM Canvas2D (Phase 1):**
- Canvas appears
- ⚠️ Squares may be simplified/approximate
- Colors roughly correct
- Positions roughly correct

**WASM WebGL2 (Phase 2):**
- Canvas appears
- Pixel-perfect rendering
- Matches native output
- Smooth animation

#### 4. Performance Benchmarks

**Test:** `speed_test_swf_4` (if exists) or create benchmark

| Platform | Frame Time | FPS | Notes |
|----------|-----------|-----|-------|
| Native | 1-2ms | 500+ | Baseline |
| WASM Canvas2D | 20-50ms | 20-50 | CPU limited |
| WASM WebGL2 | 2-5ms | 200+ | GPU accelerated |
| WASM WebGPU | 1-3ms | 300+ | Native-like |

#### 5. Regression Testing

**Before every merge to main:**

```bash
# Run full test suite on both platforms
./scripts/test-both.sh

# Check for:
# - Native build still works (no regressions)
# - WASM build compiles
# - ActionScript tests pass on both
# - Graphics rendering works on both
```

### Automated Testing

**GitHub Actions:**
- Run on every push/PR
- Test both native and WASM builds
- Generate screenshots for visual comparison
- Upload WASM build as artifact for manual testing

### Manual Testing Checklist

**Before releasing:**

- [ ] Native build compiles cleanly
- [ ] WASM build compiles cleanly
- [ ] All 50 ActionScript tests pass (native)
- [ ] All 50 ActionScript tests pass (WASM - check browser console)
- [ ] Basic graphics render (two_squares)
- [ ] No console errors in browser
- [ ] File size reasonable (<2MB for WASM binary)
- [ ] Load time acceptable (<3 seconds on fast connection)

---

## Upstream Sync Strategy

### Tracking Upstream

**Git remotes:**
```bash
# Add upstream
git remote add upstream-swfrecomp https://github.com/SWFRecomp/SWFRecomp.git
git remote add upstream-runtime https://github.com/SWFRecomp/SWFModernRuntime.git

# Fetch updates
git fetch upstream-swfrecomp
git fetch upstream-runtime
```

### Merge Strategy

**Weekly sync:**
```bash
# Check for upstream changes
git fetch upstream-runtime

# Merge into your wasm-support branch
git checkout wasm-support
git merge upstream-runtime/master

# Resolve conflicts (if any)
# Priority: Keep WASM compatibility while adopting upstream improvements
```

### Conflict Resolution Guidelines

**Common conflict scenarios:**

1. **Upstream modifies flashbang.c:**
   - ✅ Keep upstream changes in `flashbang.c`
   - ✅ Update `render_native.c` wrapper if needed
   - ❌ Don't modify upstream `flashbang.c`

2. **Upstream modifies swf.c:**
   - ✅ Adopt upstream changes
   - ✅ Update render API calls if needed
   - ⚠️ Test both native and WASM builds

3. **Upstream adds new dependencies:**
   - ✅ Check if dependency works with Emscripten
   - ✅ Add preprocessor guards if needed
   - ⚠️ May need WASM alternative

### Contributing Back Upstream

**Candidates for upstream PRs:**

✅ **Submit to LittleCube:**
- Bug fixes in core runtime
- Performance improvements (platform-agnostic)
- Documentation improvements
- Test cases

❌ **Keep in fork:**
- WASM-specific code (unless requested)
- Rendering abstraction layer
- Emscripten build configuration
- Canvas2D/WebGL2 backends

**Communication:**
- Inform LittleCube about fork
- Share progress updates
- Ask about API stability needs
- Coordinate on breaking changes

---

## Timeline & Milestones

### Phase 1: Canvas2D Prototype

**Setup & Abstraction**
- [ ] Fork repositories, set up build environment
- [ ] Create rendering abstraction layer (`render_api.h`)
- [ ] Implement `render_native.c` wrapper

**WASM Implementation**
- [ ] Implement `render_canvas2d.c`
- [ ] Configure Emscripten build system
- [ ] Test compilation, fix errors

**Testing & Refinement**
- [ ] Test `trace_swf_4` (ActionScript)
- [ ] Test `two_squares` (graphics)
- [ ] Fix rendering issues
- [ ] Documentation

**Milestone 1:** ✅ WASM builds compile and run basic tests

---

### Phase 2: WebGL2 Backend (Optionals)

**Decision Point:** End of Phase 1
- Evaluate: Is SDL3 WebGPU ready?
- If NO and need performance: Proceed with Phase 2
- If YES or acceptable performance: Skip to Phase 3

**WebGL2 Setup**
- [ ] WebGL2 context creation
- [ ] Buffer management
- [ ] Basic rendering pipeline

**Shader Porting**
- [ ] Port vertex shaders
- [ ] Port fragment shaders
- [ ] Shader compilation system

**Features**
- [ ] Solid fills
- [ ] Gradients
- [ ] Bitmaps
- [ ] Transforms

**Testing & Optimization**
- [ ] All graphics tests
- [ ] Performance profiling
- [ ] Bundle size optimization

**Milestone 2:** ✅ GPU-accelerated rendering in WASM

---

### Phase 3: SDL3 WebGPU Migration

**Timeline:** TBD (dependent on SDL3)

**Monitoring Phase (ongoing):**
- [ ] Monthly check: SDL3 WebGPU status
- [ ] Test Emscripten + SDL3 compatibility
- [ ] Follow SDL development blog/GitHub

**Migration Phase (when ready):**
- [ ] Test upstream runtime with Emscripten
- [ ] Migrate build system
- [ ] Testing and validation
- [ ] Documentation and deprecation

**Milestone 3:** ✅ Using upstream runtime for both native and WASM

---

## Risk Management

### Technical Risks

#### Risk 1: Emscripten Compatibility Issues
**Probability:** Medium
**Impact:** High
**Mitigation:**
- Test early and often
- Use stable Emscripten version
- Avoid bleeding-edge features
- Have fallback to Canvas2D

#### Risk 2: SDL3 WebGPU Delayed
**Probability:** High
**Impact:** Medium
**Mitigation:**
- ✅ Phase 1 provides working solution
- ✅ Phase 2 provides fallback
- ⏳ No hard dependency on SDL3 timeline

#### Risk 3: Upstream Breaking Changes
**Probability:** High (active development)
**Impact:** Medium
**Mitigation:**
- Weekly upstream syncs
- Good abstraction layer
- Comprehensive test suite
- Direct communication with LittleCube

#### Risk 4: Performance Issues
**Probability:** Medium
**Impact:** Medium
**Mitigation:**
- Phase 1: Accept poor performance (proof of concept)
- Phase 2: GPU acceleration solves most issues
- Profile and optimize hot paths
- Consider WebAssembly SIMD if needed

#### Risk 5: Browser Compatibility
**Probability:** Low
**Impact:** Low
**Mitigation:**
- Target modern browsers (last 2 years)
- Use standard APIs (WebGL2, WebGPU)
- Feature detection and fallbacks
- Document minimum browser versions

### Project Risks

#### Risk 1: Scope Creep
**Probability:** Medium
**Impact:** Medium
**Mitigation:**
- Clear phase boundaries
- Stick to plan
- Phase 1 is minimal viable product
- Phase 2 is truly optional

#### Risk 2: Maintenance Burden
**Probability:** High
**Impact:** High
**Mitigation:**
- ✅ Minimize custom code (rendering backends only)
- ✅ Automate testing (CI/CD)
- ✅ Good documentation
- ✅ Plan for Phase 3 migration (reduce to zero custom code)

#### Risk 3: Upstream Divergence
**Probability:** Medium
**Impact:** High
**Mitigation:**
- Regular syncs
- Clean abstraction prevents conflicts
- Communicate with LittleCube
- Be prepared to abandon fork if necessary

### Contingency Plans

**If Phase 1 fails:**
- Reassess Emscripten compatibility
- Consider alternative: asm.js fallback
- Consider alternative: Wait for SDL3 only

**If Phase 2 is too complex:**
- Skip to Phase 3 (wait for SDL3)
- Use Canvas2D as interim solution
- Focus on non-graphics features

**If SDL3 WebGPU never materializes:**
- Maintain WebGL2 backend long-term
- Consider contributing WebGPU backend to SDL3
- Accept maintenance burden

**If upstream rejects fork concept:**
- Continue as independent project
- Manual upstream merges
- Fork becomes permanent

---

## Success Metrics

### Technical Metrics

**Phase 1 Success:**
- [ ] Native build: 100% tests pass (no regressions)
- [ ] WASM build: Compiles without errors
- [ ] WASM build: 50/50 ActionScript tests pass
- [ ] WASM build: Basic rendering visible
- [ ] Bundle size: <5MB total (HTML + JS + WASM)

**Phase 2 Success (if pursued):**
- [ ] WASM build: 14/14 graphics tests pass
- [ ] WASM build: Rendering matches native (visual comparison)
- [ ] Performance: >30 FPS for test games
- [ ] Bundle size: <2MB total

**Phase 3 Success:**
- [ ] Using upstream runtime unmodified
- [ ] Zero custom rendering code
- [ ] Native and WASM builds identical source
- [ ] Performance: Native-like

### Project Metrics

- [ ] Documentation complete and up-to-date
- [ ] CI/CD pipeline functional
- [ ] At least 1 complete game ported to WASM
- [ ] Positive feedback from community
- [ ] No negative impact on upstream project

---

## Next Steps

### Immediate Actions

1. **Environment Setup**
   ```bash
   # Fork repositories
   # Install Emscripten
   # Set up git remotes
   # Create wasm-support branch
   ```

2. **Create Abstraction Layer**
   ```bash
   # Create src/rendering/ directory
   # Write render_api.h
   # Implement render_native.c wrapper
   # Test native build still works
   ```

3. **Implement Canvas2D**
   ```bash
   # Write render_canvas2d.c
   # Create web/shell.html template
   # Test basic Emscripten compilation
   ```

4. **First WASM Build**
   ```bash
   # Configure CMakeLists.txt for WASM
   # Attempt full build
   # Fix compilation errors
   # Get *something* running in browser
   ```

### Decision Points

- ✅ Does abstraction layer work?
- ✅ Does native build still work?
- ✅ Is approach viable?

- ✅ Does WASM compile?
- ✅ Does Canvas2D render anything?
- ✅ Continue?

- ✅ Is Canvas2D good enough?
- ✅ Proceed to Phase 2 or wait for SDL3?

---

## Appendix A: Technology Reference

### Emscripten

**Version:** Latest stable (3.1.x as of Oct 2025)
**Documentation:** https://emscripten.org/docs/
**Installation:**
```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

### WebGL2

**Browser Support:** All modern browsers (2020+)
**Reference:** https://developer.mozilla.org/en-US/docs/Web/API/WebGL_API
**GLSL ES 3.0:** https://www.khronos.org/files/opengles_shading_language.pdf

### WebGPU

**Browser Support:** Chrome 113+, Edge 113+, (Firefox/Safari experimental)
**Reference:** https://gpuweb.github.io/gpuweb/
**Status:** https://caniuse.com/webgpu

### SDL3

**Repository:** https://github.com/libsdl-org/SDL
**SDL_GPU Documentation:** https://wiki.libsdl.org/SDL3/CategoryGPU
**Emscripten Support:** https://wiki.libsdl.org/SDL3/README/emscripten

---

## Appendix B: File Templates

### shell.html Template

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SWF Recompiled</title>
    <style>
        body {
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            background: #1a1a1a;
            font-family: sans-serif;
            color: #fff;
        }
        #container {
            text-align: center;
        }
        #canvas {
            border: 2px solid #333;
            background: #000;
            image-rendering: pixelated;
            image-rendering: crisp-edges;
        }
        #status {
            margin-top: 1em;
            font-size: 0.9em;
            color: #888;
        }
        #output {
            margin-top: 1em;
            padding: 1em;
            background: #000;
            border: 1px solid #333;
            text-align: left;
            font-family: monospace;
            font-size: 0.8em;
            max-height: 200px;
            overflow-y: auto;
        }
    </style>
</head>
<body>
    <div id="container">
        <h1>SWF Recompiled to WASM</h1>
        <canvas id="canvas"></canvas>
        <div id="status">Loading...</div>
        <div id="output"></div>
    </div>

    <script>
        var Module = {
            preRun: [],
            postRun: [],
            print: function(text) {
                console.log(text);
                var output = document.getElementById('output');
                output.innerHTML += text + '\n';
                output.scrollTop = output.scrollHeight;
            },
            printErr: function(text) {
                console.error(text);
                var output = document.getElementById('output');
                output.innerHTML += '<span style="color: #f88">' + text + '</span>\n';
                output.scrollTop = output.scrollHeight;
            },
            setStatus: function(text) {
                document.getElementById('status').textContent = text;
            },
            canvas: document.getElementById('canvas')
        };
    </script>
    {{{ SCRIPT }}}
</body>
</html>
```

### .gitignore additions

```gitignore
# WASM builds
build-wasm/
*.wasm
*.wat

# Emscripten cache
.emscripten_cache/
.emscripten_ports/
```

---

### GitHub README Addition

```markdown
## WebAssembly Support (Fork)

This fork adds WebAssembly (WASM) compilation support while maintaining full native runtime compatibility.

### Quick Start (WASM)

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh

# Build for WASM
cd SWFModernRuntime
./scripts/build-wasm.sh

# Test in browser
cd build-wasm
python3 -m http.server 8000
# Open http://localhost:8000/TestSWFRecompiled.html
```

### Current Status

- ✅ Phase 1: Canvas2D rendering (working)
- ⏳ Phase 2: WebGL2 rendering (optional)
- ⏳ Phase 3: SDL3 WebGPU (waiting on SDL3)

### Upstream Sync

This fork tracks upstream regularly. All native functionality remains unchanged.

**Upstream:** https://github.com/SWFRecomp/SWFModernRuntime
```

---

## Current Implementation Status (October 27, 2025)

### What's Working

**✅ ActionScript Execution (Phase 1a - Complete)**
- Full ActionScript VM running in WASM
- String operations, variables, stack management
- Frame-by-frame execution
- Console output via printf/trace
- Example: `trace_swf_4` running at https://peerinfinity.github.io/SWFModernRuntime/

**✅ Build Infrastructure**
- Emscripten compilation working
- Clean build scripts (`build.sh` per example)
- HTML shell templates
- GitHub Pages deployment
- Live demos accessible

**✅ Project Structure**
- Clean separation: `wasm/` directory for all WASM code
- Minimal merge conflicts with upstream
- Documentation in place

### What's Next

**Immediate Priorities (Phase 1b - Rendering):**

1. **Create Rendering Abstraction Layer**
   - Design `render_api.h` interface
   - Align with upstream's gradient/bitmap support
   - Plan for texture_info (not just color_info)
   - Support display lists and transforms

2. **Implement Canvas2D Backend (Minimal)**
   - Basic shape rendering
   - Solid color fills
   - Simple transforms
   - Get `mess` test rendering *something*

3. **Test Graphics Examples**
   - Port `mess` test to WASM
   - Port `wild_shadow` test
   - Visual comparison with native
   - Document limitations

4. **Evaluate Phase 2 Transition**
   - Is Canvas2D sufficient?
   - Or proceed directly to WebGL2?
   - Document decision rationale

### Technical Debt to Address

1. **Upstream Architecture Alignment**
   - Current runtime.c is minimal/standalone
   - Need to integrate with upstream display list architecture
   - Character and DisplayObject structs now in upstream
   - Transform system has been refactored

2. **Feature Gap Analysis**
   - Gradients (linear + radial) - upstream has this
   - Bitmaps - upstream has this
   - MSAA - may skip for WASM
   - Compute shaders - need WebGL2 compute or workaround

3. **Build System Integration**
   - Current: standalone examples with shell scripts
   - Future: integrate with main CMake system
   - Need: EMSCRIPTEN build target in root CMakeLists.txt

### Recommended Adjustments to Plan

**Strategic Shifts:**

1. **Phase 1 → Phase 2 Faster**
   - Original plan: Canvas2D proof-of-concept, then evaluate
   - New reality: Upstream rendering is sophisticated
   - Recommendation: Get Canvas2D working minimally, then prioritize WebGL2

2. **Feature Parity Focus**
   - Original goal: "basic rendering"
   - New goal: Match upstream gradient/bitmap support
   - Reason: Test suite expects these features

3. **Performance Bar Raised**
   - Original: Canvas2D acceptable for proof-of-concept
   - New: Complex games need GPU acceleration
   - Impact: WebGL2 now essential, not optional

**Timeline Adjustments:**

- **Phase 1a (ActionScript):** COMPLETE ✅
- **Phase 1b (Canvas2D):**
- **Phase 2 (WebGL2):** NOW RECOMMENDED (was: Optional)
  - Priority: High

### Open Questions

1. **Architecture Decision:**
   - Should WASM runtime use upstream's display list code?
   - Or keep standalone runtime for simplicity?
   - Trade-off: Code reuse vs. complexity

2. **Rendering Backend Strategy:**
   - Canvas2D minimal → WebGL2 full?
   - Or skip Canvas2D, go directly to WebGL2?
   - Current lean: Canvas2D minimal first for incremental progress

3. **Compute Shader Workaround:**
   - Upstream uses compute shader for gradient matrix inversion
   - WebGL2 doesn't have compute shaders
   - Options:
     - Pre-compute on CPU (WASM)
     - Use WebGL2 fragment shader workaround
     - Wait for WebGPU

4. **Test Suite:**
   - Which tests should WASM target?
   - Focus on working upstream tests (mess, wild_shadow, gradients)
   - Or fix ActionScript tests for WASM compatibility?

### Success Metrics (Updated)

**Phase 1 Success (Revised):**
- [x] ActionScript VM working in WASM
- [x] Build system and deployment pipeline
- [ ] Canvas2D rendering showing *something* (even if imperfect)
- [ ] At least 1 graphics test rendered in browser
- [ ] Documentation of limitations and next steps

**Phase 2 Success (Elevated Priority):**
- [ ] WebGL2 backend with shader support
- [ ] Gradients working (linear + radial)
- [ ] Bitmaps/textures working
- [ ] All upstream graphics tests pass visually
- [ ] Performance: 30+ FPS for test games
- [ ] Bundle size: <2MB

## Document History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-10-27 | Initial planning document |
| 1.1 | 2025-10-27 | Updated with current implementation status, upstream rendering progress, adjusted priorities and timelines |

---

*End of Planning Document*
