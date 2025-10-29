# C vs C++ Architecture in SWFRecomp

**Document Version:** 1.0
**Date:** October 28, 2025
**Status:** Architecture Documentation

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [The Core Principle](#the-core-principle)
3. [Current Architecture Analysis](#current-architecture-analysis)
4. [Where C++ Makes Sense](#where-c-makes-sense)
5. [Where C Makes Sense](#where-c-makes-sense)
6. [Real-World Examples from the Codebase](#real-world-examples-from-the-codebase)
7. [Future AS3 Implementation](#future-as3-implementation)
8. [Performance & Binary Size Analysis](#performance--binary-size-analysis)
9. [Best Practices](#best-practices)
10. [References](#references)

---

## Executive Summary

SWFRecomp already uses an **optimal hybrid C/C++ architecture** based on a simple principle:

> **Use C++ for build-time tools. Use C for runtime code.**

This document explains why this architecture is optimal, provides concrete examples from the codebase, and offers guidance for future development (including AS3 support).

### Key Takeaways

- ✅ **SWFRecomp (recompiler)**: C++17 - Complex algorithms, STL containers, third-party libraries
- ✅ **Generated code**: Pure C - Small, fast, predictable
- ✅ **SWFModernRuntime**: C17 - Minimal overhead, maximum performance
- ✅ **Future AS3 runtime**: Should stay pure C (per LittleCube's guidance)

**Why this works:**
- Build-time tools run once, can use heavy C++ features
- Runtime code runs repeatedly, needs minimal overhead
- Binary size only matters for runtime (especially WASM)
- Complexity in Flash semantics, not in data structures

---

## The Core Principle

### Build-Time vs Runtime

```
┌────────────────────────────────────────────────────────────┐
│                      BUILD TIME                            │
│  (Runs once per SWF file, on developer's machine)         │
│                                                            │
│  ┌──────────────────────────────────────────────┐         │
│  │  SWFRecomp (C++)                             │         │
│  │  - Parse SWF format                          │         │
│  │  - Decompress (zlib, LZMA)                   │         │
│  │  - Parse geometry (shapes, gradients)        │         │
│  │  - Triangulate polygons (earcut)             │         │
│  │  - Detect cycles in paths (Johnson's algo)   │         │
│  │  - Translate ActionScript → C               │         │
│  │  - Generate C source files                   │         │
│  │                                              │         │
│  │  Uses: std::vector, std::unordered_map,      │         │
│  │        std::string, std::stringstream,       │         │
│  │        tomlplusplus, earcut.hpp              │         │
│  └──────────────────────────────────────────────┘         │
│                          │                                 │
│                          │ Outputs                         │
│                          ▼                                 │
│  ┌──────────────────────────────────────────────┐         │
│  │  Generated C Files                           │         │
│  │  - script_0.c, script_1.c, ...              │         │
│  │  - tagMain.c                                 │         │
│  │  - draws.c                                   │         │
│  │  - constants.c                               │         │
│  └──────────────────────────────────────────────┘         │
└────────────────────────────────────────────────────────────┘
                          │
                          │ Compiles & Links
                          ▼
┌────────────────────────────────────────────────────────────┐
│                      RUNTIME                               │
│  (Runs every time user plays the game/animation)          │
│                                                            │
│  ┌──────────────────────────────────────────────┐         │
│  │  Executable / WASM                           │         │
│  │  ┌────────────────────────────────────────┐  │         │
│  │  │  Generated C Code                      │  │         │
│  │  │  - Pure C17                            │  │         │
│  │  │  - No STL, no C++ runtime              │  │         │
│  │  │  - Minimal overhead                    │  │         │
│  │  └────────────────────────────────────────┘  │         │
│  │                                              │         │
│  │  ┌────────────────────────────────────────┐  │         │
│  │  │  SWFModernRuntime (C)                  │  │         │
│  │  │  - Stack machine (AS1/2)               │  │         │
│  │  │  - Vulkan rendering                    │  │         │
│  │  │  - Action implementations              │  │         │
│  │  │  - SDL3 integration                    │  │         │
│  │  └────────────────────────────────────────┘  │         │
│  └──────────────────────────────────────────────┘         │
│                                                            │
│  Binary size: ~500KB (WASM)                                │
│  Startup time: <100ms                                      │
│  Frame time: <16ms (60 FPS)                                │
└────────────────────────────────────────────────────────────┘
```

### Why This Separation Matters

**Build-Time (C++):**
- Runs once per SWF file during development
- Binary size doesn't matter (it's a dev tool)
- Complexity is in algorithms (geometry, parsing, code generation)
- C++ STL helps manage complex data structures
- Can use heavy third-party libraries
- Developer productivity matters more than runtime performance

**Runtime (C):**
- Runs every time the end-user plays the game/animation
- Binary size critically important (especially WASM)
- Performance critically important (60 FPS target)
- Complexity is in Flash semantics, not data structures
- Simple data structures (arrays, structs) are sufficient
- No need for C++ overhead (vtables, RTTI, exceptions)

---

## Current Architecture Analysis

### SWFRecomp - The Recompiler (C++17)

**Location:** `src/*.cpp`, `include/*.hpp`

**Language:** C++17

**Key Dependencies:**
```cpp
#include <vector>           // Dynamic arrays
#include <unordered_map>    // Hash tables
#include <string>           // String handling
#include <sstream>          // Code generation
#include <algorithm>        // STL algorithms
#include <array>            // Fixed-size arrays

#include <earcut.hpp>       // Polygon triangulation
#include <tomlplusplus>     // TOML config parsing
#include <stb_image.h>      // Image loading
#include <zlib.h>           // Decompression
#include <lzma.h>           // LZMA decompression
```

**What It Does:**
1. **Parse SWF files** - Binary format with variable-length fields
2. **Decompress** - Handle zlib, LZMA, or uncompressed SWFs
3. **Parse geometry** - DefineShape tags with paths, fills, strokes
4. **Triangulate shapes** - Convert Flash vector shapes to GPU triangles
5. **Detect cycles** - Find closed paths in shape graphs (Johnson's algorithm)
6. **Translate ActionScript** - Convert AS1/2 bytecode to C function calls
7. **Generate C code** - Output `.c` files with proper formatting

**Example from `swf.cpp`:**
```cpp
// Complex geometry processing with STL containers
std::vector<Path> paths;
std::vector<Node> nodes;
std::vector<Shape> shapes;
std::unordered_map<Node*, bool> blocked;
std::unordered_map<Node*, std::vector<Node*>> blocked_map;
std::vector<std::vector<Path>> closed_paths;

// Johnson's cycle detection algorithm
void SWF::johnson(std::vector<Node>& nodes,
                  std::vector<Path>& path_stack,
                  std::unordered_map<Node*, bool>& blocked,
                  std::unordered_map<Node*, std::vector<Node*>>& blocked_map,
                  std::vector<std::vector<Path>>& closed_paths)
{
    // Complex graph algorithm implementation
    // Benefits from C++ containers and algorithms
}

// Polygon triangulation
void SWF::fillShape(Shape& shape, std::vector<Tri>& tris)
{
    std::vector<std::vector<std::array<Coord, 2>>> polygon;
    // ... prepare polygon data ...

    // Use earcut library (C++ template library)
    std::vector<N> indices = mapbox::earcut<N>(polygon);

    // ... convert to triangles ...
}
```

**Why C++ is right here:**
- Complex algorithms (Johnson's cycle detection, triangulation)
- Dynamic data structures with unknown sizes
- STL provides tested, optimized implementations
- Code generation benefits from `std::stringstream`
- Third-party libraries (earcut, tomlplusplus) are C++
- This code **never runs in the final game** - only during development

### Generated C Code (Pure C)

**Location:** `tests/*/RecompiledScripts/*.c`, `tests/*/RecompiledTags/*.c`

**Language:** C17

**Example from `trace_swf_4`:**
```c
#include <recomp.h>

// Generated from ActionScript bytecode
void script_0(char* stack, u32* sp)
{
    // Push (String) - opcode 0x96
    PUSH_STR(str_0, 14);

    // Trace - opcode 0x26
    actionTrace(stack, sp);
}

// String constants
const char* str_0 = "sup from SWF 4";
```

**Why pure C is right here:**
- Extremely simple code (just function calls)
- No dynamic memory allocation
- No complex data structures
- Must link with C runtime (SWFModernRuntime)
- This code **runs every frame** in the final game

### SWFModernRuntime - Runtime Library (C17)

**Location:** `SWFModernRuntime/src/`

**Language:** C17

**Key Features:**
```c
// libswf/swf.c - Main runtime loop
void swfStart(FrameFunc* frame_funcs)
{
    // Initialize rendering, event loop
    // Execute frame functions
    // Pure C - no C++ overhead
}

// actionmodern/action.c - ActionScript implementation
void actionTrace(char* stack, u32* sp)
{
    // Pop string from stack
    // Print to console
    // Simple, fast, no allocations
}
```

**Why pure C is right here:**
- Runs in final executable/WASM
- Performance critical (60 FPS target)
- Binary size critical (WASM downloads)
- Simple implementations sufficient
- No need for C++ features

---

## Where C++ Makes Sense

### 1. Build-Time Tools

**✅ Use C++ when:**
- Code runs at build time, not runtime
- Complex algorithms required (graph algorithms, geometry)
- Dynamic data structures with unknown sizes
- Code generation (string manipulation, formatting)
- Parsing complex file formats
- Integration with C++ libraries

**Examples:**
- **SWFRecomp** - Parse SWF, generate C code
- **Config parsing** - TOML files (tomlplusplus)
- **Geometry processing** - Triangulation (earcut)
- **Future: ABC parser** - Parse AS3 bytecode (build-time only)

### 2. Complex Algorithms

**✅ Use C++ when:**
- Algorithm benefits from STL containers
- Performance doesn't matter (runs once)
- Readability and maintainability important

**Example from `swf.cpp` - Johnson's Cycle Detection:**
```cpp
// Finding all cycles in a directed graph
// This is a complex algorithm that benefits from C++ containers

void SWF::johnson(std::vector<Node>& nodes,
                  std::vector<Path>& path_stack,
                  std::unordered_map<Node*, bool>& blocked,
                  std::unordered_map<Node*, std::vector<Node*>>& blocked_map,
                  std::vector<std::vector<Path>>& closed_paths)
{
    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (nodes[i].used)
            continue;

        detectCycle(&nodes[i], path_stack, blocked, blocked_map, closed_paths);

        // Unblock all nodes
        for (auto& pair : blocked)
        {
            pair.second = false;
        }

        // Clear blocked_map
        blocked_map.clear();
    }
}

bool detectCycle(Node* node,
                 std::vector<Path>& path_stack,
                 std::unordered_map<Node*, bool>& blocked,
                 std::unordered_map<Node*, std::vector<Node*>>& blocked_map,
                 std::vector<std::vector<Path>>& closed_paths)
{
    if (node == path_stack[0].back)
    {
        // Found a cycle - copy path_stack to closed_paths
        std::vector<Path> cycle;
        for (const Path& p : path_stack)
        {
            cycle.push_back(p);
        }
        closed_paths.push_back(cycle);
        return true;
    }

    // ... complex graph traversal logic ...
}
```

**Why C++ here:**
- `std::vector` handles dynamic arrays easily
- `std::unordered_map` provides O(1) lookups
- Automatic memory management (no manual malloc/free)
- Range-based for loops improve readability
- This runs once at build time, not in the game

### 3. Code Generation

**✅ Use C++ when:**
- Generating source code
- String manipulation and formatting
- Text processing

**Example from `recompilation.cpp`:**
```cpp
void generateCCode(std::stringstream& out, const ActionScript& script)
{
    out << "#include <recomp.h>\n\n";
    out << "void script_" << script.id << "(char* stack, u32* sp)\n";
    out << "{\n";

    for (const Action& action : script.actions)
    {
        out << "\t// " << action.name << "\n";
        out << "\t" << action.cCode << ";\n";
    }

    out << "}\n";
}
```

**Why C++ here:**
- `std::stringstream` perfect for building strings
- `std::string` handles memory automatically
- Clean, readable code generation
- No buffer overflow risks
- Runs at build time only

### 4. Third-Party C++ Libraries

**✅ Use C++ when:**
- Integrating with C++ libraries
- Library provides significant value
- Build-time usage only

**Current examples:**
```cpp
// Polygon triangulation - earcut.hpp
#include <earcut.hpp>

std::vector<N> indices = mapbox::earcut<N>(polygon);

// TOML config parsing - tomlplusplus
#include <toml++/toml.h>

auto config = toml::parse_file("config.toml");
auto output_dir = config["output"]["directory"].value_or("output");
```

**Why C++ here:**
- These libraries are header-only or build-time only
- Provide significant functionality
- Well-tested, optimized implementations
- No runtime impact (used during build only)

---

## Where C Makes Sense

### 1. Runtime Code

**✅ Use C when:**
- Code runs in final executable/WASM
- Binary size matters
- Performance matters (runs every frame)
- Simple data structures sufficient

**Examples:**
- **Generated code** - ActionScript translations
- **SWFModernRuntime** - Stack machine, action implementations
- **Future AS3 runtime** - Type system, opcodes

### 2. Simple Algorithms

**✅ Use C when:**
- Algorithm is straightforward
- No complex data structures needed
- Performance critical

**Example from `action.c` (hypothetical AS3):**
```c
// Type conversion - runs every frame, needs to be fast
double toNumber(AS3Value* input)
{
    switch (input->type)
    {
        case TYPE_UNDEFINED:
            return NAN;
        case TYPE_NULL:
            return 0.0;
        case TYPE_BOOLEAN:
            return input->value.b ? 1.0 : 0.0;
        case TYPE_NUMBER:
            return input->value.d;
        case TYPE_STRING:
            return parseStringToNumber(input->value.s);
        case TYPE_OBJECT:
        {
            AS3Value* prim = toPrimitive(input, HINT_NUMBER);
            double result = toNumber(prim);
            release(prim);
            return result;
        }
        default:
            return NAN;
    }
}
```

**Why C here:**
- Simple switch statement
- No need for polymorphism
- Direct, fast implementation
- Runs potentially thousands of times per second
- No vtable overhead

### 3. Performance-Critical Code

**✅ Use C when:**
- Hot path code (runs every frame)
- Latency sensitive
- Need predictable performance

**Example from `flashbang.c` (Vulkan rendering):**
```c
void renderFrame(VulkanContext* ctx, DrawList* draws)
{
    // Direct function calls, no virtual dispatch
    // Predictable performance
    // No hidden allocations

    vkCmdBeginRenderPass(ctx->cmd_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    for (uint32_t i = 0; i < draws->count; i++)
    {
        Draw* draw = &draws->items[i];
        vkCmdBindPipeline(ctx->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, draw->pipeline);
        vkCmdDraw(ctx->cmd_buffer, draw->vertex_count, 1, draw->first_vertex, 0);
    }

    vkCmdEndRenderPass(ctx->cmd_buffer);
}
```

**Why C here:**
- Runs 60 times per second
- No allocations in hot path
- Direct function calls (no vtables)
- Predictable, measurable performance

### 4. Binary Size Sensitive

**✅ Use C when:**
- WASM target (download size matters)
- Embedded systems
- Want minimal runtime dependencies

**Size comparison (estimated):**

| Component | C Implementation | C++ Implementation |
|-----------|------------------|-------------------|
| Runtime library | 430 KB | 800+ KB |
| Generated code | 50 KB | 50 KB (same) |
| C++ runtime | - | 200+ KB (std::string, std::vector, etc.) |
| RTTI/exceptions | - | 50+ KB |
| **Total WASM** | **~500 KB** | **~1100 KB** |

**Why C here:**
- Every KB matters for web delivery
- Faster download, faster startup
- Better mobile experience

### 5. Semantic Complexity

**✅ Use C when:**
- Complexity is in the specification, not the implementation
- Manual implementation required anyway
- C++ doesn't simplify the logic

**Example: The infamous AS3 `add` opcode**

LittleCube's point: This complexity is the same in C and C++:

```c
// ECMA-262 Section 11.6: Additive Operators
AS3Value* add(AS3Value* v1, AS3Value* v2)
{
    // 1. If both are Numbers, add them
    if (isNumber(v1) && isNumber(v2))
    {
        return createNumber(toNumber(v1) + toNumber(v2));
    }
    // 2. If either is String or Date, concatenate as strings
    else if (isString(v1) || isString(v2) || isDate(v1) || isDate(v2))
    {
        char* s1 = toString(v1);
        char* s2 = toString(v2);
        char* result = concat(s1, s2);
        AS3Value* ret = createString(result);
        free(s1); free(s2); free(result);
        return ret;
    }
    // 3. If both are XML or XMLList, create new XMLList
    else if (isXML(v1) && isXML(v2))
    {
        AS3Value* list = createXMLList();
        xmlListAppend(list, v1);
        xmlListAppend(list, v2);
        return list;
    }
    // 4. Otherwise, use ToPrimitive and decide
    else
    {
        AS3Value* p1 = toPrimitive(v1, HINT_NONE);
        AS3Value* p2 = toPrimitive(v2, HINT_NONE);

        if (isString(p1) || isString(p2))
        {
            char* s1 = toString(p1);
            char* s2 = toString(p2);
            char* result = concat(s1, s2);
            AS3Value* ret = createString(result);
            free(s1); free(s2); free(result);
            release(p1); release(p2);
            return ret;
        }
        else
        {
            AS3Value* ret = createNumber(toNumber(p1) + toNumber(p2));
            release(p1); release(p2);
            return ret;
        }
    }
}
```

**The C++ version would look identical** - same logic, same complexity. The only difference would be `std::string` vs `char*`, but you still need to implement `toString()`, `toNumber()`, `ToPrimitive()` manually.

**Why C here:**
- Complexity is in ECMA-262 specification, not in the code
- No polymorphism helps (all types explicitly checked)
- No STL containers help (fixed algorithm)
- C++ just adds overhead without simplifying logic

---

## Real-World Examples from the Codebase

### Example 1: Polygon Triangulation (C++ - Build Time)

**File:** `src/swf.cpp:2173`

**Problem:** Convert Flash vector shapes (curves, holes) to GPU triangles

```cpp
void SWF::fillShape(Shape& shape, std::vector<Tri>& tris)
{
    // Build polygon data structure for earcut
    std::vector<std::vector<std::array<Coord, 2>>> polygon;
    std::vector<std::array<Coord, 2>> shape_array;

    // Add outer boundary
    for (const Vertex& v : shape.verts)
    {
        shape_array.push_back({v.x, v.y});
    }
    polygon.push_back(shape_array);

    // Add holes
    for (Shape* hole : shape.holes)
    {
        std::vector<std::array<Coord, 2>> hole_array;
        for (const Vertex& v : hole->verts)
        {
            hole_array.push_back({v.x, v.y});
        }
        polygon.push_back(hole_array);
    }

    // Triangulate using earcut (C++ template library)
    std::vector<N> indices = mapbox::earcut<N>(polygon);

    // Convert indices to triangles
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Tri tri;
        tri.verts[0] = all_points[indices[i + 0]];
        tri.verts[1] = all_points[indices[i + 1]];
        tri.verts[2] = all_points[indices[i + 2]];
        tris.push_back(tri);
    }
}
```

**Why C++:**
- ✅ Runs at build time (once per SWF)
- ✅ Complex nested data structures (`vector<vector<array>>`)
- ✅ Uses C++ library (earcut)
- ✅ Benefits from automatic memory management
- ✅ Readability with range-based for loops

**Generated output (C):**
```c
// draws.c - Generated triangle data
const Tri triangles_shape_1[] = {
    {{{100, 100}, {200, 100}, {150, 200}}},
    {{{200, 100}, {300, 200}, {150, 200}}},
    // ... more triangles ...
};

const u32 triangle_count_shape_1 = 42;
```

### Example 2: ActionScript Translation (C++ Build, C Runtime)

**File:** `src/action/action.cpp`

**Build-time code (C++):**
```cpp
void Action::recompile(std::stringstream& out_script)
{
    std::vector<char*> labels;

    while (!actions.empty())
    {
        ActionList::iterator action = actions.begin();

        switch (action->code)
        {
            case SWF_ACTION_ADD:
            {
                out_script << "\t// Add\n";
                out_script << "\tactionAdd(stack, sp);\n";
                break;
            }

            case SWF_ACTION_TRACE:
            {
                out_script << "\t// Trace\n";
                out_script << "\tactionTrace(stack, sp);\n";
                break;
            }

            // ... more actions ...
        }

        actions.erase(action);
    }
}
```

**Generated runtime code (C):**
```c
// script_0.c
void script_0(char* stack, u32* sp)
{
    // Add
    actionAdd(stack, sp);
    // Trace
    actionTrace(stack, sp);
}
```

**Runtime implementation (C):**
```c
// action.c
void actionAdd(char* stack, u32* sp)
{
    // Pop two values
    double v2 = popNumber(stack, sp);
    double v1 = popNumber(stack, sp);

    // Add and push result
    pushNumber(stack, sp, v1 + v2);
}

void actionTrace(char* stack, u32* sp)
{
    // Pop string
    char* str = popString(stack, sp);

    // Print
    printf("%s\n", str);

    // Free
    free(str);
}
```

**Why C++ for build, C for runtime:**
- ✅ Build: String manipulation (`std::stringstream`)
- ✅ Build: Dynamic arrays (`std::vector<char*> labels`)
- ✅ Build: Readable, maintainable
- ✅ Runtime: Simple, fast, no overhead
- ✅ Runtime: Links with C runtime library

### Example 3: Cycle Detection (C++ - Build Time)

**File:** `src/swf.cpp:2104`

**Problem:** Find all closed paths in a shape's edge graph (for fill detection)

```cpp
bool detectCycle(Node* node,
                 std::vector<Path>& path_stack,
                 std::unordered_map<Node*, bool>& blocked,
                 std::unordered_map<Node*, std::vector<Node*>>& blocked_map,
                 std::vector<std::vector<Path>>& closed_paths)
{
    // Check if we've completed a cycle
    if (node == path_stack[0].back)
    {
        // Copy path_stack to closed_paths
        std::vector<Path> cycle;
        for (const Path& p : path_stack)
        {
            cycle.push_back(p);
        }
        closed_paths.push_back(cycle);
        return true;
    }

    // Mark node as blocked
    blocked[node] = true;

    bool found_cycle = false;

    // Traverse neighbors
    for (Node* neighbor : node->neighbors)
    {
        if (blocked.find(neighbor) == blocked.end() || !blocked[neighbor])
        {
            // Not blocked - try this path
            path_stack.push_back(neighbor->path);

            if (traverseIteration(neighbor, path_stack, blocked,
                                 blocked_map, closed_paths))
            {
                found_cycle = true;
            }

            path_stack.pop_back();
        }
    }

    if (found_cycle)
    {
        unblock(node, blocked, blocked_map);
    }
    else
    {
        blockInMap(node, blocked_map);
    }

    return found_cycle;
}
```

**Why C++:**
- ✅ Complex graph algorithm (Johnson's algorithm)
- ✅ Nested data structures (`vector<vector<Path>>`)
- ✅ Hash table for O(1) lookups (`unordered_map`)
- ✅ Automatic memory management (no leaks)
- ✅ Runs once at build time
- ✅ Readability important (complex algorithm)

**Alternative C implementation would require:**
- ❌ Manual dynamic array implementation
- ❌ Manual hash table implementation
- ❌ Manual memory management (easy to leak)
- ❌ Much more boilerplate code
- ❌ Harder to debug

**But:** This is fine because it's build-time only!

### Example 4: Config Parsing (C++ - Build Time)

**File:** `src/config.cpp`

```cpp
#include <toml++/toml.h>
#include <string>
#include <string_view>

using std::string;
using std::string_view;

Config parseConfig(const char* path)
{
    Config cfg;

    // Parse TOML file
    auto config = toml::parse_file(path);

    // Extract values
    cfg.input_swf = config["input"]["swf"].value_or("");
    cfg.output_dir = config["output"]["directory"].value_or("output");
    cfg.generate_native = config["output"]["native"].value_or(true);
    cfg.generate_wasm = config["output"]["wasm"].value_or(false);

    return cfg;
}
```

**Why C++:**
- ✅ Uses C++ library (tomlplusplus)
- ✅ Modern, type-safe TOML parsing
- ✅ String handling with `std::string`
- ✅ Build-time only

**Alternative in C:**
- ❌ Would need C TOML library (fewer options, less maintained)
- ❌ Manual string management
- ❌ More error-prone

---

## Future AS3 Implementation

### Build-Time Components (Use C++)

**1. ABC Parser** (`src/abc/abc_parser.cpp`)

**✅ Use C++:**
```cpp
class ABCParser
{
public:
    ABCFile parse(const uint8_t* data, size_t length)
    {
        ABCFile abc;

        // Parse constant pools
        abc.int_pool = parseIntPool(data);
        abc.uint_pool = parseUintPool(data);
        abc.double_pool = parseDoublePool(data);
        abc.string_pool = parseStringPool(data);
        abc.namespace_pool = parseNamespacePool(data);
        abc.multiname_pool = parseMultinamePool(data);

        // Parse methods, classes, scripts
        abc.methods = parseMethodInfo(data);
        abc.classes = parseClassInfo(data);
        abc.scripts = parseScriptInfo(data);
        abc.method_bodies = parseMethodBodyInfo(data);

        return abc;
    }

private:
    std::vector<int32_t> parseIntPool(const uint8_t*& data)
    {
        std::vector<int32_t> pool;
        uint32_t count = readU30(data);

        for (uint32_t i = 0; i < count; i++)
        {
            pool.push_back(readS32(data));
        }

        return pool;
    }

    // ... more parsing methods ...
};
```

**Why C++:**
- ✅ Complex binary parsing
- ✅ Variable-length arrays (constant pools)
- ✅ Runs at build time
- ✅ Benefits from std::vector
- ✅ Automatic cleanup on parse errors

**2. AS3 Code Generator** (`src/abc/abc_codegen.cpp`)

**✅ Use C++:**
```cpp
class AS3CodeGenerator
{
public:
    void generateClass(const ClassInfo& cls, std::stringstream& out)
    {
        out << "// Class: " << cls.name << "\n\n";

        // Generate struct
        out << "typedef struct {\n";
        out << "\tAS3Object base;\n";

        for (const Trait& trait : cls.instance_traits)
        {
            if (trait.kind == TRAIT_SLOT)
            {
                out << "\t" << typeToC(trait.type_name) << " "
                    << trait.name << ";\n";
            }
        }

        out << "} " << cls.name << "_instance;\n\n";

        // Generate methods
        for (const Trait& trait : cls.instance_traits)
        {
            if (trait.kind == TRAIT_METHOD)
            {
                generateMethod(trait.method, out);
            }
        }
    }

private:
    std::string typeToC(const std::string& as3_type)
    {
        if (as3_type == "int") return "int32_t";
        if (as3_type == "Number") return "double";
        if (as3_type == "String") return "char*";
        return "AS3Value*";
    }
};
```

**Why C++:**
- ✅ String manipulation (`std::stringstream`)
- ✅ Code generation
- ✅ Build-time only
- ✅ Clean, readable

### Runtime Components (Use C)

**1. AS3 Type System** (`avm2_types.c`)

**✅ Use C:**
```c
typedef struct AS3Value {
    AS3Type type;
    union {
        int32_t i;
        uint32_t ui;
        double d;
        uint8_t b;
        char* s;
        struct AS3Object* obj;
    } value;
    uint32_t refcount;
} AS3Value;

// Type conversion - runs every frame
double toNumber(AS3Value* input)
{
    switch (input->type)
    {
        case TYPE_UNDEFINED: return NAN;
        case TYPE_NULL: return 0.0;
        case TYPE_BOOLEAN: return input->value.b ? 1.0 : 0.0;
        case TYPE_INT: return (double)input->value.i;
        case TYPE_UINT: return (double)input->value.ui;
        case TYPE_NUMBER: return input->value.d;
        case TYPE_STRING: return stringToNumber(input->value.s);
        case TYPE_OBJECT: {
            AS3Value* prim = toPrimitive(input, HINT_NUMBER);
            double result = toNumber(prim);
            release(prim);
            return result;
        }
        default: return NAN;
    }
}
```

**Why C:**
- ✅ Runs every frame (performance critical)
- ✅ Simple data structures
- ✅ No polymorphism needed
- ✅ Direct, predictable performance
- ✅ Minimal binary size

**2. AS3 Opcodes** (`avm2_opcodes.c`)

**✅ Use C:**
```c
// The infamous 'add' opcode
void opcode_add(AVM2Context* ctx)
{
    AS3Value* v2 = pop(ctx);
    AS3Value* v1 = pop(ctx);
    AS3Value* result = NULL;

    // ECMA-262 Section 11.6: Additive Operators

    if (isNumber(v1) && isNumber(v2))
    {
        result = createNumber(toNumber(v1) + toNumber(v2));
    }
    else if (isString(v1) || isString(v2))
    {
        char* s1 = toString(v1);
        char* s2 = toString(v2);
        result = createString(concat(s1, s2));
        free(s1); free(s2);
    }
    // ... more cases per ECMA-262 ...

    push(ctx, result);
    release(v1);
    release(v2);
    release(result);
}
```

**Why C:**
- ✅ Runs thousands of times per second
- ✅ Complexity in ECMA-262 spec, not in code
- ✅ C++ doesn't simplify this
- ✅ Direct implementation is fastest

**3. AS3 Object Model** (`avm2_object.c`)

**✅ Use C:**
```c
typedef struct AS3Object {
    AS3Class* klass;
    AS3Object* prototype;
    HashMap* properties;  // Simple C hash table
    AS3Value** slots;
    uint32_t slot_count;
    uint32_t refcount;
} AS3Object;

AS3Value* getProperty(AS3Object* obj, const char* name, AS3Namespace* ns)
{
    // Check slots first (fast path)
    uint32_t slot_id = findSlot(obj->klass, name, ns);
    if (slot_id != INVALID_SLOT)
    {
        return obj->slots[slot_id];
    }

    // Check dynamic properties
    AS3Value* val = hashmap_get(obj->properties, name);
    if (val) return val;

    // Check prototype chain
    if (obj->prototype)
    {
        return getProperty(obj->prototype, name, ns);
    }

    return createUndefined();
}
```

**Why C:**
- ✅ Property access runs every frame
- ✅ Simple hash table sufficient (c-hashmap library)
- ✅ No need for C++ std::unordered_map overhead
- ✅ Direct implementation, no virtual dispatch

### Summary: AS3 Build vs Runtime

```
┌─────────────────────────────────────────────────────────┐
│  AS3 BUILD-TIME (C++)                                   │
│                                                         │
│  src/abc/abc_parser.cpp         ← Parse ABC format     │
│    - std::vector for constant pools                    │
│    - std::string for names                             │
│    - Complex binary parsing                            │
│                                                         │
│  src/abc/abc_codegen.cpp        ← Generate C code      │
│    - std::stringstream for code gen                    │
│    - std::string for names                             │
│    - Template generation                               │
│                                                         │
│  Output: Pure C files                                  │
└─────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────┐
│  AS3 RUNTIME (C)                                        │
│                                                         │
│  avm2_types.c              ← Value system              │
│    - Tagged unions                                     │
│    - Type conversion (toNumber, toString)              │
│    - Reference counting                                │
│                                                         │
│  avm2_opcodes.c            ← 164 opcodes               │
│    - add, multiply, equals, etc.                       │
│    - ECMA-262 semantics                                │
│    - Performance critical                              │
│                                                         │
│  avm2_object.c             ← Object model              │
│    - Property access                                   │
│    - Prototype chain                                   │
│    - Method dispatch                                   │
│                                                         │
│  avm2_builtins.c           ← Flash API classes         │
│    - Array, String, Math, etc.                         │
│    - DisplayObject hierarchy                           │
│    - Event system                                      │
└─────────────────────────────────────────────────────────┘
```

---

## Performance & Binary Size Analysis

### Build-Time Performance (Doesn't Matter Much)

**SWFRecomp (C++):**
- Runs once per SWF file
- Typical parse time: 100ms - 2000ms (depends on SWF complexity)
- Memory usage: 10MB - 100MB (complex shapes, many triangles)
- **Nobody cares** - it's a dev tool, not end-user software

### Runtime Performance (Critical)

**SWFModernRuntime (C):**
- Runs 60 times per second
- Frame budget: 16.67ms per frame (60 FPS)
- Typical frame time: 2-8ms (depends on content)

**Performance comparison (estimated):**

| Operation | C Implementation | C++ Implementation | Overhead |
|-----------|------------------|-------------------|----------|
| Function call | Direct | Virtual (vtable) | +5-10 ns |
| Property access | Direct hash lookup | std::unordered_map | +10-20 ns |
| Type check | Switch on enum | dynamic_cast | +50-100 ns |
| Memory allocation | malloc/free | new/delete | +20-50 ns |
| String concat | Manual | std::string | +100-500 ns |

**Over 1000 operations per frame:**
- C: ~15 µs overhead
- C++: ~150 µs overhead
- **10x difference** in hot path

### Binary Size Analysis

**Current (AS1/2 support):**

```
SWFRecomp (build tool) - ~2.5 MB
├── Code: 500 KB (C++ code)
├── Data: 100 KB (string literals, etc.)
└── Dependencies: 1.9 MB (zlib, LZMA, earcut, tomlplusplus)
    → Size doesn't matter (dev tool)

SWFModernRuntime (C) - ~430 KB (static library)
├── Code: 300 KB (action implementations, rendering)
├── Data: 50 KB (lookup tables)
└── Dependencies: 80 KB (SDL3 wrapper, minimal)
    → Size matters (especially for WASM)

Generated code - ~50 KB per SWF
├── Scripts: 20-30 KB
├── Draws: 20-30 KB
└── Constants: 5-10 KB

Final WASM binary - ~500 KB
├── Runtime: 430 KB
├── Generated code: 50 KB
└── WASM overhead: 20 KB
```

**Projected (with AS3 support in C++):**

```
Final WASM binary - ~1100 KB
├── Runtime (C): 430 KB
├── AS3 Runtime (C++): 500 KB
│   ├── std::string: 50 KB
│   ├── std::vector: 30 KB
│   ├── std::unordered_map: 40 KB
│   ├── RTTI: 30 KB
│   ├── Exception handling: 50 KB
│   └── AS3 code: 300 KB
├── Generated code: 50 KB
└── WASM overhead: 120 KB
```

**Projected (with AS3 support in C):**

```
Final WASM binary - ~700 KB
├── Runtime (C): 430 KB
├── AS3 Runtime (C): 200 KB
│   ├── Type system: 30 KB
│   ├── Opcodes: 80 KB
│   ├── Object model: 40 KB
│   ├── Built-ins: 30 KB
│   └── Utilities: 20 KB
├── Generated code: 50 KB
└── WASM overhead: 20 KB
```

**Size savings with C: ~400 KB (36% smaller)**

### Download Time (3G connection, 1 Mbps)

| Binary Size | Download Time | Difference |
|-------------|---------------|------------|
| 500 KB (C, AS1/2 only) | 4 seconds | Baseline |
| 700 KB (C, with AS3) | 5.6 seconds | +1.6s |
| 1100 KB (C++, with AS3) | 8.8 seconds | +4.8s |

**Why this matters:**
- Mobile users on slow connections
- First-time load experience
- Bandwidth costs for hosting

---

## Best Practices

### 1. Default to C for Runtime Code

**Rule:** If it runs in the final executable/WASM, use C unless there's a compelling reason.

**Example:**
```c
// ✅ Good - Runtime code in C
void renderSprite(Sprite* sprite, VulkanContext* ctx)
{
    // Direct, simple, fast
}
```

```cpp
// ❌ Bad - Runtime code with unnecessary C++
void renderSprite(std::shared_ptr<Sprite> sprite, VulkanContext* ctx)
{
    // Adds overhead for no benefit
}
```

### 2. Use C++ for Complex Build-Time Algorithms

**Rule:** If it's complex, runs at build time, and benefits from STL, use C++.

**Example:**
```cpp
// ✅ Good - Complex build-time algorithm
std::vector<std::vector<Path>> detectCycles(const Graph& graph)
{
    std::vector<std::vector<Path>> cycles;
    std::unordered_map<Node*, bool> visited;
    // ... Johnson's algorithm ...
    return cycles;
}
```

```c
// ❌ Bad - Reimplementing complex data structures manually
// When STL already provides them and it's build-time only
Cycles* detectCycles(Graph* graph)
{
    // Manual linked lists, manual hash tables, manual memory management
    // Much more code, more bugs, harder to maintain
    // For no benefit (this only runs at build time)
}
```

### 3. Keep Generated Code Simple

**Rule:** Generated code should be pure C, simple function calls.

**Example:**
```c
// ✅ Good - Simple generated code
void script_0(char* stack, u32* sp)
{
    actionPushString(stack, sp, "Hello");
    actionTrace(stack, sp);
}
```

```cpp
// ❌ Bad - Generated code with C++
void script_0(std::vector<AS3Value>& stack)
{
    stack.push_back(AS3String("Hello"));
    trace(stack.back());
}
```

### 4. Profile Before Optimizing

**Rule:** Measure before assuming C++ is slow.

**Example:**
```c
// Don't prematurely optimize
// First, measure if this is actually a hot path
void processValue(AS3Value* val)
{
    // Is this called 1000 times per frame? Or once per frame?
    // Profile first, then optimize if needed
}
```

### 5. Use C++ Libraries at Build Time

**Rule:** Don't reinvent the wheel for build-time tools.

**Example:**
```cpp
// ✅ Good - Use existing C++ library
#include <toml++/toml.h>
auto config = toml::parse_file("config.toml");
```

```c
// ❌ Bad - Write your own TOML parser in C
// When you only need it at build time
TOMLTable* config = toml_parse_file("config.toml");
// ... 1000 lines of parsing code you now have to maintain ...
```

### 6. Separate Build-Time and Runtime Headers

**Rule:** Make it obvious what's build-time vs runtime.

**Example:**
```
include/
├── build/              ← Build-time only (C++ OK)
│   ├── abc_parser.hpp
│   ├── abc_types.hpp
│   └── codegen.hpp
├── runtime/            ← Runtime only (C only)
│   ├── avm2_types.h
│   ├── avm2_opcodes.h
│   └── avm2_object.h
└── common/             ← Shared (C only for portability)
    └── common.h
```

### 7. Minimize Runtime Dependencies

**Rule:** Runtime code should have minimal external dependencies.

**Current dependencies (good):**
- ✅ SDL3 - Essential for window/input/audio
- ✅ Vulkan - Essential for rendering
- ✅ c-hashmap - Small, simple, single-file
- ✅ libc - Can't avoid

**Avoid:**
- ❌ STL (std::vector, std::string, etc.)
- ❌ Boost
- ❌ Heavy frameworks
- ❌ C++ runtime (RTTI, exceptions)

### 8. Test WASM Size Regularly

**Rule:** Monitor WASM binary size as a key metric.

**Example CI check:**
```bash
# Build WASM
emcc -o output.wasm ...

# Check size
SIZE=$(stat -f%z output.wasm)
if [ $SIZE -gt 1000000 ]; then  # 1 MB limit
    echo "ERROR: WASM binary too large: ${SIZE} bytes"
    exit 1
fi
```

### 9. Document Why C++ Was Chosen

**Rule:** When using C++ in a new component, document why.

**Example:**
```cpp
// abc_parser.cpp
//
// WHY C++:
// - Runs at build time only (not in final binary)
// - Complex binary parsing with variable-length arrays
// - Benefits from std::vector for constant pools
// - No performance impact on end-users
//
// See: C_VS_CPP_ARCHITECTURE.md

class ABCParser { ... };
```

### 10. Benchmark Critical Paths

**Rule:** For hot path code, benchmark C vs C++ if unsure.

**Example:**
```c
// Benchmark: toNumber() called 10,000 times
// C implementation: 150 µs
// C++ with virtual dispatch: 380 µs
// Decision: Use C (2.5x faster)

double toNumber(AS3Value* input)  // C version
{
    switch (input->type) { ... }
}
```

---

## References

### LittleCube's Guidance

From Discord feedback (October 28, 2025):

> "we shouldn't use C++, we can't afford the bloat/overhead, we need the raw power of pure C"

> "C++ doesn't actually help that much with making the implementation simpler either. We'd need to implement a lot of things manually either way, like toNumber and toString"

> "the point being that the complexity of these instructions is not something C++ can help us with, and will most likely only get in the way unfortunately"

**Key insight:** The complexity is in the **Flash/AS3 semantics** (ECMA-262 specifications), not in the implementation language. C++ adds overhead without simplifying the core logic.

### Related Documentation

- **AS3_IMPLEMENTATION_PLAN.md** - Full AS3 plan with C++ (before LittleCube's feedback)
- **AS3_C_IMPLEMENTATION_PLAN.md** - Full AS3 plan with pure C (after feedback)
- **SEEDLING_IMPLEMENTATION_PLAN.md** - Targeted AS3 for Seedling game
- **SEEDLING_MANUAL_CPP_CONVERSION.md** - Manual AS3→C++ conversion alternative
- **SYNERGY_ANALYSIS.md** - How manual conversion and SWFRecomp synergize

### External References

1. **Ruffle** (Rust Flash emulator)
   - https://github.com/ruffle-rs/ruffle
   - Uses Rust for both build and runtime
   - Larger binary size (~2-3 MB WASM)
   - Different trade-offs than SWFRecomp

2. **Flash Player Source** (leaked)
   - C++ codebase
   - Multiple megabytes
   - Not suitable for web delivery
   - Demonstrates why C is better for recompilation

3. **Emscripten Documentation**
   - https://emscripten.org/docs/optimizing/Optimizing-Code.html
   - Tips for minimizing WASM size
   - Importance of avoiding C++ features

4. **WebAssembly Binary Size**
   - https://surma.dev/things/js-to-asc/
   - Case study: JavaScript vs C vs C++ → WASM
   - Shows C produces smallest binaries

---

## Conclusion

**SWFRecomp's current hybrid C/C++ architecture is optimal:**

✅ **C++ for build-time tools:**
- SWFRecomp (recompiler)
- ABC parser (when AS3 is implemented)
- Code generators
- Complex algorithms (triangulation, cycle detection)
- Config parsing

✅ **C for runtime code:**
- Generated code
- SWFModernRuntime
- Future AS3 runtime
- Action implementations
- Object model

**This gives us:**
- 🚀 Fast development (C++ for complex build tools)
- 🚀 Fast execution (C for runtime)
- 🚀 Small binaries (C produces compact WASM)
- 🚀 Maintainable (right tool for each job)

**Key principle:**
> Use C++ where it helps productivity (build-time).
> Use C where performance and size matter (runtime).

This is not dogma - it's pragmatism based on the actual requirements of the project.
