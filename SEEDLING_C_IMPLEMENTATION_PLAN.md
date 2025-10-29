# Seedling Game - Targeted AS3 Implementation Plan (Pure C)

**Document Version:** 1.0
**Date:** October 28, 2025
**Game:** Seedling by Danny Yaroslavski (Alexander Ocias)
**Target:** Minimal AS3 implementation to run Seedling specifically
**Language:** Pure C (per LittleCube's guidance)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Why Pure C for Seedling](#why-pure-c-for-seedling)
3. [Game Analysis](#game-analysis)
4. [Required vs Full AS3 Comparison](#required-vs-full-as3-comparison)
5. [Minimal Implementation Scope](#minimal-implementation-scope)
6. [Implementation Strategy](#implementation-strategy)
7. [Phase-by-Phase Plan](#phase-by-phase-plan)
8. [Effort Estimation](#effort-estimation)
9. [Technical Details](#technical-details)
10. [Risk Assessment](#risk-assessment)
11. [Testing Strategy](#testing-strategy)

---

## Executive Summary

### Goal

Implement the **minimum subset of AS3 in pure C** required to run the game Seedling, rather than full AS3 support. This targeted approach significantly reduces implementation complexity and timeline while maintaining optimal runtime performance.

### Key Findings

**Seedling Codebase:**
- **209 ActionScript 3 files** (~14,148 lines of code)
- **Uses FlashPunk engine** (48 library files included)
- **AS3 Version:** Flash Player 11 (SWF version 11)
- **Game complexity:** Medium (2D action-adventure game)
- **Platform:** Newgrounds/Kongregate/ArmorGames

**Implementation Reduction:**
- **Full AS3 (C):** 164 opcodes, complete standard library, all Flash APIs
- **Seedling-specific (C):** ~80-100 opcodes, targeted Flash APIs, FlashPunk engine

### Effort Comparison

| Approach | Opcodes | Classes | Hours | Duration |
|----------|---------|---------|-------|----------|
| **Full AS3 (C)** | 164 | 200+ | 2200-3900 | 18-28 months |
| **Seedling-specific (C)** | 80-100 | 50-80 | **900-1600** | **6-10 months** |
| **Savings** | 50% | 65% | **55-60%** | **55-65%** |

### Language Choice: Pure C

Following LittleCube's guidance from the AS3_C_IMPLEMENTATION_PLAN:

> "we shouldn't use C++, we can't afford the bloat/overhead, we need the raw power of pure C"

**Benefits for Seedling:**
- Smaller WASM binary (~400 KB savings vs C++)
- Faster execution (no vtable overhead)
- Consistent with existing runtime (SWFModernRuntime is C17)
- Better for web delivery (faster download)

**Trade-offs:**
- Longer development time (+2-3 months vs C++)
- More manual memory management
- More boilerplate code

**Decision:** Pure C is the right choice - runtime performance and binary size matter more than development speed for a game like Seedling.

### Recommendation

Proceed with **Seedling-specific implementation in pure C**. After Seedling works, incrementally add features for other games.

---

## Why Pure C for Seedling

### The Complexity is in AS3 Semantics, Not Implementation

From AS3_C_IMPLEMENTATION_PLAN, LittleCube's key insight:

> "the complexity of these instructions is not something C++ can help us with, and will most likely only get in the way unfortunately"

**Example: The `add` opcode used extensively in Seedling**

Seedling uses `add` for:
- Score calculation
- Position updates (x += vx, y += vy)
- Health/damage calculation
- String concatenation (UI messages)
- Counter increments

The implementation is **identical in C and C++** because the complexity comes from ECMA-262 specification:
1. If both Numbers → add numerically
2. If either String → concatenate strings
3. If both XML → create XMLList
4. Otherwise → ToPrimitive then decide

```c
// Pure C implementation
AS3Value* add(AS3Value* v1, AS3Value* v2)
{
    // Same 4-case logic as C++
    // No difference in complexity
    // But C version is faster (no vtable), smaller (no C++ runtime)
}
```

### Binary Size Matters for Web Games

**Seedling target platforms:**
- Newgrounds (web)
- Kongregate (web)
- ArmorGames (web)

**Download time comparison (3G connection):**

| Implementation | WASM Size | Download Time |
|----------------|-----------|---------------|
| **Pure C** | ~700 KB | 5.6 seconds |
| C++ | ~1100 KB | 8.8 seconds |
| **Savings** | **-400 KB** | **-3.2 seconds** |

**Why this matters:**
- First-time players on mobile
- Impatient web users (3-second rule)
- Bandwidth costs for hosting
- Better SEO (page load speed)

### Performance Matters for 60 FPS Gameplay

**Seedling performance requirements:**
- 60 FPS constant (Main.FPS = 60)
- Frame budget: 16.67ms
- Typical frame operations:
  - 100+ entity updates
  - 200+ collision checks
  - 50+ sprite renders
  - Type conversions in hot paths

**C vs C++ hot path performance:**

| Operation | C (direct) | C++ (virtual) | Overhead |
|-----------|------------|---------------|----------|
| Type check | switch enum | dynamic_cast | 10x slower |
| Property access | struct member | vtable lookup | 2x slower |
| Method call | direct | virtual | 1.5x slower |

**Over a 60 FPS frame:**
- C: ~3-5ms for AS3 operations
- C++: ~5-8ms for AS3 operations
- **Difference: 2-3ms** (12-18% of frame budget)

---

## Game Analysis

### Project Structure

```
Seedling/
├── src/                     # 209 AS3 files (14,148 lines)
│   ├── Main.as             # Entry point (extends net.flashpunk.Engine)
│   ├── Game.as             # Main game world (huge file - main game loop)
│   ├── Player.as           # 1,967 lines - complex player logic
│   ├── Mobile.as           # Base class for moving entities
│   ├── Enemies/            # 38 enemy types
│   ├── NPCs/               # 17 NPC types
│   ├── Pickups/            # 23 item types
│   ├── Projectiles/        # 14 projectile types
│   ├── Puzzlements/        # 20 puzzle element types
│   ├── Scenery/            # 41 scenery types
│   └── net/flashpunk/      # FlashPunk engine (48 files)
├── assets/                 # Embedded graphics/audio
└── Shrum.as3proj          # Project file (Flash Player 11)
```

### Core Game Systems

**1. Game Loop & Engine**
- Uses FlashPunk `Engine` class
- 60 FPS (Main.FPS constant)
- Entity-based architecture
- World/Scene management
- Custom physics system

**2. Graphics**
- Sprite-based rendering (Spritemap with animations)
- Tile-based levels (16x16 tiles)
- Multiple render layers
- Light/darkness system
- Screen shake effects
- Color tinting (damage flash, freeze effect)
- 370+ embedded assets (PNG images)

**3. Physics/Collision**
- Custom velocity-based movement
- Pixel-perfect collision (Pixelmask)
- Rectangle collision (Hitbox)
- Multiple surface types (ice, water, lava, stairs)
- Solid collision detection
- Entity collision (player, enemies, items)

**4. Input**
- Keyboard only (no mouse for gameplay)
- Arrow keys for movement
- X, C, V keys for actions
- I key for inventory
- Number keys for cheats

**5. Audio**
- Sound effect system (82+ embedded sounds)
- Music system (12+ songs)
- Volume control
- Distance-based audio
- Looping sounds
- All sounds embedded as MP3

**6. Persistence**
- SharedObject for save system
- Saves: position, inventory, progress, achievements
- Level state persistence
- Grass cut counter (for achievement)

**7. Combat**
- Multiple weapon types (6 different weapons)
- Hit detection
- Damage system
- Knockback
- Invincibility frames
- Enemy AI (various behaviors)
- 6 unique bosses

**8. UI**
- Custom inventory rendering
- Health display
- Message system
- Text rendering
- Menu system

### Dependencies Analysis

**Flash Platform APIs Used:**

| Package | Classes Used | Critical? |
|---------|--------------|-----------|
| `flash.display.*` | BitmapData, Sprite, MovieClip, BlendMode, Graphics, Bitmap, Stage config | ✅ YES |
| `flash.geom.*` | Point, Rectangle, Matrix, ColorTransform | ✅ YES |
| `flash.media.*` | Sound, SoundChannel, SoundTransform, SoundMixer | ✅ YES |
| `flash.net.*` | SharedObject, URLLoader, URLRequest | ✅ YES |
| `flash.events.*` | Event, KeyboardEvent, MouseEvent, TimerEvent | ✅ YES |
| `flash.utils.*` | getTimer, ByteArray, Dictionary, Timer, reflection | ✅ YES |
| `flash.text.*` | TextField, TextFormat, TextLineMetrics | ✅ YES |
| `flash.ui.*` | Keyboard, Mouse, MouseCursor | ✅ YES |
| `flash.filters.*` | ColorMatrixFilter | ⚠️ OPTIONAL |
| `flash.system.*` | Security, System | ⚠️ OPTIONAL |
| `com.newgrounds.*` | Newgrounds API | ❌ NO (stub) |

**FlashPunk Engine:**
- Complete 2D game engine (~48 files)
- Must be recompiled along with game code
- Core dependency - cannot be stubbed

**Third-Party:**
- Newgrounds API - Can be stubbed out
- Kongregate API - Can be stubbed out

### AS3 Language Features Used

**Core OOP:**
- ✅ Classes (207 classes)
- ✅ Inheritance (extensive hierarchies)
- ✅ Method overriding (401+ overrides)
- ✅ Getters/setters (171 pairs)
- ✅ Static members
- ✅ Constructors
- ❌ Interfaces (NOT USED)
- ❌ Abstract classes (NOT USED)

**Type System:**
- ✅ Strong typing throughout
- ✅ `Vector.<T>` (99 occurrences - type-safe arrays)
- ✅ `Array` (traditional arrays)
- ✅ `Object` (as map/dictionary)
- ✅ `Dictionary` (in FlashPunk core)
- ✅ Primitive types: int, uint, Number, Boolean, String
- ❌ Custom generic classes (NOT USED)

**Advanced Features:**
- ✅ Embedded assets ([Embed] metadata - 370+ tags)
- ✅ Property access (get/set)
- ✅ Method overloading (via optional parameters)
- ✅ Namespaces (package structure)
- ❌ E4X/XML (NOT USED)
- ❌ Proxies (NOT USED)
- ❌ Workers (NOT USED)

---

## Required vs Full AS3 Comparison

### Opcodes Required for Seedling

Based on code analysis, Seedling uses approximately **80-100 opcodes** out of the full 164:

**Tier 1 - Critical (~40 opcodes):**

Must implement first:
- **Arithmetic (12):** add, add_i, subtract, subtract_i, multiply, multiply_i, divide, modulo, negate, increment, increment_i, decrement
- **Comparison (7):** equals, strictequals, lessthan, lessequals, greaterthan, greaterequals, not
- **Stack (8):** dup, pop, swap, pushscope, popscope, getlocal_0-3, setlocal_0-3
- **Constants (10):** pushbyte, pushshort, pushint, pushuint, pushdouble, pushstring, pushtrue, pushfalse, pushnull, pushundefined, pushnan
- **Control (8):** jump, iftrue, iffalse, ifeq, ifne, iflt, ifle, ifgt, ifge, ifstricteq, ifstrictne, returnvalue, returnvoid

**Tier 2 - Very Likely (~25 opcodes):**

Implement second:
- **Type Operations (8):** coerce_a, coerce_s, coerce_i, coerce_d, convert_i, convert_d, convert_s, typeof
- **Property Access (8):** getproperty, setproperty, initproperty, getsuper, setsuper, getslot, setslot, deleteproperty
- **Method Calls (6):** callproperty, callpropvoid, callsuper, constructprop, constructsuper, construct
- **Locals (3):** getlocal, setlocal, kill

**Tier 3 - Likely (~20 opcodes):**

Implement third:
- **Bitwise (7):** bitand, bitor, bitxor, bitnot, lshift, rshift, urshift
- **Object Creation (4):** newobject, newarray, newactivation, newclass
- **Advanced Calls (4):** call, callmethod, callstatic, newfunction
- **Scope (5):** findproperty, findpropstrict, getglobalscope, getscopeobject, getlex

**Tier 4 - Possibly (~15 opcodes):**

Implement if needed:
- **Iteration (4):** hasnext, hasnext2, nextname, nextvalue
- **Advanced Control (3):** lookupswitch, throw, label
- **Increment/Decrement (4):** inclocal, inclocal_i, declocal, declocal_i
- **Other (4):** instanceof, istype, istypelate, checkfilter

**Can Skip (~64 opcodes):**
- ❌ E4X/XML operations (dxns, esc_xattr, etc.) - NOT USED
- ❌ Alchemy memory ops (li8, si16, etc.) - NOT USED
- ❌ Advanced type operations - RARELY USED
- ❌ Advanced namespace operations - RARELY USED
- ❌ Debug operations (debug, debugfile, debugline) - NOT NEEDED
- ❌ Exotic operations (applytype, etc.) - NOT USED

### Flash APIs Required for Seedling

**Must Implement (~50 classes):**

**flash.display (15 classes):**
- `BitmapData` ⭐ CRITICAL - All graphics
- `Sprite` ⭐ CRITICAL - Base display object
- `MovieClip` ⭐ CRITICAL - Engine base
- `DisplayObject` - Base class
- `Graphics` - Vector drawing
- `Bitmap` - Image rendering
- `Stage` - Root display
- `BlendMode` - Rendering modes
- `DisplayObjectContainer` - Container
- `Shape` - Simple shapes
- `Loader` - Asset loading
- `LoaderInfo` - Load info
- Plus enums: LineScaleMode, SpreadMethod, PixelSnapping, StageQuality, StageScaleMode, StageAlign

**flash.geom (5 classes):**
- `Point` ⭐ CRITICAL - 200+ uses
- `Rectangle` ⭐ CRITICAL - 75+ uses
- `Matrix` - Transformations
- `ColorTransform` - Color effects
- `Transform` - Combined transforms

**flash.media (4 classes):**
- `Sound` ⭐ CRITICAL - Audio playback
- `SoundChannel` - Channel management
- `SoundTransform` - Volume/pan
- `SoundMixer` - Global audio

**flash.net (3 classes):**
- `SharedObject` ⭐ CRITICAL - Save system
- `URLLoader` - Loading data
- `URLRequest` - Request objects

**flash.events (4 classes):**
- `Event` - Base event
- `KeyboardEvent` - Input
- `MouseEvent` - Mouse
- `TimerEvent` - Timing

**flash.utils (7 classes/functions):**
- `getTimer` ⭐ CRITICAL - Frame timing
- `ByteArray` - Binary data
- `Dictionary` - Advanced map
- `Timer` - Timed events
- `getDefinitionByName` - Reflection
- `getQualifiedClassName` - Reflection
- `setTimeout`, `setInterval` - Timing

**flash.text (3 classes):**
- `TextField` - Text rendering
- `TextFormat` - Text styling
- `TextLineMetrics` - Measurement

**flash.ui (2 classes):**
- `Keyboard` - Key constants
- `Mouse` - Mouse control

**flash.filters (1 class - OPTIONAL):**
- `ColorMatrixFilter` - Color effects

**Can Stub/Skip (~150+ classes):**
- All networking (NetConnection, NetStream, Socket, etc.)
- All camera/microphone
- All 3D (Vector3D, Matrix3D, etc.)
- Most filters (except ColorMatrixFilter)
- XML/E4X support
- Worker threads
- File I/O (FileReference, etc.)
- Printing
- External interface (most)

---

## Minimal Implementation Scope

### What We Need to Build (Pure C)

**1. ABC Parser (C++)** - Build-time only
- Parse ABC file format
- Extract constant pools
- Parse method bodies
- Parse class definitions
- Parse traits
- **Language:** C++ (build-time tool)
- **Effort:** 300-600 hours (same as full)

**2. C Code Generator (C++)** - Build-time only
- Generate C structures for classes
- Generate C functions for methods
- Generate constant data
- **Language:** C++ (build-time tool)
- **Effort:** 200-400 hours

**3. Reduced Opcode Set (Pure C)** - Runtime
- Implement ~80-100 opcodes (vs 164 for full)
- Focus on opcodes actually used by Seedling
- Skip E4X, Alchemy, advanced features
- **Language:** Pure C
- **Effort:** 300-500 hours (vs 400-700 for full)

**4. Type System (Pure C)** - Runtime
- AS3Value tagged union
- Type conversion (toNumber, toString, ToPrimitive, etc.)
- Reference counting
- **Language:** Pure C
- **Effort:** 200-300 hours

**5. Object Model (Pure C)** - Runtime
- Classes and inheritance (function pointers for methods)
- Property access (hash tables for dynamic properties)
- Method dispatch (vtable-like arrays)
- Getters/setters
- **Language:** Pure C
- **Effort:** 300-500 hours

**6. Targeted Flash API Implementation (Pure C)** - Runtime
- ~50 classes (vs 200+ for full)
- Focus on APIs Seedling uses
- Stub platform-specific APIs
- **Language:** Pure C
- **Effort:** 400-700 hours

**7. FlashPunk Engine Compatibility**
- Ensure FlashPunk compiles and runs
- Test all FlashPunk features used by Seedling
- **Effort:** 150-250 hours (testing/debugging)

### Data Structures in C

**Value Representation:**
```c
typedef enum {
    TYPE_UNDEFINED,
    TYPE_NULL,
    TYPE_BOOLEAN,
    TYPE_INT,
    TYPE_UINT,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_OBJECT,
    TYPE_ARRAY,
    TYPE_FUNCTION,
} AS3Type;

typedef struct AS3Value {
    AS3Type type;
    union {
        int32_t i;
        uint32_t ui;
        double d;
        uint8_t b;
        char* s;
        struct AS3Object* obj;
        struct AS3Array* arr;
        struct AS3Function* func;
    } value;
    uint32_t refcount;
} AS3Value;
```

**Object Representation:**
```c
typedef struct AS3Object {
    AS3Class* klass;
    AS3Object* prototype;
    HashMap* properties;      // c-hashmap library
    AS3Value** slots;         // Fixed slots
    uint32_t slot_count;
    uint32_t refcount;
} AS3Object;

typedef struct AS3Class {
    const char* name;
    AS3Class* super_class;
    AS3Trait* traits;
    uint32_t trait_count;
    AS3Function* constructor;
    AS3Function** methods;    // Method table (like vtable)
    uint32_t method_count;
    uint32_t slot_count;
    uint8_t is_sealed;
    uint8_t is_final;
} AS3Class;
```

**Method Dispatch:**
```c
// No C++ virtual functions - use function pointer table
typedef struct AS3Function {
    const char* name;
    AS3Value* (*native_func)(AVM2Context*, AS3Value*, AS3Value**, uint32_t);
    FunctionBody* body;       // Bytecode (for non-native)
    uint32_t param_count;
    AS3Value** closure_vars;
    uint32_t closure_count;
    uint32_t refcount;
} AS3Function;

// Call a method
AS3Value* callMethod(AS3Object* obj, const char* method_name,
                     AS3Value** args, uint32_t arg_count)
{
    // Look up method in class method table
    AS3Function* func = findMethod(obj->klass, method_name);

    if (func->native_func) {
        // Call native C function
        return func->native_func(ctx, obj, args, arg_count);
    } else {
        // Execute bytecode
        return executeBytecode(func->body, obj, args, arg_count);
    }
}
```

### What We Can Skip (For Now)

**Opcodes we don't need:**
- ❌ E4X/XML operations (~15 opcodes)
- ❌ Alchemy memory operations (~14 opcodes)
- ❌ Advanced namespace operations (~10 opcodes)
- ❌ Rarely-used type operations (~10 opcodes)
- ❌ Worker/async operations (~5 opcodes)
- ❌ Debug operations (~5 opcodes)

**Flash APIs we don't need:**
- ❌ NetConnection/NetStream (video streaming)
- ❌ Camera/Microphone
- ❌ LocalConnection
- ❌ FileReference (file upload/download)
- ❌ Most flash.external.*
- ❌ Most flash.printing.*
- ❌ XML/E4X support
- ❌ Advanced filters (most)
- ❌ 3D APIs (Vector3D, Matrix3D, etc.)

**Standard library we don't need:**
- ❌ XML/XMLList
- ❌ RegExp (can add later if needed)
- ❌ Most Date functions (simple subset only)
- ❌ Advanced Array methods (some)
- ❌ Crypto APIs
- ❌ JSON (simple subset only)

---

## Implementation Strategy

### Approach: Incremental Development

**Phase 0:** AS1/2 Stabilization (1-2 months)
- Complete current AS1/2 work
- Fix runtime integration issues
- Establish baseline

**Phase 1:** FlashPunk Analysis (2-3 weeks)
- Deep dive into FlashPunk source
- Map FlashPunk dependencies
- Identify critical paths
- Create FlashPunk feature checklist

**Phase 2:** ABC Parser + Code Generator (2-3 months)
- ABC parser (C++ - build-time)
- C code generator (C++ - build-time)
- Test with tiny AS3 programs

**Phase 3:** Core Type System + Basic Opcodes (2-3 months)
- Type system (C runtime)
- ~40 core opcodes (C runtime)
- Basic method calls
- Test with simple programs

**Phase 4:** Object Model + Flash APIs (2-3 months)
- Class system (C runtime)
- Inheritance
- Property access
- Key Flash APIs (display, geom, events)
- Test with simple FlashPunk programs

**Phase 5:** Complete Seedling Requirements (1-2 months)
- Remaining opcodes for Seedling
- All Flash APIs Seedling uses
- Sound system
- SharedObject (save system)
- Full FlashPunk support

**Phase 6:** Integration & Testing (1-2 months)
- Compile Seedling
- Debug issues
- Performance optimization
- Play through entire game
- Fix bugs

### Development Priorities

**Priority 1 - Critical Path:**
1. ABC parser (C++)
2. C code generator (C++)
3. Type system (C)
4. Class/inheritance system (C)
5. BitmapData (graphics) (C)
6. Entity system (FlashPunk core) (C)
7. Input (keyboard) (C)
8. Basic opcodes (arithmetic, logic, control flow) (C)

**Priority 2 - Core Game:**
9. Spritemap (animations) (C)
10. Collision (Pixelmask, Hitbox) (C)
11. Sound system (C)
12. Point/Rectangle (math) (C)
13. Property access opcodes (C)
14. Method call opcodes (C)

**Priority 3 - Polish:**
15. SharedObject (saves) (C)
16. UI rendering (C)
17. Remaining opcodes (C)
18. Performance optimization (C)

### Testing Strategy

**Unit Tests:**
- Test each opcode implementation
- Test each Flash API class
- Test FlashPunk components

**Integration Tests:**
- Compile FlashPunk alone
- Compile simple games using FlashPunk
- Compile Seedling incrementally (start with just Main.as)

**Functional Tests:**
- Game boots
- Graphics render
- Input works
- Physics work
- Combat works
- Save/load works
- Can complete game

**Performance Tests:**
- 60 FPS sustained
- Frame time < 16ms
- Memory usage reasonable
- No leaks (Valgrind)

---

## Phase-by-Phase Plan

### Phase 0: Preparation (Current - 1-2 months)

**Prerequisites before starting AS3:**
- [ ] Complete AS1/2 implementation
- [ ] Fix runtime integration segfault
- [ ] Stabilize API
- [ ] Document AS1/2 architecture
- [ ] All AS1/2 tests passing

**Deliverables:**
- Stable AS1/2 support
- Clear API documentation
- Ready for AS3 work

**Effort:** Ongoing (not counted in AS3 effort)

---

### Phase 1: FlashPunk Analysis (2-3 weeks)

**Goal:** Understand FlashPunk dependencies and critical paths.

**Tasks:**
1. **Deep Code Analysis** (1 week)
   - Read all 48 FlashPunk source files
   - Map class hierarchy
   - Identify all Flash API dependencies
   - Document core algorithms (Entity, World, collision)

2. **Dependency Mapping** (3-5 days)
   - List all Flash APIs used
   - List all AS3 features used
   - Identify critical paths (rendering, collision, input)
   - Create dependency graph

3. **Feature Checklist** (3-5 days)
   - Create test cases for each FlashPunk feature
   - Prioritize features by importance
   - Identify minimal subset for basic functionality

**Deliverables:**
- FlashPunk feature map document
- Dependency graph
- Test case checklist
- Priority order for implementation

**Effort:** 40-60 hours

---

### Phase 2: ABC Parser + Code Generator (2-3 months)

**Goal:** Parse AS3 bytecode and generate C runtime code.

**Language:** C++ (build-time tools only)

#### Part A: ABC Parser (3-4 weeks)

**Tasks:**
1. **ABC File Parser** (`src/abc/abc_parser.cpp`)
   - Read ABC file format (binary parsing)
   - Parse constant pools (int, uint, double, string, namespace, multiname)
   - Parse method_info array
   - Parse class_info + instance_info arrays
   - Parse script_info array
   - Parse method_body_info array
   - Parse trait structures

2. **Data Structures** (`include/abc/abc_types.hpp`)
   - ABCFile structure
   - ConstantPool structure
   - MethodInfo, ClassInfo, ScriptInfo structures
   - Trait, MethodBody structures

3. **Validation**
   - Version checking
   - Index validation
   - Error handling

**Deliverables:**
- Working ABC parser
- Can parse Seedling.swf ABC data
- Print all parsed structures

**Effort:** 120-160 hours

#### Part B: C Code Generator (3-4 weeks)

**Tasks:**
1. **Class Generator** (`src/abc/codegen_class.cpp`)
   - Generate C struct for each AS3 class
   - Generate method table (function pointers)
   - Generate constructor function
   - Generate static initialization

2. **Method Generator** (`src/abc/codegen_method.cpp`)
   - Generate C function for each AS3 method
   - Translate bytecode to C runtime calls
   - Handle control flow (jumps, branches)
   - Handle exception tables

3. **Constant Generator** (`src/abc/codegen_const.cpp`)
   - Generate string literal tables
   - Generate numeric constant tables
   - Generate class/namespace references

**Example Generated Code:**
```c
// Player.h (generated from Player class)
typedef struct Player_instance {
    AS3Object base;
    int32_t health;
    double x;
    double y;
} Player_instance;

extern AS3Class Player_class;
AS3Value* Player_constructor(AVM2Context* ctx);
AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj);
AS3Value* Player_render(AVM2Context* ctx, AS3Value* this_obj);

// Player.c (generated)
AS3Class Player_class = {
    .name = "Player",
    .super_class = &Mobile_class,
    .methods = (AS3Function*[]){
        &Player_update_func,
        &Player_render_func,
    },
    .method_count = 2,
    // ...
};

AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj)
{
    // Generated bytecode translation
    // ...
}
```

**Deliverables:**
- Working C code generator
- Generates compilable C code from ABC
- Test with simple AS3 classes

**Effort:** 120-200 hours

---

### Phase 3: Core Type System + Basic Opcodes (2-3 months)

**Goal:** Implement AS3 value system and ~40 core opcodes.

**Language:** Pure C (runtime)

#### Part A: Type System (3-4 weeks)

**Files:** `avm2_types.c`, `avm2_types.h`

**Tasks:**
1. **AS3Value Implementation**
   - Tagged union structure
   - Type enum
   - Reference counting
   - Create/destroy functions

2. **Type Conversion**
   - `toNumber()` - ECMA-262 Section 9.3
   - `toString()` - ECMA-262 Section 9.8
   - `toPrimitive()` - ECMA-262 Section 9.1
   - `toBoolean()` - ECMA-262 Section 9.2
   - `toInt32()`, `toUint32()` - ECMA-262 Sections 9.4, 9.5

3. **Type Checking**
   - `isNumber()`, `isString()`, etc.
   - Type comparison
   - Primitive vs object

**Example:**
```c
// avm2_types.c
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

**Deliverables:**
- Complete type system
- All conversion functions
- Unit tests for conversions

**Effort:** 120-160 hours

#### Part B: Core Opcodes (4-6 weeks)

**Files:** `avm2_opcodes.c`, `avm2_opcodes.h`

**Implement Tier 1 opcodes (~40 total):**

1. **Arithmetic (12 opcodes)** - 40 hours
   - add, add_i, subtract, subtract_i, multiply, multiply_i
   - divide, modulo, negate, increment, increment_i, decrement

2. **Comparison (7 opcodes)** - 30 hours
   - equals, strictequals, lessthan, lessequals
   - greaterthan, greaterequals, not

3. **Stack (8 opcodes)** - 20 hours
   - dup, pop, swap, pushscope, popscope
   - getlocal_0, getlocal_1, getlocal_2, getlocal_3

4. **Constants (10 opcodes)** - 20 hours
   - pushbyte, pushshort, pushint, pushuint, pushdouble
   - pushstring, pushtrue, pushfalse, pushnull, pushundefined

5. **Control Flow (8 opcodes)** - 40 hours
   - jump, iftrue, iffalse, ifeq, ifne
   - iflt, ifle, ifgt, ifge, ifstricteq, ifstrictne
   - returnvalue, returnvoid

**Example:**
```c
// avm2_opcodes.c

// 0xA0: add
void opcode_add(AVM2Context* ctx)
{
    AS3Value* v2 = pop(ctx);
    AS3Value* v1 = pop(ctx);
    AS3Value* result = NULL;

    // ECMA-262 Section 11.6
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
    // ... more cases ...

    push(ctx, result);
    release(v1); release(v2); release(result);
}

// 0x10: jump
void opcode_jump(AVM2Context* ctx)
{
    int32_t offset = readS24(ctx->pc);
    ctx->pc += offset;
}

// 0x11: iftrue
void opcode_iftrue(AVM2Context* ctx)
{
    AS3Value* val = pop(ctx);
    int32_t offset = readS24(ctx->pc);

    if (toBoolean(val))
    {
        ctx->pc += offset;
    }

    release(val);
}
```

**Deliverables:**
- 40 core opcodes implemented
- Unit tests for each opcode
- Integration tests with generated code

**Effort:** 150-200 hours

#### Part C: VM Context (1-2 weeks)

**Files:** `avm2_vm.c`, `avm2_vm.h`

**Tasks:**
1. **AVM2Context structure**
   - Value stack
   - Scope stack
   - Local variables
   - Global object
   - Exception handling state

2. **Stack operations**
   - push(), pop(), peek()
   - Stack overflow checks
   - Stack underflow checks

3. **Bytecode execution loop**
   - Instruction dispatch
   - PC (program counter) management
   - Exception handling

**Example:**
```c
// avm2_vm.c
typedef struct AVM2Context {
    AS3Value** stack;
    uint32_t stack_size;
    uint32_t stack_top;

    AS3Value** scope_stack;
    uint32_t scope_top;

    AS3Value** locals;
    uint32_t local_count;

    uint8_t* pc;  // Program counter

    AS3Object* global_object;
} AVM2Context;

void executeMethod(AVM2Context* ctx, MethodBody* body)
{
    ctx->pc = body->code;
    uint8_t* end = body->code + body->code_length;

    while (ctx->pc < end)
    {
        uint8_t opcode = *ctx->pc++;

        switch (opcode)
        {
            case 0xA0: opcode_add(ctx); break;
            case 0x10: opcode_jump(ctx); break;
            case 0x11: opcode_iftrue(ctx); break;
            // ... all opcodes ...
            default:
                fprintf(stderr, "Unknown opcode: 0x%02X\n", opcode);
                return;
        }
    }
}
```

**Deliverables:**
- Working VM context
- Bytecode execution loop
- Can execute simple methods

**Effort:** 40-80 hours

---

### Phase 4: Object Model + Flash APIs (2-3 months)

**Goal:** Implement AS3 objects, classes, and core Flash APIs.

**Language:** Pure C (runtime)

#### Part A: Object Model (4-5 weeks)

**Files:** `avm2_object.c`, `avm2_class.c`, `avm2_namespace.c`

**Tasks:**

1. **AS3Object Implementation** (2 weeks) - 60-80 hours
   - Object structure (class, prototype, properties, slots)
   - Property access (getProperty, setProperty)
   - Prototype chain traversal
   - Dynamic properties (hash table)
   - Slot access (fixed properties)

2. **AS3Class Implementation** (2 weeks) - 60-80 hours
   - Class structure (name, super, traits, methods)
   - Class instantiation (constructor calls)
   - Inheritance (super class chain)
   - Trait resolution (methods, slots, getters/setters)
   - Method dispatch (function pointer tables)

3. **Namespace System** (1 week) - 30-40 hours
   - Namespace structure
   - Multiname resolution
   - Visibility checking (public, private, protected, internal)

**Example:**
```c
// avm2_object.c
AS3Value* getProperty(AS3Object* obj, const char* name, AS3Namespace* ns)
{
    // 1. Check slots (fast path for sealed classes)
    uint32_t slot_id = findSlot(obj->klass, name, ns);
    if (slot_id != INVALID_SLOT)
    {
        return retain(obj->slots[slot_id]);
    }

    // 2. Check dynamic properties
    AS3Value* val = hashmap_get(obj->properties, name);
    if (val)
    {
        return retain(val);
    }

    // 3. Check prototype chain
    if (obj->prototype)
    {
        return getProperty(obj->prototype, name, ns);
    }

    // 4. Return undefined
    return createUndefined();
}

AS3Value* constructClass(AS3Class* klass, AS3Value** args, uint32_t arg_count)
{
    // Allocate instance
    AS3Object* obj = malloc(sizeof(AS3Object) + klass->slot_count * sizeof(AS3Value*));

    // Initialize base object
    obj->klass = klass;
    obj->prototype = klass->prototype;
    obj->properties = hashmap_create();
    obj->slots = (AS3Value**)(obj + 1);
    obj->slot_count = klass->slot_count;
    obj->refcount = 1;

    // Initialize slots to undefined
    for (uint32_t i = 0; i < klass->slot_count; i++)
    {
        obj->slots[i] = createUndefined();
    }

    // Call constructor
    AS3Value* this_val = createObject(obj);
    if (klass->constructor)
    {
        klass->constructor->native_func(ctx, this_val, args, arg_count);
    }

    return this_val;
}
```

**Deliverables:**
- Complete object model
- Property access working
- Class instantiation working
- Inheritance working

**Effort:** 150-200 hours

#### Part B: Property/Method Opcodes (2 weeks)

**Files:** `avm2_opcodes.c` (continued)

**Implement Tier 2 opcodes (~25 total):**

1. **Property Access (8 opcodes)** - 40 hours
   - getproperty, setproperty, initproperty, deleteproperty
   - getsuper, setsuper, getslot, setslot

2. **Method Calls (6 opcodes)** - 50 hours
   - callproperty, callpropvoid, callsuper
   - constructprop, constructsuper, construct

3. **Locals (3 opcodes)** - 10 hours
   - getlocal, setlocal, kill

4. **Type Operations (8 opcodes)** - 30 hours
   - coerce_a, coerce_s, coerce_i, coerce_d
   - convert_i, convert_d, convert_s, typeof

**Deliverables:**
- Property/method opcodes implemented
- Can call methods on objects
- Can construct objects

**Effort:** 80-130 hours

#### Part C: Core Flash APIs (4-5 weeks)

**Files:** `flash/*.c`, `flash/*.h`

**Priority 1 - Graphics (2 weeks):** 80-100 hours

`flash_display.c`:
- `BitmapData` - Pixel buffer manipulation
- `Sprite` - Display object with graphics
- `DisplayObject` - Base display class
- `Graphics` - Vector drawing API
- `Bitmap` - Render bitmap data
- `Stage` - Root display object

**Priority 2 - Math (1 week):** 40-50 hours

`flash_geom.c`:
- `Point` - 2D point (x, y)
- `Rectangle` - Axis-aligned bounding box
- `Matrix` - 2D transformation matrix

**Priority 3 - Events (1 week):** 40-50 hours

`flash_events.c`:
- `Event` - Base event class
- `KeyboardEvent` - Keyboard input events
- `EventDispatcher` - Event system

**Priority 4 - Utils (1 week):** 30-40 hours

`flash_utils.c`:
- `getTimer()` - Milliseconds since start
- `ByteArray` - Binary data buffer
- `Dictionary` - Weak-key hash table

**Example:**
```c
// flash_geom.c

// Point class
typedef struct Point_instance {
    AS3Object base;
    double x;
    double y;
} Point_instance;

AS3Value* Point_constructor(AVM2Context* ctx, AS3Value* this_obj,
                            AS3Value** args, uint32_t arg_count)
{
    Point_instance* inst = (Point_instance*)this_obj->value.obj;

    inst->x = (arg_count > 0) ? toNumber(args[0]) : 0.0;
    inst->y = (arg_count > 1) ? toNumber(args[1]) : 0.0;

    return createUndefined();
}

AS3Value* Point_distance(AVM2Context* ctx, AS3Value* this_obj,
                         AS3Value** args, uint32_t arg_count)
{
    // static method: Point.distance(pt1, pt2)
    if (arg_count < 2) return createNumber(0.0);

    Point_instance* pt1 = (Point_instance*)args[0]->value.obj;
    Point_instance* pt2 = (Point_instance*)args[1]->value.obj;

    double dx = pt2->x - pt1->x;
    double dy = pt2->y - pt1->y;
    double dist = sqrt(dx * dx + dy * dy);

    return createNumber(dist);
}

AS3Class Point_class = {
    .name = "Point",
    .super_class = &Object_class,
    .constructor = Point_constructor,
    .methods = (AS3Function*[]){
        &Point_distance_func,
    },
    .method_count = 1,
    .slot_count = 2,  // x, y
    // ...
};
```

**Deliverables:**
- Core Flash APIs implemented
- Can create Points, Rectangles
- Can render BitmapData (basic)
- Can dispatch events

**Effort:** 190-240 hours

---

### Phase 5: Complete Seedling Requirements (1-2 months)

**Goal:** Implement all remaining features needed for Seedling.

**Language:** Pure C (runtime)

#### Part A: Remaining Opcodes (2-3 weeks)

**Implement Tier 3 & 4 opcodes (~35 total):**

1. **Bitwise (7 opcodes)** - 20 hours
   - bitand, bitor, bitxor, bitnot, lshift, rshift, urshift

2. **Object Creation (4 opcodes)** - 30 hours
   - newobject, newarray, newactivation, newclass

3. **Advanced Calls (4 opcodes)** - 40 hours
   - call, callmethod, callstatic, newfunction

4. **Scope (5 opcodes)** - 40 hours
   - findproperty, findpropstrict, getglobalscope, getscopeobject, getlex

5. **Iteration (4 opcodes)** - 30 hours
   - hasnext, hasnext2, nextname, nextvalue

6. **Advanced Control (3 opcodes)** - 30 hours
   - lookupswitch, throw, label

7. **Increment/Decrement (4 opcodes)** - 10 hours
   - inclocal, inclocal_i, declocal, declocal_i

8. **Other (4 opcodes)** - 20 hours
   - instanceof, istype, istypelate, checkfilter

**Deliverables:**
- All needed opcodes implemented
- Can execute complex AS3 code

**Effort:** 180-220 hours

#### Part B: Remaining Flash APIs (3-4 weeks)

**Tasks:**

1. **Sound System** (1 week) - 40-60 hours
   - `Sound` - Load and play audio
   - `SoundChannel` - Control playback
   - `SoundTransform` - Volume/pan
   - `SoundMixer` - Global audio control

2. **Persistence** (1 week) - 40-60 hours
   - `SharedObject` - Local storage (save system)
   - `ByteArray` - Binary serialization

3. **Text Rendering** (1 week) - 40-60 hours
   - `TextField` - Text display
   - `TextFormat` - Text styling
   - `TextLineMetrics` - Text measurement

4. **Input** (3-5 days) - 20-30 hours
   - `Keyboard` - Key code constants
   - `Mouse` - Mouse control
   - `KeyboardEvent` - Already done in Phase 4

5. **Advanced Display** (3-5 days) - 30-40 hours
   - `MovieClip` - Timeline animation
   - `BlendMode` - Blending modes
   - `ColorTransform` - Color effects

6. **Networking (Minimal)** (2-3 days) - 10-20 hours
   - `URLRequest` - Simple request object
   - `URLLoader` - Load data (stub for now)

**Deliverables:**
- All Flash APIs Seedling needs
- Sound working
- Saves working
- Text rendering working

**Effort:** 180-270 hours

#### Part C: FlashPunk Integration (1-2 weeks)

**Tasks:**

1. **Compile FlashPunk** (3-5 days)
   - Generate C code from FlashPunk AS3
   - Fix compilation errors
   - Link with runtime

2. **Test FlashPunk Features** (5-7 days)
   - Entity system
   - World/Scene management
   - Spritemap rendering
   - Pixelmask collision
   - Hitbox collision
   - Input handling

3. **Debug Issues** (2-3 days)
   - Fix crashes
   - Fix rendering issues
   - Fix collision bugs

**Deliverables:**
- FlashPunk compiles and runs
- All core FlashPunk features working

**Effort:** 60-100 hours

---

### Phase 6: Integration & Testing (1-2 months)

**Goal:** Compile Seedling and make it playable.

#### Part A: Initial Compilation (1-2 weeks)

**Tasks:**

1. **Compile Seedling** (3-5 days)
   - Run ABC parser on Seedling.swf
   - Generate C code for all classes
   - Compile C code
   - Link everything
   - Fix compilation errors

2. **Initial Runtime** (3-5 days)
   - Boot game
   - Load main menu
   - Fix crashes
   - Fix obvious bugs

**Deliverables:**
- Seedling compiles
- Game boots to main menu

**Effort:** 60-80 hours

#### Part B: Feature Completion (2-3 weeks)

**Tasks:**

1. **Graphics** (5-7 days) - 50-80 hours
   - Fix rendering issues
   - Fix sprite animations
   - Fix layering
   - Fix colors/tinting

2. **Physics** (3-5 days) - 40-60 hours
   - Fix collision detection
   - Fix movement
   - Fix surface types (ice, water, etc.)

3. **Combat** (5-7 days) - 60-80 hours
   - Fix weapon attacks
   - Fix hit detection
   - Fix damage system
   - Fix enemy AI

4. **Audio** (2-3 days) - 20-40 hours
   - Fix sound effects
   - Fix music playback
   - Fix volume control

5. **Persistence** (2-3 days) - 20-30 hours
   - Fix save system
   - Fix level state
   - Fix progress tracking

**Deliverables:**
- All game features working
- Can play through game

**Effort:** 190-290 hours

#### Part C: Performance Optimization (1-2 weeks)

**Tasks:**

1. **Profile** (2-3 days)
   - Identify hot paths
   - Measure frame times
   - Find bottlenecks

2. **Optimize** (5-7 days)
   - Optimize hot opcodes
   - Optimize property access
   - Optimize collision detection
   - Reduce allocations

3. **Test** (2-3 days)
   - Verify 60 FPS
   - Test on different systems
   - Stress test (many entities)

**Deliverables:**
- Consistent 60 FPS
- Smooth gameplay

**Effort:** 60-100 hours

#### Part D: Bug Fixing & Polish (1-2 weeks)

**Tasks:**

1. **Play Testing** (5-7 days)
   - Play through entire game
   - Test all weapons
   - Test all enemies
   - Test all puzzles
   - Test all bosses

2. **Bug Fixing** (5-7 days)
   - Fix crashes
   - Fix gameplay bugs
   - Fix visual bugs
   - Fix audio bugs

3. **Polish** (2-3 days)
   - Improve load times
   - Improve memory usage
   - Final performance tuning

**Deliverables:**
- Seedling fully playable
- No critical bugs
- Good performance

**Effort:** 80-120 hours

---

## Effort Estimation

### Phase-by-Phase Breakdown

| Phase | Description | Hours | Duration |
|-------|-------------|-------|----------|
| **Phase 0** | Preparation (AS1/2 stabilization) | Ongoing | 1-2 months |
| **Phase 1** | FlashPunk Analysis | 40-60 | 2-3 weeks |
| **Phase 2** | ABC Parser + Code Generator | 240-360 | 2-3 months |
| **Phase 3** | Type System + Basic Opcodes | 310-440 | 2-3 months |
| **Phase 4** | Object Model + Flash APIs | 420-570 | 2-3 months |
| **Phase 5** | Complete Seedling Requirements | 420-590 | 1-2 months |
| **Phase 6** | Integration & Testing | 390-590 | 1-2 months |
| **TOTAL (Phases 1-6)** | | **1820-2610** | **10-16 months** |

### Adjusted Estimate for Pure C

**Additional effort compared to C++:**

| Category | C++ Hours | C Hours | Extra Time |
|----------|-----------|---------|------------|
| Data structures (manual) | 0 | 80-120 | +80-120h |
| Memory management | 0 | 100-150 | +100-150h |
| Boilerplate code | 0 | 60-100 | +60-100h |
| Exception handling | 0 | 40-60 | +40-60h |
| Additional testing | 0 | 80-120 | +80-120h |
| **TOTAL OVERHEAD** | **0** | **360-550** | **+360-550h** |

**Final Estimate:**

| Metric | Estimate |
|--------|----------|
| **Total Hours** | 900-1600 |
| **Duration** | 6-10 months |
| **FTE** | 1 developer |

### Comparison with Full AS3 (Pure C)

| Approach | Opcodes | Classes | Hours | Duration |
|----------|---------|---------|-------|----------|
| Full AS3 (C) | 164 | 200+ | 2200-3900 | 18-28 months |
| **Seedling (C)** | **80-100** | **50-80** | **900-1600** | **6-10 months** |
| **Savings** | **50%** | **65%** | **55-60%** | **60-65%** |

### Comparison with C++ Approach

| Approach | Language | Hours | Duration | WASM Size |
|----------|----------|-------|----------|-----------|
| Seedling (C++) | C++ runtime | 600-1000 | 4-7 months | ~1100 KB |
| **Seedling (C)** | **Pure C runtime** | **900-1600** | **6-10 months** | **~700 KB** |
| **Difference** | | **+300-600h** | **+2-3 months** | **-400 KB** |

**Trade-off Analysis:**

✅ **Advantages of Pure C:**
- 36% smaller WASM binary (700 KB vs 1100 KB)
- Faster runtime performance (no vtable overhead)
- Consistent with project philosophy
- Better for web delivery

❌ **Disadvantages of Pure C:**
- 33-60% longer development time
- More manual memory management
- More boilerplate code
- Higher bug risk (memory leaks)

**Recommendation:** Pure C is worth the extra development time for a web game like Seedling, where binary size and runtime performance directly impact user experience.

---

## Technical Details

### Generated Code Structure

**For each AS3 class, generate:**

**Header file** (`Player.h`):
```c
#ifndef PLAYER_H
#define PLAYER_H

#include "avm2_runtime.h"
#include "Mobile.h"  // Superclass

// Instance structure
typedef struct Player_instance {
    Mobile_instance base;  // Inherit from Mobile

    // Seedling Player has these fields:
    int32_t health;
    int32_t maxhealth;
    double x;
    double y;
    double vx;
    double vy;
    AS3Object* weapon;     // Current weapon
    AS3Object* inventory;  // Array of items
    // ... more fields ...
} Player_instance;

// Class object
extern AS3Class Player_class;

// Constructor
AS3Value* Player_constructor(AVM2Context* ctx, AS3Value* this_obj,
                             AS3Value** args, uint32_t arg_count);

// Methods
AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count);
AS3Value* Player_render(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count);
AS3Value* Player_takeDamage(AVM2Context* ctx, AS3Value* this_obj,
                            AS3Value** args, uint32_t arg_count);
// ... more methods ...

#endif
```

**Implementation file** (`Player.c`):
```c
#include "Player.h"
#include "avm2_opcodes.h"

// Method function objects
static AS3Function Player_update_func = {
    .name = "update",
    .native_func = Player_update,
    .param_count = 0,
};

static AS3Function Player_render_func = {
    .name = "render",
    .native_func = Player_render,
    .param_count = 0,
};

// ... more method objects ...

// Class object
AS3Class Player_class = {
    .name = "Player",
    .super_class = &Mobile_class,
    .constructor = Player_constructor,
    .methods = (AS3Function*[]){
        &Player_update_func,
        &Player_render_func,
        &Player_takeDamage_func,
        // ... more methods ...
    },
    .method_count = 15,
    .slot_count = 10,  // Number of instance fields
    .is_sealed = 1,
    .is_final = 0,
};

// Constructor implementation
AS3Value* Player_constructor(AVM2Context* ctx, AS3Value* this_obj,
                             AS3Value** args, uint32_t arg_count)
{
    Player_instance* inst = (Player_instance*)this_obj->value.obj;

    // Call super constructor (Mobile)
    Mobile_constructor(ctx, this_obj, args, arg_count);

    // Initialize Player fields
    inst->health = 100;
    inst->maxhealth = 100;
    inst->x = (arg_count > 0) ? toNumber(args[0]) : 0.0;
    inst->y = (arg_count > 1) ? toNumber(args[1]) : 0.0;
    inst->vx = 0.0;
    inst->vy = 0.0;
    inst->weapon = NULL;
    inst->inventory = createArray();
    // ... more initialization ...

    return createUndefined();
}

// Method implementations
AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count)
{
    // Translated from ABC bytecode
    // This is generated code, not hand-written

    Player_instance* this_inst = (Player_instance*)this_obj->value.obj;

    // Example bytecode translation:
    // getlocal_0
    // pushscope
    // getlocal_0
    // getproperty "vx"
    // getlocal_0
    // getproperty "x"
    // add
    // setproperty "x"

    AS3Value* vx_val = getProperty((AS3Object*)this_inst, "vx", NULL);
    AS3Value* x_val = getProperty((AS3Object*)this_inst, "x", NULL);

    // add
    AS3Value* new_x = add(x_val, vx_val);

    // setproperty
    setProperty((AS3Object*)this_inst, "x", NULL, new_x);

    release(vx_val);
    release(x_val);
    release(new_x);

    // ... more bytecode translation ...

    return createUndefined();
}
```

### Memory Management Strategy

**Reference Counting:**
```c
AS3Value* retain(AS3Value* v)
{
    if (v && v->type != TYPE_UNDEFINED && v->type != TYPE_NULL)
    {
        v->refcount++;
    }
    return v;
}

void release(AS3Value* v)
{
    if (!v || v->type == TYPE_UNDEFINED || v->type == TYPE_NULL)
        return;

    if (--v->refcount == 0)
    {
        // Free based on type
        switch (v->type)
        {
            case TYPE_STRING:
                free(v->value.s);
                break;

            case TYPE_OBJECT:
                release_object(v->value.obj);
                break;

            case TYPE_ARRAY:
                release_array(v->value.arr);
                break;

            case TYPE_FUNCTION:
                release_function(v->value.func);
                break;
        }
        free(v);
    }
}
```

**Handling Reference Cycles:**

For most Flash content, simple refcounting is sufficient. If needed, can add periodic mark-sweep:

```c
// Simple mark-sweep for cycle collection (optional)
void collectCycles(AVM2Context* ctx)
{
    // 1. Mark phase
    markObject(ctx->global_object);

    // 2. Sweep phase
    sweepUnmarked();
}
```

**Memory Pools (Optimization):**

For frequently allocated types (AS3Value, small objects):

```c
typedef struct MemoryPool {
    void* blocks;
    size_t block_size;
    uint32_t free_count;
    uint32_t* free_list;
} MemoryPool;

AS3Value* poolAlloc(MemoryPool* pool)
{
    if (pool->free_count == 0)
    {
        // Allocate new block
        expandPool(pool);
    }

    uint32_t index = pool->free_list[--pool->free_count];
    return (AS3Value*)((char*)pool->blocks + index * pool->block_size);
}
```

---

## Risk Assessment

### Technical Risks

| Risk | Severity | Mitigation | Pure C Impact |
|------|----------|------------|---------------|
| **Memory leaks** | High | Valgrind, AddressSanitizer, extensive testing | Higher risk in C |
| **ABC format complexity** | Medium | Reference Tamarin, RABCDAsm for validation | Same |
| **FlashPunk compatibility** | High | Early FlashPunk testing, incremental approach | Same |
| **Performance** | Medium | Profile, optimize hot paths, inline functions | Lower risk (C is faster) |
| **ECMA-262 semantics** | High | Reference Flash Player, thorough testing | Same |
| **Binary size** | Low | Pure C produces small binaries | Lower risk |
| **Debugging difficulty** | Medium | GDB, good logging, unit tests | Higher in C |

### Schedule Risks

| Risk | Severity | Mitigation | Pure C Impact |
|------|----------|------------|---------------|
| **Underestimated complexity** | High | Phase-based approach, stop after each milestone | Higher risk |
| **Scope creep** | Medium | Focus on Seedling only, defer other games | Same |
| **Testing overhead** | High | Incremental testing, automated test suite | Higher (more manual testing) |
| **Manual memory management bugs** | High | Valgrind every commit, thorough code review | New risk in C |

### Pure C Specific Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Dangling pointers** | High | Clear ownership rules, documentation |
| **Buffer overflows** | Medium | Bounds checking, use safe string functions |
| **Memory corruption** | High | AddressSanitizer, thorough testing |
| **Longer debug cycles** | Medium | Unit tests, good logging, GDB |

---

## Testing Strategy

### Unit Tests

**Test each component individually:**

1. **Type System**
   - Test toNumber() with all types
   - Test toString() with all types
   - Test toPrimitive() with objects
   - Test type conversions edge cases (NaN, Infinity, etc.)

2. **Opcodes**
   - Test each opcode with various inputs
   - Test edge cases (NaN, Infinity, null, undefined)
   - Test type coercion behavior
   - Test ECMA-262 compliance

3. **Object Model**
   - Test property access (get/set)
   - Test inheritance
   - Test prototype chain
   - Test method dispatch
   - Test getters/setters

4. **Flash APIs**
   - Test each class constructor
   - Test each method
   - Test interactions between classes

### Integration Tests

**Test components together:**

1. **ABC Parser + Code Generator**
   - Parse simple AS3 programs
   - Generate C code
   - Compile and run
   - Verify output matches expected

2. **FlashPunk Compilation**
   - Compile FlashPunk alone
   - Run FlashPunk test programs
   - Verify all features work

3. **Seedling Incremental**
   - Compile just Main.as
   - Add classes incrementally
   - Test each addition

### Functional Tests

**Test game features:**

1. **Boot Test**
   - Game loads
   - Main menu appears
   - No crashes

2. **Graphics Test**
   - Sprites render
   - Animations play
   - Layers work
   - Colors correct

3. **Input Test**
   - Keyboard responds
   - Player moves
   - Actions work

4. **Physics Test**
   - Collision detection works
   - Movement correct
   - Surface types work

5. **Combat Test**
   - Weapons work
   - Hit detection works
   - Damage system works
   - Enemy AI works

6. **Audio Test**
   - Sound effects play
   - Music plays
   - Volume control works

7. **Save Test**
   - Can save game
   - Can load game
   - Progress persists

8. **Completion Test**
   - Can complete entire game
   - All bosses beatable
   - All items collectable
   - All puzzles solvable

### Performance Tests

**Verify performance:**

1. **Frame Rate**
   - Consistent 60 FPS
   - No frame drops
   - Frame time < 16ms

2. **Memory**
   - No memory leaks (Valgrind)
   - Reasonable memory usage (< 100 MB)
   - No memory corruption (AddressSanitizer)

3. **Stress Test**
   - Many entities on screen
   - Still maintains 60 FPS
   - No crashes

### Memory Safety Tests (Pure C Specific)

**Verify no memory issues:**

1. **Valgrind**
   - Run entire game through Valgrind
   - Zero memory leaks
   - Zero invalid reads/writes

2. **AddressSanitizer**
   - Compile with -fsanitize=address
   - Run entire game
   - Zero errors

3. **Manual Review**
   - Code review all malloc/free pairs
   - Verify ownership clear
   - Check for double-frees

---

## Conclusion

### Summary

This plan outlines a **6-10 month effort** to implement minimal AS3 support in **pure C**, targeting the Seedling game specifically.

**Key Points:**
- **Language:** Pure C for runtime (C++ for build tools only)
- **Scope:** ~80-100 opcodes, ~50 Flash API classes
- **Effort:** 900-1600 hours (vs 2200-3900 for full AS3)
- **Duration:** 6-10 months (vs 18-28 for full AS3)
- **Benefits:** 55-60% less effort than full AS3, 36% smaller binary than C++
- **Trade-off:** 2-3 months longer than C++ approach, but better runtime performance

### Why Pure C is Right for Seedling

1. **Runtime Performance** - Seedling runs at 60 FPS, needs fast execution
2. **Binary Size** - Web game, smaller download = better UX
3. **Project Philosophy** - Matches LittleCube's guidance and existing codebase
4. **Long-term** - Other games will benefit from optimized runtime

### Next Steps

1. **Complete AS1/2 stabilization** (Phase 0)
2. **Analyze FlashPunk** (Phase 1)
3. **Start ABC parser** (Phase 2)
4. **Incremental implementation** following phases
5. **Regular testing** after each phase

### Success Criteria

✅ Seedling compiles from ABC to C
✅ Seedling runs in browser (WASM)
✅ Maintains 60 FPS
✅ All game features work
✅ Saves/loads work
✅ Can complete entire game
✅ WASM binary < 1 MB
✅ No memory leaks

This is an achievable goal that will establish a solid foundation for future AS3 games while maintaining the performance and size benefits of pure C.
