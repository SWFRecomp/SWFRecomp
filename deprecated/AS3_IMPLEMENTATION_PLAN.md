# ActionScript 3 (AS3) Implementation Plan for SWFRecomp

**Document Version:** 1.0
**Date:** October 28, 2024
**Status:** Planning Phase
**Recommended Approach:** Option A - Hybrid C/C++ Strategy

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Background](#background)
3. [Current Architecture](#current-architecture)
4. [AS3 vs AS1/2: Key Differences](#as3-vs-as12-key-differences)
5. [Why C++ is Superior for AS3](#why-c-is-superior-for-as3)
6. [Recommended Architecture: Hybrid Approach](#recommended-architecture-hybrid-approach)
7. [Implementation Roadmap](#implementation-roadmap)
8. [Technical Specifications](#technical-specifications)
9. [Effort Estimates](#effort-estimates)
10. [Risk Assessment](#risk-assessment)
11. [Success Criteria](#success-criteria)
12. [References](#references)

---

## Executive Summary

This document outlines a comprehensive plan to add ActionScript 3 (AS3) support to SWFRecomp. AS3 support would enable recompilation of modern Flash content, significantly expanding the project's scope beyond the current AS1/2 capabilities.

### Key Findings

- **AS3 is fundamentally different from AS1/2**: Uses AVM2 instead of AVM1, ABC bytecode format, 164 opcodes vs 40, full OOP with classes/inheritance
- **C++ implementation is strongly recommended**: Reduces implementation effort by ~40%, provides natural mapping for AS3 concepts, enables automatic memory management
- **Hybrid strategy is optimal**: Maintain existing C runtime for AS1/2, create new C++ runtime for AS3, coexist in same binary
- **Estimated effort with C++**: 10-16 months (1350-2350 hours) vs 14-20 months for pure C
- **Major components needed**: ABC parser, DoABC tag handler, C++ runtime library, class system, 164 opcode implementations

### Recommendation

Proceed with **Option A: Hybrid C/C++ Strategy** after completing AS1/2 stabilization. This provides the best balance of implementation efficiency, code maintainability, and architectural cleanliness.

---

## Background

### Project Context

SWFRecomp is a static recompiler that translates Adobe Flash SWF files into portable C code. Currently supports:
- ✅ ActionScript 1/2 (AVM1) via DoAction tags
- ✅ SWF 4 bytecode (40 opcodes implemented)
- ✅ Graphics recompilation (shapes, gradients, bitmaps)
- ✅ Native and WebAssembly compilation targets
- ⚠️ Integration with SWFModernRuntime in progress

### Why AS3 Support Matters

**Games requiring AS3:**
- Epic Battle Fantasy 5 (AS3) - APWorld in development by TheSpookster_2
- Many Newgrounds games (2006+)
- Kongregate games (2008+)
- Most commercial Flash games after 2006

**Current limitations:**
- AS1/2 only covers pre-2006 era Flash content
- Many preservation targets require AS3
- Archipelago integration goals include AS3 games

### Flash/ActionScript Timeline

| Year | Flash Version | ActionScript | SWF Version | VM | Notes |
|------|---------------|--------------|-------------|-----|-------|
| 1996-2005 | Flash 1-7 | AS1/AS2 | SWF 1-7 | AVM1 | Current SWFRecomp support |
| 2006 | Flash Player 9 | AS3 | SWF 9+ | AVM2 | **Target for this plan** |
| 2011 | - | - | - | - | Adobe donates Flex to Apache |
| 2020 | - | - | - | - | Flash Player EOL |

---

## Current Architecture

### SWFRecomp (Compiler)

**Language:** C++17
**Purpose:** Parse SWF, translate bytecode to C, generate source files
**Key files:**
- `src/swf.cpp` (61,997 bytes) - Main SWF parser
- `src/action/action.cpp` (9,317 bytes) - AS1/2 bytecode translator
- `src/tag.cpp` - Tag parsing
- `include/tag.hpp` - Tag definitions (DoABC already defined at line 27)

**Build system:** CMake with C++17 standard

### SWFModernRuntime (Runtime Library)

**Language:** C17
**Purpose:** Execute recompiled Flash code with GPU-accelerated rendering
**Key files:**
- `src/libswf/swf.c` - Runtime loop, frame execution
- `src/actionmodern/action.c` - AS1/2 opcode implementations
- `src/flashbang/flashbang.c` - Vulkan rendering engine

**Build system:** CMake with C17 standard

### Generated Code

**Current output:** Pure C code (`.c` files)
**Example:**
```c
void script_0(char* stack, u32* sp)
{
    // Push (String)
    PUSH_STR(str_0, 14);
    // Trace
    actionTrace(stack, sp);
}
```

**Integration:** Links against `libSWFModernRuntime.a` (430KB)

### Data Flow

```
┌─────────────┐
│  test.swf   │  SWF file with AS1/2 bytecode
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────┐
│  SWFRecomp (C++)                    │
│  - Parse SWF structure              │
│  - Extract DoAction tags            │
│  - Translate AS1/2 bytecode to C    │
└──────┬──────────────────────────────┘
       │
       │ Generates
       ▼
┌─────────────────────────────────────┐
│  C Source Files                     │
│  - RecompiledTags/*.c               │
│  - RecompiledScripts/*.c            │
└──────┬──────────────────────────────┘
       │
       │ Compiles with
       ▼
┌─────────────────────────────────────┐
│  SWFModernRuntime (C)               │
│  - Stack machine                    │
│  - Action implementations           │
│  - Vulkan rendering                 │
└──────┬──────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────┐
│  Native Executable                  │
└─────────────────────────────────────┘
```

---

## AS3 vs AS1/2: Key Differences

### Virtual Machine Architecture

| Aspect | AS1/2 (AVM1) | AS3 (AVM2) |
|--------|--------------|------------|
| **Bytecode Format** | Linear action stream | Structured ABC format |
| **File Container** | DoAction tag (12) | DoABC tag (82) |
| **Opcodes** | ~40 simple operations | 164 complex operations |
| **Type System** | Dynamic, untyped | Static typing + runtime checks |
| **Object Model** | Prototype-based | Class-based OOP |
| **Performance** | Interpreted | JIT-compiled (10x faster) |
| **VM Implementation** | AVM1 (legacy) | AVM2 (complete rewrite) |
| **Complexity** | Simple stack machine | Full VM with verifier, JIT |
| **Memory Model** | Manual/simple GC | Generational GC |

### Bytecode Format Comparison

**AS1/2 (DoAction tag):**
```
Tag Header (6 bytes)
├── Tag type: 12 (DoAction)
└── Length: N bytes

Action Stream:
├── Action code (1 byte)
├── Length (2 bytes, if code >= 0x80)
└── Parameters (variable)
...
├── Action code: 0x00 (End)
```

**AS3 (DoABC tag):**
```
Tag Header (6 bytes)
├── Tag type: 82 (DoABC)
└── Length: N bytes

DoABC Structure:
├── Flags (4 bytes, UI32)
├── Name (STRING, null-terminated)
└── ABC Data (remainder of tag)

ABC File Structure:
├── minor_version (U16)
├── major_version (U16)
├── constant_pool
│   ├── cpool_int[]      (variable-length integers)
│   ├── cpool_uint[]     (unsigned integers)
│   ├── cpool_double[]   (IEEE 754 doubles)
│   ├── cpool_string[]   (UTF-8 strings)
│   ├── cpool_namespace[]
│   ├── cpool_ns_set[]
│   └── cpool_multiname[]
├── method_info[]        (method signatures)
├── metadata_info[]      (annotations, metadata)
├── instance_info[]      (class instances)
├── class_info[]         (class definitions)
├── script_info[]        (script initialization)
└── method_body_info[]   (method bytecode)
    ├── max_stack
    ├── local_count
    ├── init_scope_depth
    ├── max_scope_depth
    ├── code[]           (actual bytecode)
    └── exception_info[] (try/catch/finally)
```

### Opcode Comparison

**AS1/2 (40 implemented opcodes):**
- Arithmetic: Add, Subtract, Multiply, Divide
- Comparison: Equals, Less
- Logical: And, Or, Not
- String: StringAdd, StringEquals, StringLength
- Stack: Push, Pop
- Control: Jump, If
- Variables: GetVariable, SetVariable
- Debug: Trace, GetTime

**AS3 (164 total opcodes):**

**Categories:**
1. **Arithmetic** (14): add, add_d, add_i, subtract, subtract_i, multiply, multiply_i, divide, modulo, negate, negate_i, increment, increment_i, decrement, decrement_i
2. **Bitwise** (7): bitand, bitor, bitxor, bitnot, lshift, rshift, urshift
3. **Type Conversion** (16): coerce, coerce_a, coerce_b, coerce_d, coerce_i, coerce_o, coerce_s, coerce_u, convert_b, convert_d, convert_i, convert_o, convert_s, convert_u, checkfilter, astype, astypelate
4. **Control Flow** (12): jump, ifeq, iffalse, ifge, ifgt, ifle, iflt, ifne, iftrue, ifstricteq, ifstrictne, lookupswitch
5. **Stack** (5): dup, pop, swap, pushscope, popscope
6. **Property Access** (10): getproperty, setproperty, deleteproperty, getlex, initproperty, getdescendants, findproperty, findpropstrict, getsuper, setsuper
7. **Memory/Slots** (10): getslot, setslot, getglobalslot, setglobalslot, getlocal, setlocal, getlocal_0, getlocal_1, getlocal_2, getlocal_3
8. **Function/Method Calls** (12): call, callmethod, callproperty, callproplex, callpropvoid, callstatic, callsuper, callsupervoid, construct, constructprop, constructsuper, newfunction
9. **Local Variables** (16): setlocal_0-3, inclocal, declocal, inclocal_i, declocal_i, kill
10. **Push Constants** (13): pushbyte, pushshort, pushint, pushuint, pushstring, pushdouble, pushnan, pushtrue, pushfalse, pushnull, pushundefined, pushnamespace, pushconstant
11. **Type Checking** (5): astype, astypelate, istype, istypelate, instanceof, typeof
12. **Comparison** (7): equals, strictequals, lessthan, lessequals, greaterthan, greaterequals, in
13. **Object/Array** (8): newobject, newarray, newclass, newactivation, getglobalscope, getscopeobject, hasnext, hasnext2
14. **Debugging** (6): debug, debugfile, debugline, bkpt, bkptline, timestamp
15. **Alchemy/Memory** (14): li8, li16, li32, lf32, lf64, si8, si16, si32, sf32, sf64, sxi1, sxi8, sxi16, sign_extend
16. **Exception Handling**: throw (handled in method_body exception_info)
17. **Misc** (20+): returnvalue, returnvoid, nop, label, nextname, nextvalue, typeof, not, dxns, dxnslate, applytype, coerce_b, esc_xattr, esc_xelem, etc.

### Object Model Comparison

**AS1/2:**
```javascript
// Prototype-based
function MyClass() {
    this.property = 42;
}
MyClass.prototype.method = function() {
    return this.property;
};
```

**AS3:**
```actionscript
// Class-based
package com.example {
    public class MyClass {
        private var property:int = 42;

        public function method():int {
            return property;
        }
    }
}
```

### SWF Tags Comparison

**AS1/2 uses:**
- DoAction (12) - Execute ActionScript
- DoInitAction (59) - Initialize on load

**AS3 uses:**
- DoABC (82) - Execute AS3 bytecode
- SymbolClass (76) - Map character IDs to class names
- DefineSceneAndFrameLabelData (86) - Scene information
- Metadata (77) - SWF metadata

**Currently in SWFRecomp:**
```cpp
// include/tag.hpp:27
SWF_TAG_DO_ABC = 82  // Defined but not implemented

// src/swf.cpp:869
if ((flags & 0b00001000) != 0)
{
    //EXC("ActionScript 3 SWFs not implemented.\n");  // Detection exists
}

// src/swf.cpp:875-880
case SWF_TAG_SYMBOL_CLASS:
{
    cur_pos += tag.length;  // Currently skipped
    break;
}

// src/swf.cpp:889-894
//~ case SWF_TAG_DO_ABC:  // Commented out
//~ {
    //~ cur_pos += tag.length;
    //~ break;
//~ }
```

---

## Why C++ is Superior for AS3

### Problem 1: Type System

**AS3 Requirement:**
- Dynamic typing with runtime type checking
- Value can be: int, uint, Number, String, Boolean, Object, Array, Function, Class, null, undefined
- Each operation must check types and convert as needed

**C Approach (painful):**
```c
typedef enum {
    TYPE_INT, TYPE_UINT, TYPE_NUMBER, TYPE_STRING,
    TYPE_BOOLEAN, TYPE_OBJECT, TYPE_ARRAY, TYPE_FUNCTION,
    TYPE_NULL, TYPE_UNDEFINED
} ValueType;

typedef struct {
    ValueType type;
    union {
        int32_t i;
        uint32_t ui;
        double d;
        char* s;
        uint8_t b;
        void* obj;
    } value;
} AS3Value;

// Every operation needs manual type checking
AS3Value* add(AS3Value* a, AS3Value* b) {
    if (a->type == TYPE_NUMBER && b->type == TYPE_NUMBER) {
        return createNumber(a->value.d + b->value.d);
    } else if (a->type == TYPE_STRING || b->type == TYPE_STRING) {
        char* str1 = toString(a);
        char* str2 = toString(b);
        char* result = concat(str1, str2);
        return createString(result);
    }
    // ... many more cases
}
```

**C++ Approach (elegant):**
```cpp
class AS3Value {
public:
    virtual ~AS3Value() = default;
    virtual AS3Value* add(AS3Value* other) = 0;
    virtual double toNumber() = 0;
    virtual std::string toString() = 0;
};

class AS3Number : public AS3Value {
    double value;
public:
    AS3Value* add(AS3Value* other) override {
        return new AS3Number(value + other->toNumber());
    }
};

class AS3String : public AS3Value {
    std::string value;
public:
    AS3Value* add(AS3Value* other) override {
        return new AS3String(value + other->toString());
    }
};
```

### Problem 2: Memory Management

**AS3 Requirement:**
- Automatic garbage collection
- Reference cycles must be handled
- Objects can have arbitrary lifetimes

**C Approach (complex):**
```c
// Manual reference counting
typedef struct Object {
    uint32_t refcount;
    void (*destructor)(struct Object*);
    // ... object data
} Object;

void retain(Object* obj) {
    if (obj) obj->refcount++;
}

void release(Object* obj) {
    if (obj && --obj->refcount == 0) {
        if (obj->destructor) obj->destructor(obj);
        free(obj);
    }
}

// Or implement mark-and-sweep GC (500+ lines of code)
```

**C++ Approach (automatic):**
```cpp
// Smart pointers handle everything
std::shared_ptr<AS3Object> obj = std::make_shared<AS3Object>();
// Automatic reference counting, handles cycles with weak_ptr
// No manual memory management needed
```

### Problem 3: Exception Handling

**AS3 Requirement:**
- try/catch/finally blocks
- Exception objects with stack traces
- Exception propagation across call stack

**C Approach (ugly):**
```c
// Using setjmp/longjmp
#include <setjmp.h>

typedef struct {
    jmp_buf buf;
    AS3Value* exception;
    struct ExceptionFrame* previous;
} ExceptionFrame;

ExceptionFrame* current_exception_frame = NULL;

#define TRY \
    ExceptionFrame __frame; \
    __frame.previous = current_exception_frame; \
    current_exception_frame = &__frame; \
    if (setjmp(__frame.buf) == 0) {

#define CATCH \
    } else {

#define THROW(ex) \
    current_exception_frame->exception = ex; \
    longjmp(current_exception_frame->buf, 1);
```

**C++ Approach (native):**
```cpp
try {
    // AS3 code
} catch (AS3Error& e) {
    // Handle exception
} finally {
    // Cleanup (use RAII instead)
}
```

### Problem 4: Closures and Function Objects

**AS3 Requirement:**
- Functions are first-class objects
- Can capture variables from outer scope
- Can be passed as arguments, returned from functions

**C Approach (verbose):**
```c
typedef struct {
    void (*func)(void*, AS3Value*, int, AS3Value*);
    void* closure_data;
    uint32_t refcount;
} FunctionObject;

// For each closure, generate a struct
typedef struct {
    int captured_var1;
    char* captured_var2;
} MyClosure;

void my_closure_func(void* closure, AS3Value* args, int argc, AS3Value* result) {
    MyClosure* data = (MyClosure*)closure;
    // Use data->captured_var1, data->captured_var2
}

FunctionObject* create_closure(int var1, char* var2) {
    MyClosure* data = malloc(sizeof(MyClosure));
    data->captured_var1 = var1;
    data->captured_var2 = strdup(var2);

    FunctionObject* func = malloc(sizeof(FunctionObject));
    func->func = my_closure_func;
    func->closure_data = data;
    func->refcount = 1;
    return func;
}
```

**C++ Approach (concise):**
```cpp
// Lambda with capture
auto closure = [captured_var1, captured_var2](AS3Value** args, int argc) {
    return new AS3Number(captured_var1 + args[0]->toNumber());
};

// Or std::function
std::function<AS3Value*(AS3Value**, int)> func = closure;
```

### Problem 5: Object-Oriented Features

**AS3 Requirement:**
- Classes with inheritance
- Virtual methods
- Interfaces
- Abstract classes
- Access control (public, private, protected, internal)
- Namespaces
- Packages

**C Approach (manual vtables):**
```c
// Base class
typedef struct AS3Object_vtable {
    void (*constructor)(struct AS3Object*);
    void (*destructor)(struct AS3Object*);
    AS3Value* (*getProperty)(struct AS3Object*, const char*);
    void (*setProperty)(struct AS3Object*, const char*, AS3Value*);
} AS3Object_vtable;

typedef struct AS3Object {
    AS3Object_vtable* vtable;
    uint32_t refcount;
    // ... properties
} AS3Object;

// Derived class
typedef struct MyClass {
    AS3Object base;  // Inheritance by embedding
    int my_property;
} MyClass;

AS3Object_vtable MyClass_vtable = {
    .constructor = MyClass_constructor,
    .destructor = MyClass_destructor,
    .getProperty = MyClass_getProperty,
    .setProperty = MyClass_setProperty
};

// Manual dispatch
AS3Value* call_method(AS3Object* obj, const char* method) {
    return obj->vtable->getProperty(obj, method);
}
```

**C++ Approach (native):**
```cpp
class AS3Object {
public:
    virtual ~AS3Object() = default;
    virtual AS3Value* getProperty(const std::string& name) = 0;
    virtual void setProperty(const std::string& name, AS3Value* value) = 0;
};

class MyClass : public AS3Object {
    int my_property;
public:
    AS3Value* getProperty(const std::string& name) override {
        if (name == "myProperty") return new AS3Int(my_property);
        return AS3Object::getProperty(name);
    }
};

// Automatic dispatch
AS3Value* value = obj->getProperty("myProperty");
```

### Code Size Comparison

**Estimated lines of code for full AS3 implementation:**

| Component | C Implementation | C++ Implementation | Reduction |
|-----------|------------------|-------------------|-----------|
| Type system | 2,000 | 800 | 60% |
| Memory management | 1,500 | 200 (using smart_ptr) | 87% |
| Object model | 3,000 | 1,200 | 60% |
| Exception handling | 800 | 100 (native) | 88% |
| Closures | 1,200 | 300 | 75% |
| ABC parser | 5,000 | 3,000 | 40% |
| Opcode implementations | 8,000 | 5,000 | 38% |
| Runtime support | 3,500 | 2,000 | 43% |
| **TOTAL** | **25,000** | **12,600** | **~50%** |

### Performance Considerations

**C++ Overhead:**
- Virtual function calls: ~1-2 CPU cycles overhead (negligible)
- Smart pointers: Reference counting overhead (comparable to manual)
- Exception handling: Zero cost if not thrown
- RTTI (if used): Minimal memory overhead

**C++ Benefits:**
- Better compiler optimizations (more semantic information)
- Inline templates can be faster than function pointers
- Move semantics reduce copying
- Modern C++ (C++17) is as fast as C for most operations

**Verdict:** C++ overhead is negligible compared to implementation complexity reduction.

---

## Recommended Architecture: Hybrid Approach

### Option A: Hybrid C/C++ Strategy ✅ RECOMMENDED

**Concept:** Maintain existing C runtime for AS1/2, create new C++ runtime for AS3, coexist in same binary.

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         SWFRecomp (C++)                          │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  SWF Parser (existing)                                     │ │
│  │  - Detects SWF version                                     │ │
│  │  - Checks for DoAction vs DoABC tags                       │ │
│  └────────────────┬───────────────────────────────────────────┘ │
│                   │                                              │
│         ┌─────────┴─────────┐                                    │
│         ▼                   ▼                                    │
│  ┌──────────────┐    ┌──────────────────┐                       │
│  │ AS1/2 Path   │    │   AS3 Path       │                       │
│  │ (existing)   │    │   (new)          │                       │
│  │              │    │                  │                       │
│  │ DoAction     │    │ DoABC            │                       │
│  │ ↓            │    │ ↓                │                       │
│  │ action.cpp   │    │ abc_parser.cpp   │                       │
│  │ ↓            │    │ ↓                │                       │
│  │ Generate     │    │ Generate         │                       │
│  │ C code       │    │ C++ code         │                       │
│  └──────┬───────┘    └────────┬─────────┘                       │
│         │                     │                                  │
└─────────┼─────────────────────┼──────────────────────────────────┘
          │                     │
          │ Output              │ Output
          ▼                     ▼
  ┌───────────────┐     ┌─────────────────┐
  │ C Files       │     │ C++ Files       │
  │ *.c           │     │ *.cpp           │
  │               │     │                 │
  │ script_0.c    │     │ as3_script_0.cpp│
  │ tagMain.c     │     │ as3_classes.cpp │
  │ draws.c       │     │ as3_main.cpp    │
  └───────┬───────┘     └────────┬────────┘
          │                      │
          │ Links with           │ Links with
          ▼                      ▼
  ┌──────────────────┐   ┌──────────────────────┐
  │ SWFModernRuntime │   │ SWFModernRuntime_AS3 │
  │ (C, existing)    │   │ (C++, new)           │
  │                  │   │                      │
  │ - Stack machine  │   │ - AVM2 VM            │
  │ - 40 opcodes     │   │ - 164 opcodes        │
  │ - Simple types   │   │ - OOP support        │
  │ - SDL3/Vulkan    │   │ - Smart pointers     │
  └──────────────────┘   │ - Exception handling │
                         │ - SDL3/Vulkan (same) │
                         └──────────────────────┘
                                    │
                                    ▼
                         ┌──────────────────────┐
                         │ Native Executable    │
                         │                      │
                         │ Contains:            │
                         │ - C runtime (AS1/2)  │
                         │ - C++ runtime (AS3)  │
                         │ - Both coexist       │
                         └──────────────────────┘
```

### Directory Structure

```
SWFRecomp/
├── src/
│   ├── action/              # Existing AS1/2
│   │   └── action.cpp
│   ├── abc/                 # New AS3 components
│   │   ├── abc_parser.cpp   # ABC file parsing
│   │   ├── abc_method.cpp   # Method parsing
│   │   ├── abc_class.cpp    # Class definitions
│   │   ├── abc_script.cpp   # Script initialization
│   │   └── abc_codegen.cpp  # C++ code generation
│   ├── swf.cpp              # Modified to route AS1/2 vs AS3
│   └── ...
├── include/
│   ├── action/
│   │   └── action.hpp
│   └── abc/                 # New AS3 headers
│       ├── abc_types.hpp    # ABC data structures
│       ├── abc_opcodes.hpp  # 164 opcode definitions
│       └── abc_context.hpp  # Code generation context
└── tests/
    ├── trace_swf_4/         # Existing AS1/2 test
    └── as3_hello_world/     # New AS3 test

SWFModernRuntime/
├── src/
│   ├── libswf/              # Existing C runtime (AS1/2)
│   │   ├── swf.c
│   │   └── tag.c
│   ├── actionmodern/
│   │   ├── action.c
│   │   └── variables.c
│   └── avm2/                # New C++ runtime (AS3)
│       ├── avm2_vm.cpp      # AVM2 virtual machine
│       ├── avm2_types.cpp   # Type system
│       ├── avm2_object.cpp  # Object model
│       ├── avm2_class.cpp   # Class system
│       ├── avm2_opcodes.cpp # 164 opcode implementations
│       ├── avm2_builtins.cpp# Built-in classes (Array, Math, etc.)
│       ├── avm2_memory.cpp  # Smart pointer wrappers
│       └── avm2_namespace.cpp # Namespace resolution
├── include/
│   ├── libswf/              # Existing C headers
│   │   └── recomp.h
│   └── avm2/                # New C++ headers
│       ├── avm2_runtime.hpp
│       ├── avm2_types.hpp
│       └── ...
└── CMakeLists.txt           # Modified for C and C++ compilation
```

### Build System Changes

**SWFRecomp CMakeLists.txt:**
```cmake
# Already supports both (no change needed)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

# Add new ABC source files
set(ABC_SOURCES
    ${CMAKE_SOURCE_DIR}/src/abc/abc_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/abc/abc_method.cpp
    ${CMAKE_SOURCE_DIR}/src/abc/abc_class.cpp
    ${CMAKE_SOURCE_DIR}/src/abc/abc_script.cpp
    ${CMAKE_SOURCE_DIR}/src/abc/abc_codegen.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES} ${ABC_SOURCES})
```

**SWFModernRuntime CMakeLists.txt:**
```cmake
# Change from C-only to C and C++
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)  # Add this
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Add this

# Existing C sources
set(C_SOURCES
    ${PROJECT_SOURCE_DIR}/src/libswf/swf.c
    ${PROJECT_SOURCE_DIR}/src/libswf/tag.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/action.c
    ${PROJECT_SOURCE_DIR}/src/actionmodern/variables.c
    ${PROJECT_SOURCE_DIR}/src/flashbang/flashbang.c
    ${PROJECT_SOURCE_DIR}/src/utils.c
    ${PROJECT_SOURCE_DIR}/lib/c-hashmap/map.c
)

# New C++ sources
set(CXX_SOURCES
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_vm.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_types.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_object.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_class.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_opcodes.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_builtins.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_memory.cpp
    ${PROJECT_SOURCE_DIR}/src/avm2/avm2_namespace.cpp
)

add_library(${PROJECT_NAME} STATIC ${C_SOURCES} ${CXX_SOURCES})
```

### Code Generation Examples

**AS1/2 (existing):**
```c
// Generated: RecompiledScripts/script_0.c
#include <recomp.h>

void script_0(char* stack, u32* sp)
{
    // Push (String)
    PUSH_STR(str_0, 14);
    // Trace
    actionTrace(stack, sp);
}
```

**AS3 (new):**
```cpp
// Generated: RecompiledScripts/as3_script_0.cpp
#include <avm2_runtime.hpp>

using namespace avm2;

void as3_script_0(AVM2Context* ctx)
{
    // GetLex "trace" (get global trace function)
    auto trace_func = ctx->getGlobalProperty("trace");

    // PushString "Hello from AS3"
    auto str = std::make_shared<AS3String>("Hello from AS3");
    ctx->push(str);

    // CallProperty with 1 argument
    ctx->callFunction(trace_func, 1);
}
```

**AS3 Class (new):**
```cpp
// Generated: RecompiledScripts/as3_MyClass.cpp
#include <avm2_runtime.hpp>

namespace as3_generated {

class MyClass : public avm2::AS3Object {
private:
    std::shared_ptr<avm2::AS3Int> myProperty;

public:
    MyClass() : myProperty(std::make_shared<avm2::AS3Int>(42)) {
        registerProperty("myProperty", myProperty);
    }

    std::shared_ptr<avm2::AS3Value> myMethod(
        const std::vector<std::shared_ptr<avm2::AS3Value>>& args)
    {
        return myProperty;
    }
};

} // namespace as3_generated
```

### Runtime Interface

**C Runtime (existing):**
```c
// include/libswf/recomp.h
void swfStart(frame_func* frame_funcs);
void actionTrace(char* stack, u32* sp);
void actionAdd(char* stack, u32* sp);
// ... 40 action functions
```

**C++ Runtime (new):**
```cpp
// include/avm2/avm2_runtime.hpp
namespace avm2 {

class AVM2Context {
public:
    void push(std::shared_ptr<AS3Value> value);
    std::shared_ptr<AS3Value> pop();
    std::shared_ptr<AS3Value> getGlobalProperty(const std::string& name);
    void callFunction(std::shared_ptr<AS3Value> func, int argc);
    // ... AVM2 operations
};

void avm2Start(script_func* scripts, class_info* classes);

} // namespace avm2
```

### Detection and Routing in SWFRecomp

**Modified src/swf.cpp:**
```cpp
// Early detection
bool has_as3 = false;
bool has_as1_2 = false;

// In parseAllTags():
case SWF_TAG_FILE_ATTRIBUTES:
{
    u8 flags = (u8) tag.fields[0].value;
    if ((flags & 0b00001000) != 0)
    {
        has_as3 = true;  // Enable AS3 mode
    }
    break;
}

case SWF_TAG_DO_ACTION:
{
    has_as1_2 = true;
    // Existing AS1/2 handling
    break;
}

case SWF_TAG_DO_ABC:
{
    has_as3 = true;

    // Parse DoABC tag
    tag.setFieldCount(2);
    tag.configureNextField(SWF_FIELD_UI32);   // flags
    tag.configureNextField(SWF_FIELD_STRING); // name
    tag.parseFields(cur_pos);

    u32 flags = (u32) tag.fields[0].value;
    char* abc_name = (char*) tag.fields[1].value;

    // ABC data is remainder of tag
    size_t abc_size = tag.length - (cur_pos - tag_start);
    char* abc_data = cur_pos;

    // Parse ABC bytecode using new parser
    ABC::ABCFile abc;
    abc.parse(abc_data, abc_size);

    // Generate C++ code
    abc.generateCppCode(context, flags, abc_name);

    cur_pos += abc_size;
    break;
}

// At end of parsing:
if (has_as3 && has_as1_2) {
    // Hybrid SWF (rare but possible)
    // Generate both C and C++ code
}
```

### Compatibility

**C++ features available (C++17):**
- ✅ Classes and inheritance
- ✅ Virtual functions
- ✅ Templates
- ✅ Smart pointers (std::shared_ptr, std::unique_ptr)
- ✅ Standard library (std::vector, std::map, std::string)
- ✅ Lambda expressions
- ✅ Exception handling
- ✅ RAII (Resource Acquisition Is Initialization)
- ✅ Move semantics
- ✅ std::optional (C++17)
- ✅ std::variant (C++17)

**All dependencies support C++:**
- ✅ SDL3 - C API, works from C++
- ✅ Vulkan - C API, C++ wrappers available
- ✅ zlib - Pure C, works from C++
- ✅ LZMA - Pure C, works from C++

**Platform compatibility:**
- ✅ Linux (GCC 7+, Clang 5+)
- ✅ Windows (MSVC 2017+, MinGW)
- ✅ macOS (Xcode 10+)
- ✅ WebAssembly (Emscripten supports C++17)

### Migration Path

**Phase 0: Preparation (Current)**
- ✅ SWFRecomp already uses C++17
- ✅ DoABC tag defined in tag.hpp
- ✅ AS3 detection exists but disabled
- ⏳ Complete AS1/2 implementation
- ⏳ Fix runtime integration issues

**Phase 1: Foundation**
- Create `src/abc/` directory
- Create `include/abc/` directory
- Add C++ support to SWFModernRuntime CMakeLists
- Create basic AVM2 namespace and types

**Phase 2: Coexistence**
- Both C and C++ runtimes compile
- Both link into same executable
- Detection code routes to appropriate runtime
- Test with mixed AS1/2 and AS3 SWFs (if they exist)

**Phase 3: Full AS3 Support**
- Implement ABC parser
- Implement 164 opcodes
- Implement class system
- Complete built-in library

**Rollback strategy:**
- AS1/2 code unchanged - zero risk
- AS3 code in separate namespace - no conflicts
- Can be disabled with CMake option if needed

---

## Implementation Roadmap

### Overview

**Total duration:** 10-16 months (1350-2350 hours with C++)
**Phases:** 5 major phases
**Dependencies:** None (can start after AS1/2 stabilization)

### Phase 0: Prerequisites (Current - 2 months)

**Status:** In progress
**Blockers:** Must complete before Phase 1

**Tasks:**
- [x] Complete AS1/2 opcode implementation
- [x] Document current architecture
- [ ] Fix runtime integration segfault (PROJECT_STATUS.md:936-939)
- [ ] Stabilize SWFModernRuntime API
- [ ] Complete test suite for AS1/2 (50/50 tests passing)
- [ ] Document AS1/2 codegen patterns

**Deliverables:**
- Stable AS1/2 implementation
- No known crashes
- Comprehensive test coverage
- API documentation

**Exit criteria:**
- All 50 AS1/2 tests passing
- No segfaults in runtime
- API frozen and documented

---

### Phase 1: ABC Parser Foundation (3-4 months)

**Goal:** Parse ABC bytecode into data structures
**Language:** C++
**Estimated effort:** 300-500 hours

#### Milestones

**1.1: Project Structure Setup (1 week)**
- [ ] Create `src/abc/` directory
- [ ] Create `include/abc/` directory
- [ ] Add ABC sources to CMakeLists.txt
- [ ] Set up test infrastructure
- [ ] Create sample ABC files for testing

**1.2: ABC File Format (3 weeks)**
- [ ] Define ABC data structures in `abc_types.hpp`
  - abcFile structure
  - Constant pool types
  - Method info
  - Class info
  - Script info
  - Method body info
- [ ] Implement binary reader utility
  - U8, U16, U30 (variable-length) readers
  - String reader (UTF-8)
- [ ] Parse ABC header (minor/major version)
- [ ] Validate ABC file integrity

**1.3: Constant Pool Parsing (2 weeks)**
- [ ] Parse integer constant pool (cpool_int)
- [ ] Parse unsigned int constant pool (cpool_uint)
- [ ] Parse double constant pool (cpool_double)
- [ ] Parse string constant pool (cpool_string)
- [ ] Parse namespace constant pool (cpool_namespace)
- [ ] Parse namespace set constant pool (cpool_ns_set)
- [ ] Parse multiname constant pool (cpool_multiname)
- [ ] Validate all constant pool references

**1.4: Method Parsing (2 weeks)**
- [ ] Parse method_info structures
  - Parameter count
  - Return type
  - Parameter types
  - Method name
  - Flags (NEED_ARGUMENTS, NEED_ACTIVATION, etc.)
- [ ] Parse method_body_info structures
  - max_stack, local_count
  - init_scope_depth, max_scope_depth
  - Bytecode instructions
  - Exception handlers
  - Traits
- [ ] Validate method signatures

**1.5: Class Parsing (2 weeks)**
- [ ] Parse instance_info structures
  - Class name (multiname)
  - Superclass name
  - Interface list
  - Instance initializer
  - Instance traits
- [ ] Parse class_info structures
  - Class initializer
  - Class traits
- [ ] Parse trait structures
  - Trait_Slot (var/const)
  - Trait_Method
  - Trait_Getter
  - Trait_Setter
  - Trait_Class
  - Trait_Function
- [ ] Build class hierarchy tree

**1.6: Script Parsing (1 week)**
- [ ] Parse script_info structures
- [ ] Parse script initialization methods
- [ ] Parse script traits

**1.7: Metadata Parsing (1 week)**
- [ ] Parse metadata_info structures
- [ ] Associate metadata with classes/methods

**1.8: Testing and Validation (2 weeks)**
- [ ] Create test suite with 50+ ABC files
  - Simple AS3 scripts
  - Class definitions
  - Inheritance hierarchies
  - Interface implementations
  - Complex namespacing
- [ ] Validate against reference implementation (Tamarin)
- [ ] Fuzz testing with invalid ABC files
- [ ] Performance benchmarking

#### Deliverables

**Code:**
- `src/abc/abc_parser.cpp/hpp` (est. 1500 lines)
- `src/abc/abc_method.cpp/hpp` (est. 800 lines)
- `src/abc/abc_class.cpp/hpp` (est. 1000 lines)
- `src/abc/abc_script.cpp/hpp` (est. 400 lines)
- `include/abc/abc_types.hpp` (est. 800 lines)
- **Total:** ~4500 lines of C++

**Tests:**
- 50+ ABC files
- Unit tests for each component
- Integration tests
- Validation against reference data

**Documentation:**
- ABC format reference
- Parser API documentation
- Data structure diagrams

#### Exit Criteria

- [ ] Can parse any valid ABC file
- [ ] All data structures populated correctly
- [ ] Zero memory leaks (valgrind)
- [ ] Test suite passing (50/50)
- [ ] Performance acceptable (< 100ms for typical ABC)

---

### Phase 2: Basic Code Generation (2-3 months)

**Goal:** Generate C++ code for simple AS3 scripts
**Language:** C++
**Estimated effort:** 200-350 hours

#### Milestones

**2.1: Code Generation Framework (2 weeks)**
- [ ] Design C++ output format
- [ ] Create code generation context
- [ ] Set up output file structure
  - `as3_script_<N>.cpp`
  - `as3_classes.cpp`
  - `as3_main.cpp`
- [ ] Implement code formatting utilities

**2.2: Simple Opcodes (4 weeks)**
Implement ~50 basic opcodes:
- [ ] **Arithmetic** (10): add, add_i, subtract, subtract_i, multiply, multiply_i, divide, modulo, negate, increment
- [ ] **Comparison** (6): equals, strictequals, lessthan, lessequals, greaterthan, greaterequals
- [ ] **Logical** (3): not, iftrue, iffalse
- [ ] **Stack** (8): dup, pop, swap, push\*, getlocal_0-3, setlocal_0-3
- [ ] **Constants** (10): pushbyte, pushshort, pushint, pushuint, pushstring, pushdouble, pushnan, pushtrue, pushfalse, pushnull
- [ ] **Control flow** (5): jump, label, returnvalue, returnvoid, nop
- [ ] **Debug** (3): debug, debugline, debugfile

**2.3: Method Code Generation (2 weeks)**
- [ ] Generate method signatures
- [ ] Generate method bodies
- [ ] Handle local variables
- [ ] Generate return statements
- [ ] Map bytecode to C++ statements

**2.4: Basic Runtime Functions (3 weeks)**
- [ ] Implement AS3Value base class
- [ ] Implement AS3Int, AS3UInt, AS3Number
- [ ] Implement AS3Boolean, AS3String
- [ ] Implement AS3Null, AS3Undefined
- [ ] Implement basic type conversions
- [ ] Implement basic arithmetic operations

**2.5: Testing (2 weeks)**
- [ ] Test with "Hello World" AS3 SWF
- [ ] Test arithmetic operations
- [ ] Test control flow
- [ ] Test local variables
- [ ] Validate output correctness

#### Sample Output

**Input ABC bytecode:**
```
method_body:
  max_stack: 2
  local_count: 1
  code:
    getlocal_0        // this
    pushscope
    findpropstrict "trace"
    pushstring "Hello AS3"
    callpropvoid 1
    returnvoid
```

**Generated C++:**
```cpp
#include <avm2_runtime.hpp>

void script_init(avm2::AVM2Context* ctx) {
    // getlocal_0
    auto local_0 = ctx->getLocal(0);

    // pushscope
    ctx->pushScope(local_0);

    // findpropstrict "trace"
    auto trace_prop = ctx->findPropertyStrict("trace");

    // pushstring "Hello AS3"
    ctx->push(std::make_shared<avm2::AS3String>("Hello AS3"));

    // callpropvoid 1
    ctx->callPropertyVoid(trace_prop, 1);

    // returnvoid
    return;
}
```

#### Deliverables

**Code:**
- `src/abc/abc_codegen.cpp/hpp` (est. 2000 lines)
- `src/avm2/avm2_vm.cpp/hpp` (est. 1500 lines)
- `src/avm2/avm2_types.cpp/hpp` (est. 1200 lines)
- `include/abc/abc_opcodes.hpp` (est. 500 lines)
- **Total:** ~5200 lines of C++

**Tests:**
- "Hello World" AS3 SWF
- Arithmetic test suite
- Control flow tests
- 20+ simple AS3 tests

#### Exit Criteria

- [ ] Can compile simple AS3 scripts
- [ ] Generated C++ code compiles
- [ ] "Hello World" AS3 works
- [ ] Basic opcodes functional
- [ ] Test suite passing (20/20)

---

### Phase 3: Object Model (3-4 months)

**Goal:** Full OOP support - classes, inheritance, methods, properties
**Language:** C++
**Estimated effort:** 250-400 hours

#### Milestones

**3.1: Object System Foundation (3 weeks)**
- [ ] Implement AS3Object base class
  - Property storage (map)
  - Prototype chain
  - Dynamic property access
  - Sealed vs dynamic objects
- [ ] Implement property attributes
  - ATTR_Enumerable
  - ATTR_DontEnum
  - ATTR_DontDelete
  - ATTR_ReadOnly
- [ ] Implement slot storage (fixed properties)
- [ ] Implement dynamic property storage

**3.2: Class System (4 weeks)**
- [ ] Implement AS3Class structure
  - Class name
  - Superclass reference
  - Interface list
  - Instance traits
  - Class traits
  - Constructor method
- [ ] Generate C++ classes from ABC class_info
- [ ] Implement constructor generation
- [ ] Implement method generation
- [ ] Implement property generation (getters/setters)
- [ ] Handle access modifiers (public, private, protected, internal)

**3.3: Inheritance (3 weeks)**
- [ ] Implement prototype chain traversal
- [ ] Implement super() calls
- [ ] Implement superclass method calls
- [ ] Handle method overriding
- [ ] Implement virtual dispatch
- [ ] Test multi-level inheritance

**3.4: Interfaces (2 weeks)**
- [ ] Implement interface definitions
- [ ] Implement interface type checking
- [ ] Implement "implements" verification
- [ ] Test interface polymorphism

**3.5: Namespaces (2 weeks)**
- [ ] Implement namespace objects
- [ ] Implement namespace-qualified names
- [ ] Implement namespace resolution
- [ ] Implement "use namespace" directive
- [ ] Test package structures

**3.6: Property Access Opcodes (2 weeks)**
- [ ] getproperty
- [ ] setproperty
- [ ] initproperty
- [ ] deleteproperty
- [ ] getsuper
- [ ] setsuper
- [ ] getdescendants (E4X)
- [ ] getslot / setslot
- [ ] getglobalslot / setglobalslot

**3.7: Method Call Opcodes (3 weeks)**
- [ ] call (function call)
- [ ] callmethod (call by method index)
- [ ] callproperty (call method by name)
- [ ] callproplex (call property with lex scope)
- [ ] callpropvoid (call property, ignore return)
- [ ] callstatic (call static method)
- [ ] callsuper (call superclass method)
- [ ] callsupervoid
- [ ] construct (new operator)
- [ ] constructprop (construct via property)
- [ ] constructsuper (super constructor)

**3.8: Scope Chain (2 weeks)**
- [ ] Implement scope stack
- [ ] Implement pushscope / popscope
- [ ] Implement getscopeobject
- [ ] Implement getglobalscope
- [ ] Implement findproperty / findpropstrict
- [ ] Test lexical scoping

**3.9: Testing (2 weeks)**
- [ ] Test class instantiation
- [ ] Test inheritance hierarchies
- [ ] Test method dispatch
- [ ] Test property access
- [ ] Test namespace resolution
- [ ] Test interface implementation
- [ ] 50+ OOP test cases

#### Sample Code

**Input AS3:**
```actionscript
package com.example {
    public class Animal {
        private var _name:String;

        public function Animal(name:String) {
            _name = name;
        }

        public function speak():String {
            return "...";
        }

        public function get name():String {
            return _name;
        }
    }

    public class Dog extends Animal {
        public function Dog(name:String) {
            super(name);
        }

        override public function speak():String {
            return "Woof!";
        }
    }
}
```

**Generated C++:**
```cpp
namespace as3_com_example {

class Animal : public avm2::AS3Object {
private:
    std::shared_ptr<avm2::AS3String> _name;

public:
    Animal(std::shared_ptr<avm2::AS3String> name)
        : _name(name) {}

    virtual std::shared_ptr<avm2::AS3String> speak() {
        return std::make_shared<avm2::AS3String>("...");
    }

    std::shared_ptr<avm2::AS3String> get_name() {
        return _name;
    }
};

class Dog : public Animal {
public:
    Dog(std::shared_ptr<avm2::AS3String> name)
        : Animal(name) {}

    std::shared_ptr<avm2::AS3String> speak() override {
        return std::make_shared<avm2::AS3String>("Woof!");
    }
};

} // namespace as3_com_example
```

#### Deliverables

**Code:**
- `src/avm2/avm2_object.cpp/hpp` (est. 2000 lines)
- `src/avm2/avm2_class.cpp/hpp` (est. 2500 lines)
- `src/avm2/avm2_namespace.cpp/hpp` (est. 800 lines)
- Enhanced `abc_codegen.cpp` (+1500 lines)
- **Total:** ~6800 lines of C++

**Tests:**
- 50+ OOP test cases
- Class hierarchy tests
- Interface tests
- Namespace tests

#### Exit Criteria

- [ ] Can generate C++ classes from AS3
- [ ] Inheritance works correctly
- [ ] Method dispatch correct
- [ ] Property access works
- [ ] Namespaces resolve correctly
- [ ] Test suite passing (50/50)

---

### Phase 4: Advanced Features (4-6 months)

**Goal:** Complete opcode set, exceptions, built-ins, standard library
**Language:** C++
**Estimated effort:** 400-700 hours

#### Milestones

**4.1: Remaining Opcodes (6 weeks)**

Implement all 164 opcodes (114 remaining):

- [ ] **Type operations** (10): coerce_*, convert_*, checkfilter, astype, astypelate, istype, istypelate, instanceof, typeof, in
- [ ] **Bitwise** (7): bitand, bitor, bitxor, bitnot, lshift, rshift, urshift
- [ ] **Array/Object creation** (5): newobject, newarray, newclass, newactivation, newfunction
- [ ] **Iteration** (4): hasnext, hasnext2, nextname, nextvalue
- [ ] **Switch** (1): lookupswitch
- [ ] **Increment/Decrement** (8): increment, increment_i, decrement, decrement_i, inclocal, inclocal_i, declocal, declocal_i
- [ ] **Advanced control** (2): kill (local variable)
- [ ] **XML/E4X** (10+): dxns, dxnslate, esc_xattr, esc_xelem, etc.
- [ ] **Alchemy memory ops** (14): li8, li16, li32, lf32, lf64, si8, si16, si32, sf32, sf64, sxi1, sxi8, sxi16, sign_extend
- [ ] **Misc** (remaining): applytype, pushnamespace, etc.

**4.2: Exception Handling (3 weeks)**
- [ ] Implement try/catch/finally using C++ exceptions
- [ ] Parse exception_info from method_body
- [ ] Generate try/catch blocks
- [ ] Implement throw opcode
- [ ] Implement exception objects (AS3Error, TypeError, etc.)
- [ ] Test exception propagation
- [ ] Test finally blocks

**4.3: Closures and Functions (3 weeks)**
- [ ] Implement Function objects
- [ ] Implement newfunction opcode
- [ ] Implement closure capture
- [ ] Use C++ lambdas for closures
- [ ] Implement apply() and call()
- [ ] Test nested closures

**4.4: Arrays (3 weeks)**
- [ ] Implement AS3Array class
  - Sparse array support
  - Dense array optimization
  - Array methods (push, pop, shift, unshift, etc.)
- [ ] Implement Vector.<T> (typed arrays)
- [ ] Implement array access (getproperty on int indices)
- [ ] Test array performance

**4.5: Standard Library - Core (4 weeks)**
- [ ] **Object** - base class, hasOwnProperty, toString, etc.
- [ ] **Function** - apply, call, bind
- [ ] **Array** - push, pop, slice, splice, map, filter, reduce, etc.
- [ ] **String** - charAt, substring, indexOf, replace, split, etc.
- [ ] **Number** - parseInt, parseFloat, MAX_VALUE, etc.
- [ ] **Math** - sin, cos, sqrt, random, etc.
- [ ] **Boolean** - toString, valueOf
- [ ] **Date** - Date manipulation (complex!)
- [ ] **RegExp** - Regular expressions (very complex!)

**4.6: Standard Library - Advanced (4 weeks)**
- [ ] **Error** classes - Error, TypeError, ReferenceError, etc.
- [ ] **XML** / **XMLList** - E4X support (if needed)
- [ ] **ByteArray** - Binary data manipulation
- [ ] **Dictionary** - Weak-keyed maps
- [ ] **JSON** - JSON.parse, JSON.stringify
- [ ] **flash.utils** - getTimer, setTimeout, setInterval

**4.7: Integration with Graphics (2 weeks)**
- [ ] Link AS3 classes to display objects
- [ ] Implement MovieClip for AS3
- [ ] Implement Sprite for AS3
- [ ] Link SymbolClass tags to AS3 classes
- [ ] Test AS3 code controlling graphics

**4.8: Performance Optimization (3 weeks)**
- [ ] Profile generated code
- [ ] Optimize hot paths
- [ ] Reduce smart pointer overhead where safe
- [ ] Implement inline optimizations
- [ ] Dead code elimination
- [ ] Constant folding

**4.9: Comprehensive Testing (4 weeks)**
- [ ] Port existing AS3 test suites
- [ ] Test against reference implementations
- [ ] Stress testing with large ABC files
- [ ] Memory leak testing (valgrind)
- [ ] Performance benchmarking vs Flash Player
- [ ] Test real AS3 games

#### Deliverables

**Code:**
- `src/avm2/avm2_opcodes.cpp` (+3000 lines for remaining opcodes)
- `src/avm2/avm2_builtins.cpp/hpp` (est. 4000 lines)
- `src/avm2/avm2_array.cpp/hpp` (est. 1200 lines)
- `src/avm2/avm2_exception.cpp/hpp` (est. 800 lines)
- `src/avm2/avm2_closure.cpp/hpp` (est. 600 lines)
- **Total:** ~9600 lines of C++

**Tests:**
- 100+ comprehensive tests
- Standard library test suite
- Exception handling tests
- Real-world AS3 game tests

#### Exit Criteria

- [ ] All 164 opcodes implemented
- [ ] Exception handling works
- [ ] Standard library functional
- [ ] Can run real AS3 games
- [ ] Test suite passing (100/100)
- [ ] No memory leaks
- [ ] Performance acceptable

---

### Phase 5: Optimization & Polish (2-3 months)

**Goal:** Production-ready AS3 support
**Language:** C++
**Estimated effort:** 200-400 hours

#### Milestones

**5.1: Performance Optimization (4 weeks)**
- [ ] Profile with real AS3 games
- [ ] Identify bottlenecks
- [ ] Optimize type checking
- [ ] Optimize property access
- [ ] Optimize method dispatch
- [ ] Consider JIT compilation (future)
- [ ] Target: Within 2x of Flash Player performance

**5.2: Memory Optimization (2 weeks)**
- [ ] Reduce smart pointer overhead
- [ ] Implement object pooling where beneficial
- [ ] Optimize string storage
- [ ] Reduce memory allocations
- [ ] Target: < 2x Flash Player memory usage

**5.3: Code Generation Optimization (3 weeks)**
- [ ] Inline small methods
- [ ] Constant propagation
- [ ] Dead code elimination
- [ ] Loop unrolling (where safe)
- [ ] Reduce generated code size

**5.4: Cross-Platform Testing (3 weeks)**
- [ ] Test on Windows (MSVC, MinGW)
- [ ] Test on macOS
- [ ] Test on Linux (various distros)
- [ ] Test WASM builds
- [ ] Fix platform-specific issues

**5.5: Error Handling (2 weeks)**
- [ ] Better error messages for invalid ABC
- [ ] Runtime error reporting
- [ ] Debug symbol generation
- [ ] Stack trace support

**5.6: Documentation (3 weeks)**
- [ ] API documentation (Doxygen)
- [ ] User guide for AS3 recompilation
- [ ] Architecture documentation
- [ ] Code examples
- [ ] Troubleshooting guide

**5.7: Integration Testing (2 weeks)**
- [ ] Test AS1/2 + AS3 hybrid SWFs
- [ ] Test with SWFModernRuntime graphics
- [ ] Test with real games
- [ ] Fix integration issues

**5.8: Release Preparation (1 week)**
- [ ] Version tagging
- [ ] Release notes
- [ ] Binary distribution
- [ ] Example projects
- [ ] Announce to community

#### Deliverables

**Code:**
- Optimized codebase
- Platform-specific fixes
- Documentation

**Documentation:**
- Complete API reference
- User guide
- Architecture documentation
- Examples

**Release:**
- Binary releases for Windows/Linux/macOS
- Example AS3 projects
- Test suite

#### Exit Criteria

- [ ] Performance acceptable (< 2x Flash Player)
- [ ] Memory usage reasonable
- [ ] Works on all target platforms
- [ ] Comprehensive documentation
- [ ] Ready for production use
- [ ] Community feedback positive

---

### Phase Summary

| Phase | Duration | Effort (hours) | Key Deliverable |
|-------|----------|----------------|-----------------|
| **Phase 0** | 2 months | - | AS1/2 stable |
| **Phase 1** | 3-4 months | 300-500 | ABC Parser |
| **Phase 2** | 2-3 months | 200-350 | Basic Codegen |
| **Phase 3** | 3-4 months | 250-400 | Object Model |
| **Phase 4** | 4-6 months | 400-700 | All Features |
| **Phase 5** | 2-3 months | 200-400 | Production Ready |
| **TOTAL** | **16-22 months** | **1350-2350** | **AS3 Support** |

**Note:** Phases can overlap slightly. Phase 0 is prerequisite for Phase 1, but Phases 2-4 have some parallelizable work.

---

## Technical Specifications

### ABC File Format

**Reference:** Adobe AVM2 Overview (http://www.adobe.com/devnet/actionscript/articles/avm2overview.pdf)

#### File Structure

```cpp
struct abcFile {
    u16 minor_version;
    u16 major_version;
    cpool_info constant_pool;
    u30 method_count;
    method_info methods[method_count];
    u30 metadata_count;
    metadata_info metadata[metadata_count];
    u30 class_count;
    instance_info instances[class_count];
    class_info classes[class_count];
    u30 script_count;
    script_info scripts[script_count];
    u30 method_body_count;
    method_body_info method_bodies[method_body_count];
};
```

#### Constant Pool

```cpp
struct cpool_info {
    u30 int_count;
    s32 integers[int_count];

    u30 uint_count;
    u32 uintegers[uint_count];

    u30 double_count;
    d64 doubles[double_count];

    u30 string_count;
    string_info strings[string_count];

    u30 namespace_count;
    namespace_info namespaces[namespace_count];

    u30 ns_set_count;
    ns_set_info ns_sets[ns_set_count];

    u30 multiname_count;
    multiname_info multinames[multiname_count];
};
```

#### Method Info

```cpp
struct method_info {
    u30 param_count;
    u30 return_type;
    u30 param_types[param_count];
    u30 name;
    u8 flags;
    option_info options;  // if HAS_OPTIONAL
    param_info param_names[param_count];  // if HAS_PARAM_NAMES
};

struct method_body_info {
    u30 method;
    u30 max_stack;
    u30 local_count;
    u30 init_scope_depth;
    u30 max_scope_depth;
    u30 code_length;
    u8 code[code_length];
    u30 exception_count;
    exception_info exceptions[exception_count];
    u30 trait_count;
    traits_info traits[trait_count];
};
```

#### Class Info

```cpp
struct instance_info {
    u30 name;
    u30 super_name;
    u8 flags;
    u30 protectedNs;  // if CONSTANT_ClassProtectedNs
    u30 intrf_count;
    u30 interfaces[intrf_count];
    u30 iinit;
    u30 trait_count;
    traits_info traits[trait_count];
};

struct class_info {
    u30 cinit;
    u30 trait_count;
    traits_info traits[trait_count];
};
```

#### Traits

```cpp
struct traits_info {
    u30 name;
    u8 kind;
    union {
        trait_slot slot;       // kind 0,6
        trait_class class;     // kind 4
        trait_function func;   // kind 5
        trait_method method;   // kind 1,2,3
    } data;
    u30 metadata_count;
    u30 metadata[metadata_count];
};
```

### C++ Type System Design

#### Base Value Type

```cpp
namespace avm2 {

enum class ValueType {
    UNDEFINED,
    NULL_TYPE,
    BOOLEAN,
    INT,
    UINT,
    NUMBER,
    STRING,
    OBJECT,
    ARRAY,
    FUNCTION,
    CLASS
};

class AS3Value {
public:
    virtual ~AS3Value() = default;

    virtual ValueType getType() const = 0;

    // Type conversions
    virtual bool toBoolean() = 0;
    virtual int32_t toInt() = 0;
    virtual uint32_t toUInt() = 0;
    virtual double toNumber() = 0;
    virtual std::string toString() = 0;
    virtual std::shared_ptr<AS3Object> toObject() = 0;

    // Operations
    virtual std::shared_ptr<AS3Value> add(std::shared_ptr<AS3Value> other) = 0;
    virtual std::shared_ptr<AS3Value> subtract(std::shared_ptr<AS3Value> other) = 0;
    virtual std::shared_ptr<AS3Value> multiply(std::shared_ptr<AS3Value> other) = 0;
    virtual std::shared_ptr<AS3Value> divide(std::shared_ptr<AS3Value> other) = 0;

    // Comparison
    virtual bool equals(std::shared_ptr<AS3Value> other) = 0;
    virtual bool strictEquals(std::shared_ptr<AS3Value> other) = 0;
    virtual bool lessThan(std::shared_ptr<AS3Value> other) = 0;
};

} // namespace avm2
```

#### Primitive Types

```cpp
class AS3Int : public AS3Value {
    int32_t value;
public:
    AS3Int(int32_t v) : value(v) {}

    ValueType getType() const override { return ValueType::INT; }
    int32_t toInt() override { return value; }
    bool toBoolean() override { return value != 0; }
    double toNumber() override { return static_cast<double>(value); }
    std::string toString() override { return std::to_string(value); }

    std::shared_ptr<AS3Value> add(std::shared_ptr<AS3Value> other) override {
        return std::make_shared<AS3Int>(value + other->toInt());
    }

    // ... other operations
};

class AS3Number : public AS3Value {
    double value;
public:
    AS3Number(double v) : value(v) {}
    // Similar implementation
};

class AS3String : public AS3Value {
    std::string value;
public:
    AS3String(const std::string& v) : value(v) {}

    std::shared_ptr<AS3Value> add(std::shared_ptr<AS3Value> other) override {
        return std::make_shared<AS3String>(value + other->toString());
    }
    // ... other operations
};

class AS3Boolean : public AS3Value {
    bool value;
public:
    AS3Boolean(bool v) : value(v) {}
    // ... implementation
};

class AS3Undefined : public AS3Value {
    // Singleton
    ValueType getType() const override { return ValueType::UNDEFINED; }
    // ... implementation
};

class AS3Null : public AS3Value {
    // Singleton
    ValueType getType() const override { return ValueType::NULL_TYPE; }
    // ... implementation
};
```

#### Object Type

```cpp
class AS3Object : public AS3Value {
protected:
    // Dynamic properties
    std::unordered_map<std::string, std::shared_ptr<AS3Value>> dynamicProperties;

    // Slots (fixed properties)
    std::vector<std::shared_ptr<AS3Value>> slots;

    // Prototype chain
    std::shared_ptr<AS3Object> prototype;

    // Class information
    AS3Class* classInfo;

public:
    AS3Object() = default;

    ValueType getType() const override { return ValueType::OBJECT; }

    // Property access
    virtual std::shared_ptr<AS3Value> getProperty(const std::string& name);
    virtual void setProperty(const std::string& name, std::shared_ptr<AS3Value> value);
    virtual bool hasProperty(const std::string& name);
    virtual bool deleteProperty(const std::string& name);

    // Slot access (faster)
    std::shared_ptr<AS3Value> getSlot(uint32_t index);
    void setSlot(uint32_t index, std::shared_ptr<AS3Value> value);

    // Prototype chain
    std::shared_ptr<AS3Value> getPropertyFromPrototype(const std::string& name);
};
```

#### Array Type

```cpp
class AS3Array : public AS3Object {
private:
    // Sparse array storage
    std::unordered_map<uint32_t, std::shared_ptr<AS3Value>> elements;

    // Dense array optimization (if no gaps)
    std::vector<std::shared_ptr<AS3Value>> denseArray;
    bool isDense;

    uint32_t _length;

public:
    AS3Array();

    ValueType getType() const override { return ValueType::ARRAY; }

    uint32_t length() const { return _length; }
    void setLength(uint32_t len);

    std::shared_ptr<AS3Value> getElement(uint32_t index);
    void setElement(uint32_t index, std::shared_ptr<AS3Value> value);

    // Array methods
    void push(std::shared_ptr<AS3Value> value);
    std::shared_ptr<AS3Value> pop();
    std::shared_ptr<AS3Value> shift();
    void unshift(std::shared_ptr<AS3Value> value);
    std::shared_ptr<AS3Array> slice(int start, int end);
    void splice(int start, int deleteCount, const std::vector<std::shared_ptr<AS3Value>>& items);
    // ... more methods
};
```

#### Function Type

```cpp
class AS3Function : public AS3Object {
public:
    using NativeFunc = std::function<std::shared_ptr<AS3Value>(
        AVM2Context*,
        const std::vector<std::shared_ptr<AS3Value>>&
    )>;

private:
    NativeFunc func;

    // Closure capture
    std::vector<std::shared_ptr<AS3Value>> capturedVariables;

public:
    AS3Function(NativeFunc f) : func(f) {}

    ValueType getType() const override { return ValueType::FUNCTION; }

    std::shared_ptr<AS3Value> call(
        AVM2Context* ctx,
        std::shared_ptr<AS3Object> thisObj,
        const std::vector<std::shared_ptr<AS3Value>>& args
    );

    void captureVariable(std::shared_ptr<AS3Value> var) {
        capturedVariables.push_back(var);
    }
};
```

### AVM2 Context

```cpp
class AVM2Context {
private:
    // Operand stack
    std::vector<std::shared_ptr<AS3Value>> stack;

    // Scope stack
    std::vector<std::shared_ptr<AS3Object>> scopeStack;

    // Local variables
    std::vector<std::shared_ptr<AS3Value>> locals;

    // Global object
    std::shared_ptr<AS3Object> global;

    // Current exception frame
    std::vector<ExceptionFrame> exceptionFrames;

public:
    // Stack operations
    void push(std::shared_ptr<AS3Value> value);
    std::shared_ptr<AS3Value> pop();
    std::shared_ptr<AS3Value> peek(int offset = 0);

    // Scope operations
    void pushScope(std::shared_ptr<AS3Object> scope);
    std::shared_ptr<AS3Object> popScope();
    std::shared_ptr<AS3Object> getScopeObject(uint32_t index);
    std::shared_ptr<AS3Object> getGlobalScope();

    // Local variables
    std::shared_ptr<AS3Value> getLocal(uint32_t index);
    void setLocal(uint32_t index, std::shared_ptr<AS3Value> value);

    // Property operations
    std::shared_ptr<AS3Object> findProperty(const std::string& name);
    std::shared_ptr<AS3Object> findPropertyStrict(const std::string& name);
    std::shared_ptr<AS3Value> getGlobalProperty(const std::string& name);

    // Method calls
    void callFunction(std::shared_ptr<AS3Function> func, uint32_t argc);
    void callProperty(const std::string& name, uint32_t argc);
    void callPropertyVoid(const std::string& name, uint32_t argc);
    void callSuper(const std::string& name, uint32_t argc);

    // Object creation
    std::shared_ptr<AS3Object> constructObject(std::shared_ptr<AS3Class> cls, uint32_t argc);

    // Exception handling
    void throwException(std::shared_ptr<AS3Value> exception);
    void pushExceptionFrame(uint32_t catchTarget);
    void popExceptionFrame();
};
```

### Class System

```cpp
struct AS3Trait {
    enum Kind {
        SLOT, METHOD, GETTER, SETTER, CLASS, FUNCTION
    };

    Kind kind;
    std::string name;
    uint32_t slotId;
    std::shared_ptr<AS3Value> value;
    std::shared_ptr<AS3Function> getter;
    std::shared_ptr<AS3Function> setter;
};

class AS3Class {
public:
    std::string name;
    std::shared_ptr<AS3Class> superclass;
    std::vector<std::shared_ptr<AS3Class>> interfaces;

    std::vector<AS3Trait> instanceTraits;
    std::vector<AS3Trait> classTraits;

    std::shared_ptr<AS3Function> constructor;
    std::shared_ptr<AS3Function> staticConstructor;

    bool isSealed;
    bool isFinal;
    bool isInterface;

    // Create new instance
    std::shared_ptr<AS3Object> createInstance(AVM2Context* ctx, const std::vector<std::shared_ptr<AS3Value>>& args);

    // Check if class implements interface
    bool implements(std::shared_ptr<AS3Class> interface);

    // Type checking
    bool isInstance(std::shared_ptr<AS3Object> obj);
};
```

### Generated Code Example

**Input AS3:**
```actionscript
package {
    public class Main {
        public static function main():void {
            var x:int = 42;
            var y:int = x + 10;
            trace("Result: " + y);
        }
    }
}
```

**Generated C++:**
```cpp
#include <avm2_runtime.hpp>

namespace as3_generated {

class Main : public avm2::AS3Object {
public:
    static void main(avm2::AVM2Context* ctx) {
        // var x:int = 42;
        auto local_1 = std::make_shared<avm2::AS3Int>(42);
        ctx->setLocal(1, local_1);

        // var y:int = x + 10;
        auto temp1 = ctx->getLocal(1);
        auto temp2 = std::make_shared<avm2::AS3Int>(10);
        auto local_2 = temp1->add(temp2);
        ctx->setLocal(2, local_2);

        // trace("Result: " + y);
        auto trace_func = ctx->getGlobalProperty("trace");
        auto str1 = std::make_shared<avm2::AS3String>("Result: ");
        auto temp3 = ctx->getLocal(2);
        auto str2 = str1->add(temp3);
        ctx->push(str2);
        ctx->callFunction(std::static_pointer_cast<avm2::AS3Function>(trace_func), 1);
    }
};

} // namespace as3_generated

// Script initialization
void as3_script_init(avm2::AVM2Context* ctx) {
    // Register Main class
    auto mainClass = std::make_shared<avm2::AS3Class>();
    mainClass->name = "Main";
    mainClass->staticConstructor = std::make_shared<avm2::AS3Function>(
        [](avm2::AVM2Context* ctx, const std::vector<std::shared_ptr<avm2::AS3Value>>& args) {
            as3_generated::Main::main(ctx);
            return avm2::AS3Undefined::instance();
        }
    );

    ctx->registerClass("Main", mainClass);

    // Call Main.main()
    as3_generated::Main::main(ctx);
}
```

---

## Effort Estimates

### With C++ Implementation

| Component | Lines of Code | Hours | Complexity |
|-----------|---------------|-------|------------|
| **ABC Parser** | 4,500 | 300-500 | High |
| **Code Generator** | 2,000 | 200-350 | Medium-High |
| **Type System** | 1,200 | 100-150 | Medium |
| **Object Model** | 2,000 | 150-250 | High |
| **Class System** | 2,500 | 200-300 | Very High |
| **Namespace System** | 800 | 80-120 | Medium |
| **164 Opcodes** | 5,000 | 400-600 | Very High |
| **Built-in Library** | 4,000 | 300-500 | High |
| **Array/Collections** | 1,200 | 100-150 | Medium |
| **Exception Handling** | 800 | 100-150 | Medium-High |
| **Closures/Functions** | 600 | 80-120 | Medium |
| **Testing** | - | 200-300 | - |
| **Documentation** | - | 100-150 | - |
| **Optimization** | - | 150-250 | - |
| **TOTAL** | **~24,600** | **2,460-3,490** | - |

### Comparison: C vs C++

| Metric | C Implementation | C++ Implementation | Savings |
|--------|------------------|-------------------|---------|
| Total LOC | ~45,000 | ~25,000 | 44% |
| Total Hours | 3,500-5,000 | 2,500-3,500 | 30% |
| Duration | 18-24 months | 12-18 months | 33% |
| Complexity | Very High | High | Significant |
| Maintainability | Difficult | Moderate | Much better |

### Developer Velocity Assumptions

**Based on:**
- LittleCube's current pace: 2-5 commits/day
- SWFRecomp current state: ~80% AS1/2 complete
- Single experienced developer
- Part-time work (20-30 hours/week)

**Adjusted estimates:**
- **Full-time (40h/week):** 12-16 months
- **Part-time (20h/week):** 24-32 months
- **Team of 2-3:** 8-12 months

---

## Risk Assessment

### High-Risk Items

#### 1. Scope Creep ⚠️ HIGH
**Risk:** AS3 is very large, easy to get sidetracked

**Mitigation:**
- Strict phase boundaries
- Regular reassessment
- Focus on minimum viable implementation first
- Defer non-essential features

**Impact if occurs:** +6-12 months to schedule

#### 2. Integration Issues ⚠️ MEDIUM
**Risk:** C/C++ runtime integration may have issues

**Mitigation:**
- Early integration testing
- Clear API boundaries
- Use extern "C" where needed
- Test mixed AS1/2 + AS3 early

**Impact if occurs:** +1-3 months

#### 3. Performance Problems ⚠️ MEDIUM
**Risk:** Generated C++ code may be slower than expected

**Mitigation:**
- Early performance testing
- Profile and optimize incrementally
- Consider JIT compilation if needed
- Compare against Ruffle benchmarks

**Impact if occurs:** +2-4 months for optimization

#### 4. Memory Management Bugs ⚠️ MEDIUM
**Risk:** Smart pointers may have circular references or leaks

**Mitigation:**
- Use weak_ptr for cycles
- Extensive valgrind testing
- Memory profiling
- ASan/MSan builds

**Impact if occurs:** +1-2 months for debugging

#### 5. Incomplete AS3 Specification ⚠️ LOW-MEDIUM
**Risk:** Some AS3 behaviors undocumented

**Mitigation:**
- Reverse engineer from Ruffle/Tamarin
- Extensive testing with real SWFs
- Community feedback
- Document assumptions

**Impact if occurs:** +1-3 months research

#### 6. Single Developer Dependency ⚠️ HIGH
**Risk:** Only LittleCube working on project

**Mitigation:**
- Comprehensive documentation
- Seek additional contributors
- Modular design for easier onboarding
- Consider sponsorship/funding

**Impact if occurs:** Project stall if developer unavailable

### Medium-Risk Items

#### 7. Build System Complexity 📊 MEDIUM
**Risk:** C and C++ mix may cause build issues

**Mitigation:**
- Test on all platforms early
- Clear CMake organization
- CI/CD for automated testing

#### 8. Standard Library Compatibility 📊 MEDIUM
**Risk:** AS3 standard library is large

**Mitigation:**
- Implement most commonly used first
- Stub out less common features
- Prioritize based on target games

#### 9. Exception Handling Edge Cases 📊 LOW-MEDIUM
**Risk:** C++ exceptions may not perfectly match AS3

**Mitigation:**
- Extensive exception testing
- Document differences
- Wrap exceptions carefully

### Low-Risk Items

#### 10. Platform Support ✅ LOW
**Risk:** Cross-platform issues

**Mitigation:**
- C++17 is standardized
- All dependencies cross-platform
- Test early on all platforms

#### 11. Tool Chain ✅ LOW
**Risk:** Compiler/linker issues

**Mitigation:**
- C++ compilers mature
- CMake handles most issues
- Clear build documentation

---

## Success Criteria

### Phase 1 Success Criteria
- [ ] Can parse any valid ABC file without errors
- [ ] All constant pools extracted correctly
- [ ] Class hierarchies constructed
- [ ] Test suite: 50/50 passing
- [ ] Zero memory leaks
- [ ] Performance: < 100ms for typical ABC

### Phase 2 Success Criteria
- [ ] "Hello World" AS3 SWF compiles and runs
- [ ] Basic opcodes functional
- [ ] Simple arithmetic works
- [ ] Control flow correct
- [ ] Test suite: 20/20 passing

### Phase 3 Success Criteria
- [ ] Can create AS3 class instances
- [ ] Inheritance works correctly
- [ ] Method dispatch correct
- [ ] Property access works
- [ ] Namespaces resolve
- [ ] Test suite: 50/50 passing

### Phase 4 Success Criteria
- [ ] All 164 opcodes implemented
- [ ] Exception handling works
- [ ] Standard library functional
- [ ] Can run real AS3 games
- [ ] Test suite: 100/100 passing
- [ ] No memory leaks
- [ ] Performance within 2x Flash Player

### Phase 5 Success Criteria
- [ ] Production-ready code quality
- [ ] Works on all target platforms
- [ ] Comprehensive documentation
- [ ] Real games playable
- [ ] Community adoption
- [ ] Active maintenance plan

### Overall Success Criteria
- [ ] Can recompile major AS3 games
- [ ] Performance comparable to Ruffle
- [ ] Memory usage reasonable
- [ ] Stable across platforms
- [ ] Well documented
- [ ] Community support
- [ ] Integration with Archipelago possible

---

## References

### Official Specifications

1. **Adobe AVM2 Overview**
   - URL: http://www.adobe.com/devnet/actionscript/articles/avm2overview.pdf
   - Content: Official AVM2 specification, opcode definitions (page 35+)

2. **SWF File Format Specification v19**
   - URL: https://open-flash.github.io/mirrors/swf-spec-19.pdf
   - Content: DoABC tag structure, SymbolClass tag

3. **ActionScript 3.0 Language Reference**
   - URL: https://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/
   - Content: Built-in class documentation

### Open Source Projects

4. **Tamarin (Adobe's AVM2)**
   - GitHub: https://github.com/adobe/avmplus
   - License: MPL/GPL/LGPL tri-license
   - Content: Reference AVM2 implementation, ABC parser

5. **Ruffle (Flash Player in Rust)**
   - GitHub: https://github.com/ruffle-rs/ruffle
   - License: MIT/Apache 2.0
   - Content: Modern Flash implementation, AVM2 in Rust

6. **RABCDAsm (ABC Assembler/Disassembler)**
   - GitHub: https://github.com/CyberShadow/RABCDAsm
   - License: GPL v3
   - Content: ABC file parsing, disassembly

7. **as3-commons-bytecode**
   - GitHub: https://github.com/teotigraphix/as3-commons
   - License: Apache 2.0
   - Content: 164 opcode definitions

8. **swiffas (Python SWF/ABC parser)**
   - GitHub: https://github.com/ahixon/swiffas
   - License: Unknown
   - Content: Reference parser implementation

### Related Documentation

9. **SWFRecomp Project Status**
   - File: PROJECT_STATUS.md
   - Content: Current AS1/2 implementation details

10. **SWFRecomp README**
    - File: README.md
    - Content: Project overview, build instructions

11. **N64Recomp (Inspiration)**
    - GitHub: https://github.com/N64Recomp/N64Recomp
    - Content: Static recompilation approach

### Community Resources

12. **Flash Preservation Discord**
    - Content: Active community, AS3 discussions

13. **Archipelago Discord**
    - Content: Game randomizer community, integration goals

14. **JPEXS Free Flash Decompiler**
    - URL: https://www.free-decompiler.com/flash/
    - Content: AS3 decompilation, SWF analysis

### Academic Papers

15. **"Reflash: practical ActionScript3 instrumentation with RABCDAsm"**
    - URL: https://blog-assets.f-secure.com/wp-content/uploads/2019/10/15163425/reflash-practical-actionscript3-instrumentation.pdf
    - Content: ABC bytecode manipulation

### Code Examples

16. **DoABC Tag Implementation (AS3)**
    - GitHub: https://github.com/vpmedia/swf-read-write-as3/blob/master/zeroswf/src/main/actionscript/zero/swf/tagBodys/DoABC.as
    - Content: Tag parsing example

---

## Appendix A: Code Organization

### Recommended File Structure

```
SWFRecomp/
├── src/
│   ├── abc/
│   │   ├── abc_parser.cpp       # Main ABC file parser
│   │   ├── abc_method.cpp       # Method parsing
│   │   ├── abc_class.cpp        # Class/instance parsing
│   │   ├── abc_script.cpp       # Script parsing
│   │   ├── abc_codegen.cpp      # C++ code generation
│   │   └── abc_util.cpp         # Utility functions
│   └── ...
│
├── include/
│   ├── abc/
│   │   ├── abc_types.hpp        # ABC data structures
│   │   ├── abc_parser.hpp
│   │   ├── abc_method.hpp
│   │   ├── abc_class.hpp
│   │   ├── abc_script.hpp
│   │   ├── abc_codegen.hpp
│   │   ├── abc_opcodes.hpp      # Opcode enum (164 opcodes)
│   │   └── abc_context.hpp      # Code generation context
│   └── ...
│
└── tests/
    ├── abc/
    │   ├── simple_abc/          # Basic ABC tests
    │   ├── class_abc/           # Class tests
    │   ├── inheritance_abc/     # Inheritance tests
    │   └── ...
    └── as3/
        ├── hello_world/         # Hello World AS3
        ├── simple_class/        # Simple class test
        └── ...

SWFModernRuntime/
├── src/
│   ├── avm2/
│   │   ├── avm2_vm.cpp          # AVM2 virtual machine
│   │   ├── avm2_types.cpp       # Type system implementation
│   │   ├── avm2_object.cpp      # Object model
│   │   ├── avm2_class.cpp       # Class system
│   │   ├── avm2_namespace.cpp   # Namespace resolution
│   │   ├── avm2_opcodes.cpp     # Opcode implementations (164)
│   │   ├── avm2_builtins.cpp    # Built-in classes
│   │   ├── avm2_array.cpp       # Array implementation
│   │   ├── avm2_string.cpp      # String implementation
│   │   ├── avm2_function.cpp    # Function/closure
│   │   ├── avm2_exception.cpp   # Exception handling
│   │   └── avm2_memory.cpp      # Smart pointer wrappers
│   └── ...
│
└── include/
    ├── avm2/
    │   ├── avm2_runtime.hpp     # Main runtime header
    │   ├── avm2_types.hpp       # Type definitions
    │   ├── avm2_object.hpp
    │   ├── avm2_class.hpp
    │   ├── avm2_namespace.hpp
    │   ├── avm2_opcodes.hpp
    │   ├── avm2_builtins.hpp
    │   └── ...
    └── ...
```

---

## Appendix B: Development Best Practices

### Code Style

**C++ Standard:** C++17
**Naming Conventions:**
- Classes: PascalCase (AS3Object, AVM2Context)
- Methods: camelCase (getProperty, pushScope)
- Variables: camelCase (localVar, stackPointer)
- Constants: UPPER_SNAKE_CASE (MAX_STACK_SIZE)
- Namespaces: lowercase (avm2, abc)

**File Organization:**
- One class per file (where practical)
- Header guards: `#pragma once`
- Include order: Standard library, third-party, project headers

### Testing Strategy

**Unit Tests:**
- Test each ABC parser component independently
- Test each opcode implementation
- Test type conversions
- Test object model operations

**Integration Tests:**
- Test generated C++ code compiles
- Test runtime execution
- Test with real ABC files

**Performance Tests:**
- Benchmark critical paths
- Compare against Flash Player/Ruffle
- Memory profiling

### Version Control

**Branch Strategy:**
- `main` - Stable AS1/2 support
- `as3-dev` - AS3 development
- `as3-phase-1` - ABC parser
- `as3-phase-2` - Codegen
- etc.

**Commit Messages:**
- Format: `[ABC] Add constant pool parsing`
- Prefix: [ABC], [AVM2], [CODEGEN], [TEST], [DOC]

### Continuous Integration

**Build Matrix:**
- Linux GCC 9, 10, 11
- Linux Clang 10, 11, 12
- Windows MSVC 2019, 2022
- macOS Xcode 12, 13

**Checks:**
- Compilation (C and C++)
- Unit tests
- Integration tests
- Memory leak detection (valgrind)
- Static analysis (clang-tidy)
- Format checking (clang-format)

---

## Appendix C: Frequently Asked Questions

### Q1: Can AS1/2 and AS3 coexist in the same SWF?

**A:** Technically yes, but extremely rare. Flash Player supports this via two separate VMs running in parallel. SWFRecomp will handle this by generating both C and C++ code and linking both runtimes.

### Q2: What about AS3 games that use Flash-specific APIs?

**A:** Many Flash APIs (flash.display.*, flash.events.*, etc.) will need to be implemented or stubbed. This is part of Phase 4 (Standard Library). Focus will be on APIs used by target games (Epic Battle Fantasy 5, etc.).

### Q3: Will this support AIR applications?

**A:** No. AIR (Adobe Integrated Runtime) has desktop-specific APIs that are out of scope. Focus is on browser-based Flash content.

### Q4: How will this compare to Ruffle performance?

**A:** Target is within 2x of Ruffle performance. Static recompilation should theoretically be faster than Ruffle's JIT, but implementation quality matters more. Will benchmark regularly.

### Q5: Can I use this with existing Flash content?

**A:** Yes, once complete. The workflow is: SWF → SWFRecomp → C++/C code → Native executable. Works for any AS3 SWF.

### Q6: What about ActionScript 2.5?

**A:** AS 2.5 is AS2 with some AS3-like syntax. It still uses AVM1, so current AS1/2 support should handle it.

### Q7: Will this support Alchemy (Flash C++ compiler)?

**A:** Alchemy uses special opcodes (li8, si16, etc.) which are included in the 164 opcodes. Support planned in Phase 4.

### Q8: Can I contribute?

**A:** Yes! Once Phase 1 is underway, contributions welcome. Focus areas:
- Testing with real AS3 SWFs
- Built-in class implementations
- Documentation
- Platform-specific testing

### Q9: What license will the AS3 code use?

**A:** Same as SWFRecomp/SWFModernRuntime (check LICENSE file). All code original, clean-room implementation based on public specifications.

### Q10: When will this be done?

**A:** Conservative estimate: 16-22 months after Phase 0 completes. Depends heavily on:
- Developer availability
- Community contributions
- Scope management
- Technical challenges encountered

---

**END OF DOCUMENT**
