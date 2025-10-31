# Seedling Game - AS3 Implementation Guide

**Document Version:** 2.0
**Date:** October 29, 2025
**Game:** Seedling by Danny Yaroslavski (Alexander Ocias)
**Target:** Minimal AS3 implementation to run Seedling specifically
**Language:** Pure C

---

## Table of Contents

1. [Overview](#overview)
2. [Game Analysis](#game-analysis)
3. [Implementation Scope](#implementation-scope)
4. [Architecture](#architecture)
5. [Implementation Phases](#implementation-phases)
6. [Technical Details](#technical-details)
7. [Testing Strategy](#testing-strategy)

---

## Overview

### Goal

Implement the **minimum subset of AS3** required to run the game Seedling. This targeted approach focuses on what's actually needed rather than building complete AS3 support.

### Why Seedling?

- **Sprite-based graphics** - No vector rendering complexity
- **Well-structured code** - Clean FlashPunk architecture
- **Active community** - Popular game worth preserving
- **Realistic scope** - Medium complexity, achievable target
- **Archipelago potential** - Future randomizer integration

### Implementation Strategy

Build only what Seedling needs:
- **~80-100 opcodes** instead of all ~150 implemented opcodes
- **~50 Flash API classes** instead of 200+
- **FlashPunk engine** support (48 files)
- **Sprite rendering** focus (no complex vector graphics)

This significantly reduces implementation effort compared to full AS3.

---

## Game Analysis

### Project Structure

```
Seedling/
├── src/                     # 209 AS3 files (14,148 lines)
│   ├── Main.as             # Entry point (extends net.flashpunk.Engine)
│   ├── Game.as             # Main game world
│   ├── Player.as           # 1,967 lines - player logic
│   ├── Mobile.as           # Base class for moving entities
│   ├── Enemies/            # 38 enemy types
│   ├── NPCs/               # 17 NPC types
│   ├── Pickups/            # 23 item types
│   ├── Projectiles/        # 14 projectile types
│   ├── Puzzlements/        # 20 puzzle element types
│   ├── Scenery/            # 41 scenery types
│   └── net/flashpunk/      # FlashPunk engine (48 files)
├── assets/                 # 370+ embedded graphics, 82+ sounds
└── Shrum.as3proj          # Flash Player 11 project
```

### Core Game Systems

**1. Game Loop & Engine**
- FlashPunk `Engine` class base
- 60 FPS constant (Main.FPS = 60)
- Entity-based architecture
- World/Scene management
- Custom physics

**2. Graphics**
- Sprite-based rendering (Spritemap with animations)
- Tile-based levels (16x16 tiles)
- Multiple render layers
- Light/darkness system
- Screen shake effects
- Color tinting (damage flash, freeze effect)
- 370+ embedded PNG assets

**3. Physics/Collision**
- Custom velocity-based movement
- Pixel-perfect collision (Pixelmask)
- Rectangle collision (Hitbox)
- Multiple surface types (ice, water, lava, stairs)
- Solid collision detection
- Entity collision

**4. Input**
- Keyboard only
- Arrow keys for movement
- X, C, V keys for actions
- I key for inventory

**5. Audio**
- 82+ embedded sound effects
- 12+ music tracks
- Volume control
- Distance-based audio
- Looping sounds

**6. Persistence**
- SharedObject for saves
- Level state persistence
- Achievement tracking

**7. Combat**
- 6 weapon types
- Hit detection
- Damage system
- Knockback
- Invincibility frames
- Enemy AI
- 6 unique bosses

**8. UI**
- Custom inventory rendering
- Health display
- Message system
- Text rendering
- Menu system

### Flash Platform Dependencies

**Critical APIs:**

| Package | Classes | Purpose |
|---------|---------|---------|
| `flash.display.*` | BitmapData, Sprite, MovieClip, Graphics, Bitmap, Stage | ✅ Graphics rendering |
| `flash.geom.*` | Point, Rectangle, Matrix, ColorTransform | ✅ Math & transforms |
| `flash.media.*` | Sound, SoundChannel, SoundTransform, SoundMixer | ✅ Audio system |
| `flash.net.*` | SharedObject, URLLoader, URLRequest | ✅ Save system |
| `flash.events.*` | Event, KeyboardEvent, MouseEvent, TimerEvent | ✅ Events & input |
| `flash.utils.*` | getTimer, ByteArray, Dictionary, Timer | ✅ Utilities |
| `flash.text.*` | TextField, TextFormat, TextLineMetrics | ✅ Text rendering |
| `flash.ui.*` | Keyboard, Mouse | ✅ Input constants |

**Optional/Stub:**
- `flash.filters.*` - ColorMatrixFilter (can stub)
- `com.newgrounds.*` - Newgrounds API (stub)

### AS3 Language Features Used

**Core OOP:**
- ✅ Classes (207 classes)
- ✅ Inheritance (extensive)
- ✅ Method overriding (401+ overrides)
- ✅ Getters/setters (171 pairs)
- ✅ Static members
- ✅ Constructors
- ❌ Interfaces (NOT USED)

**Type System:**
- ✅ Strong typing
- ✅ `Vector.<T>` (99 occurrences)
- ✅ `Array`
- ✅ `Object` (as map)
- ✅ `Dictionary`
- ✅ Primitive types: int, uint, Number, Boolean, String

**Advanced Features:**
- ✅ Embedded assets ([Embed] metadata - 370+ tags)
- ✅ Property access (get/set)
- ✅ Method overloading (optional parameters)
- ✅ Namespaces (package structure)
- ❌ E4X/XML (NOT USED)
- ❌ Proxies (NOT USED)

---

## Implementation Scope

### Opcodes Required (~80-100 of ~150 implemented)

**Tier 1 - Critical (~40 opcodes):**
- **Arithmetic (12):** add, add_i, subtract, subtract_i, multiply, multiply_i, divide, modulo, negate, increment, increment_i, decrement
- **Comparison (7):** equals, strictequals, lessthan, lessequals, greaterthan, greaterequals, not
- **Stack (8):** dup, pop, swap, pushscope, popscope, getlocal_0-3, setlocal_0-3
- **Constants (10):** pushbyte, pushshort, pushint, pushuint, pushdouble, pushstring, pushtrue, pushfalse, pushnull, pushundefined
- **Control (8):** jump, iftrue, iffalse, ifeq, ifne, iflt, ifle, ifgt, ifge, returnvalue, returnvoid

**Tier 2 - Very Likely (~25 opcodes):**
- **Type Operations (8):** coerce_a, coerce_s, coerce_i, coerce_d, convert_i, convert_d, convert_s, typeof
- **Property Access (8):** getproperty, setproperty, initproperty, getsuper, setsuper, getslot, setslot, deleteproperty
- **Method Calls (6):** callproperty, callpropvoid, callsuper, constructprop, constructsuper, construct
- **Locals (3):** getlocal, setlocal, kill

**Tier 3 - Likely (~20 opcodes):**
- **Bitwise (7):** bitand, bitor, bitxor, bitnot, lshift, rshift, urshift
- **Object Creation (4):** newobject, newarray, newactivation, newclass
- **Advanced Calls (4):** call, callmethod, callstatic, newfunction
- **Scope (5):** findproperty, findpropstrict, getglobalscope, getscopeobject, getlex

**Tier 4 - Possibly (~15 opcodes):**
- **Iteration (4):** hasnext, hasnext2, nextname, nextvalue
- **Advanced Control (3):** lookupswitch, throw, label
- **Increment/Decrement (4):** inclocal, inclocal_i, declocal, declocal_i
- **Other (4):** instanceof, istype, istypelate, checkfilter

**Can Skip (~50+ opcodes):**
- ❌ E4X/XML operations
- ❌ Alchemy memory operations
- ❌ Advanced namespace operations
- ❌ Debug operations
- ❌ Unimplemented/reserved opcodes

### Flash APIs Required (~50 classes)

**flash.display (15 classes):**
- `BitmapData` ⭐ CRITICAL
- `Sprite` ⭐ CRITICAL
- `MovieClip` ⭐ CRITICAL
- `DisplayObject`, `Graphics`, `Bitmap`, `Stage`, `BlendMode`, `DisplayObjectContainer`, `Shape`, `Loader`, `LoaderInfo`

**flash.geom (5 classes):**
- `Point` ⭐ CRITICAL
- `Rectangle` ⭐ CRITICAL
- `Matrix`, `ColorTransform`, `Transform`

**flash.media (4 classes):**
- `Sound` ⭐ CRITICAL
- `SoundChannel`, `SoundTransform`, `SoundMixer`

**flash.net (3 classes):**
- `SharedObject` ⭐ CRITICAL (save system)
- `URLLoader`, `URLRequest`

**flash.events (4 classes):**
- `Event`, `KeyboardEvent`, `MouseEvent`, `TimerEvent`

**flash.utils (7 classes/functions):**
- `getTimer` ⭐ CRITICAL
- `ByteArray`, `Dictionary`, `Timer`, `getDefinitionByName`, `getQualifiedClassName`, `setTimeout`, `setInterval`

**flash.text (3 classes):**
- `TextField`, `TextFormat`, `TextLineMetrics`

**flash.ui (2 classes):**
- `Keyboard`, `Mouse`

**Can Skip:**
- All networking (except URLLoader for basic loading)
- All camera/microphone
- All 3D
- Most filters
- XML/E4X
- Worker threads
- File I/O
- Printing

---

## Architecture

### Core Data Structures

**Value Representation:**
```c
typedef enum {
    TYPE_UNDEFINED,
    TYPE_NULL,
    TYPE_BOOLEAN,
    TYPE_INT,        // 32-bit signed
    TYPE_UINT,       // 32-bit unsigned
    TYPE_NUMBER,     // 64-bit float
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

**Object Model:**
```c
typedef struct AS3Object {
    AS3Class* klass;
    AS3Object* prototype;
    HashMap* properties;      // Dynamic properties
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
    AS3Function** methods;    // Method table
    uint32_t method_count;
    uint32_t slot_count;
    uint8_t is_sealed;
    uint8_t is_final;
} AS3Class;
```

**Method Dispatch:**
```c
// Function pointer based dispatch (no C++ virtual functions)
typedef struct AS3Function {
    const char* name;
    AS3Value* (*native_func)(AVM2Context*, AS3Value*, AS3Value**, uint32_t);
    FunctionBody* body;       // For bytecode methods
    uint32_t param_count;
    AS3Value** closure_vars;
    uint32_t closure_count;
    uint32_t refcount;
} AS3Function;
```

### Memory Management

Simple reference counting:

```c
AS3Value* retain(AS3Value* v) {
    if (v && v->type != TYPE_UNDEFINED && v->type != TYPE_NULL) {
        v->refcount++;
    }
    return v;
}

void release(AS3Value* v) {
    if (!v || v->type == TYPE_UNDEFINED || v->type == TYPE_NULL) {
        return;
    }

    if (--v->refcount == 0) {
        // Free based on type
        switch (v->type) {
            case TYPE_STRING:
                free(v->value.s);
                break;
            case TYPE_OBJECT:
                release_object(v->value.obj);
                break;
            // ... etc
        }
        free(v);
    }
}
```

For reference cycles (rare in Seedling), can add optional mark-sweep collection.

### VM Context

```c
typedef struct AVM2Context {
    AS3Value** stack;          // Value stack
    uint32_t stack_size;
    uint32_t stack_top;

    AS3Value** scope_stack;    // Scope chain
    uint32_t scope_size;
    uint32_t scope_top;

    AS3Value** locals;         // Local variables
    uint32_t local_count;

    uint8_t* pc;              // Program counter

    AS3Object* global_object;
    HashMap* global_properties;

    ExceptionFrame* exception_frame;
} AVM2Context;
```

---

## Implementation Phases

### Phase 0: Preparation (Ongoing)

**Prerequisites:**
- Complete AS1/2 stabilization
- Fix runtime integration
- Document AS1/2 architecture
- All AS1/2 tests passing

### Phase 1: FlashPunk Analysis

**Goal:** Understand FlashPunk dependencies.

**Tasks:**
1. Read all 48 FlashPunk source files
2. Map class hierarchy
3. Identify Flash API dependencies
4. Document core algorithms
5. Create dependency graph
6. Create feature checklist

**Deliverables:**
- FlashPunk feature map
- Dependency graph
- Test checklist
- Priority order

### Phase 2: ABC Parser + Code Generator

**Goal:** Parse AS3 bytecode and generate C code.

**Part A: ABC Parser (C++ - build-time only)**
- Parse ABC file format
- Extract constant pools
- Parse method bodies
- Parse class definitions
- Parse traits
- Validation

**Part B: C Code Generator (C++ - build-time only)**
- Generate C structures for classes
- Generate C functions for methods
- Translate bytecode to runtime calls
- Handle control flow
- Generate constant tables

**Example Generated Code:**
```c
// Player.h
typedef struct Player_instance {
    Mobile_instance base;  // Inherits from Mobile
    int32_t health;
    double x, y;
    double vx, vy;
    AS3Object* weapon;
    AS3Object* inventory;
} Player_instance;

extern AS3Class Player_class;
AS3Value* Player_constructor(AVM2Context* ctx, AS3Value* this_obj,
                             AS3Value** args, uint32_t arg_count);
AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count);
```

### Phase 3: Core Type System + Basic Opcodes

**Goal:** Implement AS3 value system and core opcodes.

**Part A: Type System (Pure C)**
- AS3Value implementation
- Type conversion functions:
  - `toNumber` (ECMA-262 §9.3)
  - `toString` (ECMA-262 §9.8)
  - `toPrimitive` (ECMA-262 §9.1)
  - `toBoolean` (ECMA-262 §9.2)
  - `toInt32`, `toUint32`
- Type checking functions
- Reference counting

**Part B: Core Opcodes (~40)**
- Arithmetic: add, subtract, multiply, divide, etc.
- Comparison: equals, strictequals, lessthan, etc.
- Stack: dup, pop, swap, pushscope, popscope
- Constants: pushbyte, pushint, pushstring, etc.
- Control: jump, iftrue, iffalse, returnvalue, etc.

**Part C: VM Context**
- Stack operations
- Bytecode execution loop
- Instruction dispatch

**Example:**
```c
// Type conversion
double toNumber(AS3Value* input) {
    switch (input->type) {
        case TYPE_UNDEFINED: return NAN;
        case TYPE_NULL: return 0.0;
        case TYPE_BOOLEAN: return input->value.b ? 1.0 : 0.0;
        case TYPE_INT: return (double)input->value.i;
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

// Opcode implementation
void opcode_add(AVM2Context* ctx) {
    AS3Value* v2 = pop(ctx);
    AS3Value* v1 = pop(ctx);
    AS3Value* result;

    if (isNumber(v1) && isNumber(v2)) {
        result = createNumber(toNumber(v1) + toNumber(v2));
    } else if (isString(v1) || isString(v2)) {
        char* s1 = toString(v1);
        char* s2 = toString(v2);
        result = createString(concat(s1, s2));
        free(s1); free(s2);
    }
    // ... more cases ...

    push(ctx, result);
    release(v1); release(v2); release(result);
}
```

### Phase 4: Object Model + Flash APIs

**Goal:** Implement AS3 objects and core Flash APIs.

**Part A: Object Model (Pure C)**
- AS3Object implementation
- Property access (get/set)
- Prototype chain
- Dynamic properties (hash table)
- Slot access (fixed properties)
- AS3Class implementation
- Inheritance
- Method dispatch
- Namespace resolution

**Part B: Property/Method Opcodes (~25)**
- Property access: getproperty, setproperty, etc.
- Method calls: callproperty, callpropvoid, etc.
- Locals: getlocal, setlocal, kill
- Type operations: coerce, convert, typeof

**Part C: Core Flash APIs (Pure C)**

**Priority 1 - Graphics:**
- `BitmapData` - Pixel buffer
- `Sprite` - Display object
- `DisplayObject` - Base class
- `Graphics` - Drawing API
- `Bitmap` - Render bitmap
- `Stage` - Root display

**Priority 2 - Math:**
- `Point` - 2D point
- `Rectangle` - Bounding box
- `Matrix` - Transform matrix

**Priority 3 - Events:**
- `Event` - Base event
- `KeyboardEvent` - Input
- `EventDispatcher` - Event system

**Priority 4 - Utils:**
- `getTimer()` - Timing
- `ByteArray` - Binary data
- `Dictionary` - Hash table

**Example:**
```c
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
    Point_instance* pt1 = (Point_instance*)args[0]->value.obj;
    Point_instance* pt2 = (Point_instance*)args[1]->value.obj;

    double dx = pt2->x - pt1->x;
    double dy = pt2->y - pt1->y;
    return createNumber(sqrt(dx * dx + dy * dy));
}
```

### Phase 5: Complete Seedling Requirements

**Goal:** Implement all remaining features for Seedling.

**Part A: Remaining Opcodes (~35)**
- Bitwise operations
- Object creation: newobject, newarray, newclass
- Advanced calls: call, callmethod, callstatic
- Scope operations: findproperty, getlex
- Iteration: hasnext, nextname, nextvalue
- Advanced control: lookupswitch, throw

**Part B: Remaining Flash APIs**
- Sound system: Sound, SoundChannel, SoundTransform
- Persistence: SharedObject (save system)
- Text rendering: TextField, TextFormat
- Input: Keyboard, Mouse constants
- Advanced display: MovieClip, BlendMode, ColorTransform

**Part C: FlashPunk Integration**
- Compile FlashPunk
- Test FlashPunk features
- Fix issues

### Phase 6: Integration & Testing

**Goal:** Make Seedling fully playable.

**Part A: Initial Compilation**
- Parse Seedling.swf
- Generate C code
- Compile and link
- Boot to main menu

**Part B: Feature Completion**
- Fix graphics rendering
- Fix physics/collision
- Fix combat system
- Fix audio playback
- Fix save system

**Part C: Performance Optimization**
- Profile hot paths
- Optimize opcodes
- Optimize property access
- Reduce allocations
- Achieve 60 FPS

**Part D: Bug Fixing & Polish**
- Play through entire game
- Fix all crashes
- Fix gameplay bugs
- Fix visual/audio bugs
- Final performance tuning

---

## Technical Details

### Generated Code Structure

For each AS3 class, generate:

**Header (`Player.h`):**
```c
#ifndef PLAYER_H
#define PLAYER_H

#include "avm2_runtime.h"
#include "Mobile.h"

typedef struct Player_instance {
    Mobile_instance base;
    int32_t health;
    int32_t maxhealth;
    double x, y;
    double vx, vy;
    AS3Object* weapon;
    AS3Object* inventory;
} Player_instance;

extern AS3Class Player_class;
AS3Value* Player_constructor(AVM2Context* ctx, AS3Value* this_obj,
                             AS3Value** args, uint32_t arg_count);
AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count);
AS3Value* Player_render(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count);

#endif
```

**Implementation (`Player.c`):**
```c
#include "Player.h"

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

AS3Class Player_class = {
    .name = "Player",
    .super_class = &Mobile_class,
    .constructor = Player_constructor,
    .methods = (AS3Function*[]){
        &Player_update_func,
        &Player_render_func,
    },
    .method_count = 2,
    .slot_count = 8,
    .is_sealed = 1,
    .is_final = 0,
};

AS3Value* Player_constructor(AVM2Context* ctx, AS3Value* this_obj,
                             AS3Value** args, uint32_t arg_count)
{
    Player_instance* inst = (Player_instance*)this_obj->value.obj;

    // Call super constructor
    Mobile_constructor(ctx, this_obj, args, arg_count);

    // Initialize fields
    inst->health = 100;
    inst->maxhealth = 100;
    inst->x = (arg_count > 0) ? toNumber(args[0]) : 0.0;
    inst->y = (arg_count > 1) ? toNumber(args[1]) : 0.0;
    inst->vx = 0.0;
    inst->vy = 0.0;
    inst->weapon = NULL;
    inst->inventory = createArray();

    return createUndefined();
}

AS3Value* Player_update(AVM2Context* ctx, AS3Value* this_obj,
                       AS3Value** args, uint32_t arg_count)
{
    // Generated bytecode translation
    Player_instance* this_inst = (Player_instance*)this_obj->value.obj;

    // Example: x += vx
    AS3Value* vx_val = getProperty((AS3Object*)this_inst, "vx", NULL);
    AS3Value* x_val = getProperty((AS3Object*)this_inst, "x", NULL);
    AS3Value* new_x = add(x_val, vx_val);
    setProperty((AS3Object*)this_inst, "x", NULL, new_x);

    release(vx_val);
    release(x_val);
    release(new_x);

    // ... more bytecode translation ...

    return createUndefined();
}
```

### Memory Management

**Reference Counting:**
```c
AS3Value* retain(AS3Value* v) {
    if (v && v->type != TYPE_UNDEFINED && v->type != TYPE_NULL) {
        v->refcount++;
    }
    return v;
}

void release(AS3Value* v) {
    if (!v || v->type == TYPE_UNDEFINED || v->type == TYPE_NULL) {
        return;
    }

    if (--v->refcount == 0) {
        switch (v->type) {
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

**Memory Pools (Optimization):**

For frequently allocated types:
```c
typedef struct MemoryPool {
    void* blocks;
    size_t block_size;
    uint32_t free_count;
    uint32_t* free_list;
} MemoryPool;

AS3Value* poolAlloc(MemoryPool* pool) {
    if (pool->free_count == 0) {
        expandPool(pool);
    }
    uint32_t index = pool->free_list[--pool->free_count];
    return (AS3Value*)((char*)pool->blocks + index * pool->block_size);
}
```

---

## Testing Strategy

### Unit Tests

**Type System:**
- Test all conversion functions with edge cases
- Test NaN, Infinity, null, undefined
- Test ECMA-262 compliance

**Opcodes:**
- Test each opcode individually
- Test type coercion
- Test edge cases

**Object Model:**
- Test property access
- Test inheritance
- Test method dispatch
- Test getters/setters

**Flash APIs:**
- Test each class constructor
- Test each method
- Test class interactions

### Integration Tests

**ABC Parser + Code Generator:**
- Parse simple AS3 programs
- Generate and compile C code
- Verify output matches expected

**FlashPunk:**
- Compile FlashPunk
- Test core features
- Verify Entity system
- Verify collision detection

**Seedling Incremental:**
- Compile Main.as only
- Add classes incrementally
- Test after each addition

### Functional Tests

**Boot:** Game loads, main menu appears
**Graphics:** Sprites render, animations play
**Input:** Keyboard responds, player moves
**Physics:** Collision works, movement correct
**Combat:** Weapons work, hit detection works
**Audio:** Sounds play, music plays
**Save:** Can save and load
**Completion:** Can complete entire game

### Performance Tests

**Frame Rate:** Consistent 60 FPS, frame time < 16ms
**Memory:** No leaks (Valgrind), reasonable usage
**Stress:** Many entities, still 60 FPS

### Memory Safety Tests

**Valgrind:** Zero memory leaks, zero invalid reads/writes
**AddressSanitizer:** Compile with -fsanitize=address, zero errors
**Manual Review:** Check all malloc/free pairs, verify ownership

---

## Success Criteria

✅ Seedling compiles from ABC to C
✅ Seedling runs in browser (WASM)
✅ Maintains 60 FPS
✅ All game features work
✅ Saves/loads work
✅ Can complete entire game
✅ WASM binary < 1 MB
✅ No memory leaks

This targeted implementation provides a solid foundation for future AS3 games while keeping scope manageable and maintaining optimal performance.
