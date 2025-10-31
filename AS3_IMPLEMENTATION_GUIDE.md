# ActionScript 3 (AS3) Implementation Guide for SWFRecomp

**Document Version:** 2.0
**Date:** October 29, 2025
**Status:** Planning Phase
**Approach:** Pure C Implementation

---

## Table of Contents

1. [Overview](#overview)
2. [Why Pure C](#why-pure-c)
3. [AS3 Complexity: The Real Challenge](#as3-complexity-the-real-challenge)
4. [Architecture](#architecture)
5. [Implementation Phases](#implementation-phases)
6. [Technical Specifications](#technical-specifications)
7. [Code Examples](#code-examples)
8. [Future: Patching and Modding](#future-patching-and-modding)
9. [References](#references)

---

## Overview

This guide outlines the implementation of ActionScript 3 (AS3) support for SWFRecomp using pure C. AS3 is significantly more complex than AS1/2, featuring:

- **Full class-based OOP** with inheritance, interfaces, and namespaces
- **Type system** with static and dynamic typing
- **Complex type coercion** following ECMA-262 specification
- **256 opcode slots** with ~150 implemented opcodes (vs ~100 for AS1/2)
- **ABC file format** for code and metadata storage
- **AVM2 virtual machine** with advanced features

### Key Insight: Compiler Simplifies the Bytecode

The ActionScript compiler handles many OOP complexities before generating bytecode:
- Lambda closures are managed by the compiler
- Many high-level language features are transformed into simpler bytecode operations
- The bytecode doesn't directly represent all OOP concepts

This means we won't need to implement every complex language feature - just the bytecode operations that result from compilation.

---

## Why Pure C

### Performance Requirements

**Binary Size:** WASM builds must be as small as possible. C produces significantly smaller binaries than C++:
- No virtual table overhead
- No RTTI (Run-Time Type Information)
- No template instantiation bloat
- No C++ standard library overhead

**Execution Speed:** Critical for interactive Flash content:
- Direct function calls (no vtable indirection)
- Predictable performance characteristics
- Simple reference counting (no atomic operations)

### The Complexity is in the Specification, Not the Language

The hard parts of AS3 implementation are:
- Understanding ECMA-262 type conversion algorithms
- Implementing ~150 opcodes with correct semantics
- Handling namespace resolution and multinames
- Building the object model according to AVM2 spec

**C++ doesn't make these easier.** We still need to manually implement `toNumber`, `toString`, `ToPrimitive`, and all the complex type coercion logic regardless of language.

### Consistency with Existing Codebase

SWFModernRuntime is written in C17. Keeping everything in C:
- Simplifies the toolchain
- Avoids C/C++ linking issues
- Maintains consistent coding style
- Makes maintenance easier

---

## AS3 Complexity: The Real Challenge

### Case Study: The `add` Opcode

The `add` instruction (0xA0) demonstrates why AS3 is complex:

**AVM2 Specification:**
```
1. If value1 and value2 are both Numbers, add them using IEEE 754 rules

2. If value1 or value2 is a String or Date, convert both to String and concatenate

3. If value1 and value2 are both XML or XMLList, create new XMLList and append both

4. Otherwise:
   - Convert both to primitives using ToPrimitive (no hint)
   - If either primitive is String, convert both to String and concatenate
   - Otherwise, convert both to Number and add
```

**Implementation (same in C and C++):**
```c
AS3Value* add(AS3Value* v1, AS3Value* v2) {
    // 1. Both numbers → numeric addition
    if (isNumber(v1) && isNumber(v2)) {
        return createNumber(toNumber(v1) + toNumber(v2));
    }

    // 2. String or Date → string concatenation
    if (isString(v1) || isString(v2) || isDate(v1) || isDate(v2)) {
        char* s1 = toString(v1);  // Must implement manually
        char* s2 = toString(v2);  // Must implement manually
        char* result = concat(s1, s2);
        free(s1); free(s2);
        return createString(result);
    }

    // 3. XML + XML → XMLList
    if (isXML(v1) && isXML(v2)) {
        // Complex XMLList construction
        return createXMLList(v1, v2);
    }

    // 4. ToPrimitive conversion
    AS3Value* p1 = toPrimitive(v1, HINT_NONE);  // Must implement manually
    AS3Value* p2 = toPrimitive(v2, HINT_NONE);  // Must implement manually

    if (isString(p1) || isString(p2)) {
        // String concatenation path
    } else {
        // Numeric addition path
    }

    release(p1);
    release(p2);
}
```

**The complexity is in understanding and correctly implementing the ECMA-262 algorithms, not in the syntax.**

### ECMA-262 Algorithms Required

These must be implemented manually, regardless of language:

#### ToPrimitive (Section 9.1)
Converts objects to primitive values by calling `valueOf()` and `toString()` in a specific order based on a hint.

#### ToNumber (Section 9.3)
Converts any value to a number:
- String parsing with complex rules (hex, infinity, whitespace handling)
- Object conversion via ToPrimitive
- Special cases for undefined, null, boolean

#### ToString (Section 9.8)
Converts any value to string:
- Number formatting (exponential notation for large values)
- Special handling for NaN, Infinity, -0
- Object conversion via ToPrimitive

#### ToBoolean, ToInt32, ToUint32
Additional conversion algorithms with specific rules.

### Example: `equals` Opcode (Abstract Equality)

ECMA-262 Section 11.9.3 defines 10 different comparison rules:

```c
int abstractEquals(AS3Value* x, AS3Value* y) {
    // 1. Same type → strict equality
    if (x->type == y->type) return strictEquals(x, y);

    // 2. null == undefined
    if ((x->type == TYPE_NULL && y->type == TYPE_UNDEFINED) ||
        (x->type == TYPE_UNDEFINED && y->type == TYPE_NULL)) return 1;

    // 3. Number == String → convert String to Number
    if (isNumber(x) && isString(y)) {
        return toNumber(x) == toNumber(y);
    }

    // 4. String == Number → convert String to Number
    if (isString(x) && isNumber(y)) {
        return toNumber(x) == toNumber(y);
    }

    // 5-6. Boolean → convert to Number
    if (isBoolean(x)) {
        return abstractEquals(createNumber(toNumber(x)), y);
    }
    if (isBoolean(y)) {
        return abstractEquals(x, createNumber(toNumber(y)));
    }

    // 7-8. Object comparison → convert to primitive
    if (isObject(y)) {
        return abstractEquals(x, toPrimitive(y, HINT_NONE));
    }
    if (isObject(x)) {
        return abstractEquals(toPrimitive(x, HINT_NONE), y);
    }

    // 9. Otherwise not equal
    return 0;
}
```

**Again, C++ provides no help with this complexity.**

---

## Architecture

### Core Philosophy

**Keep it simple, keep it C.**

- Use same architecture as existing AS1/2 implementation
- Tagged union for values
- Manual memory management with clear ownership
- Function pointers for method dispatch
- Explicit type checking

### Value Representation

```c
typedef enum {
    TYPE_UNDEFINED,
    TYPE_NULL,
    TYPE_BOOLEAN,
    TYPE_INT,        // 32-bit signed
    TYPE_UINT,       // 32-bit unsigned
    TYPE_NUMBER,     // 64-bit float (double)
    TYPE_STRING,
    TYPE_OBJECT,
    TYPE_ARRAY,
    TYPE_FUNCTION,
    TYPE_CLASS,
    TYPE_NAMESPACE,
    TYPE_XML,
    TYPE_XMLLIST,
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
        struct AS3Class* cls;
        struct AS3Namespace* ns;
        void* ptr;
    } value;

    uint32_t refcount;  // Simple reference counting
} AS3Value;
```

### Object Model

```c
typedef struct AS3Object {
    AS3Class* klass;           // Object's class
    AS3Object* prototype;      // Prototype chain
    HashMap* properties;       // Dynamic properties
    AS3Value** slots;          // Fixed slots for sealed classes
    uint32_t slot_count;
    uint32_t refcount;
} AS3Object;

typedef struct AS3Class {
    const char* name;
    AS3Class* super_class;
    AS3Namespace* ns;
    Trait* traits;             // Properties, methods, getters, setters
    uint32_t trait_count;
    AS3Function* constructor;
    uint32_t slot_count;
    uint8_t is_sealed;         // Cannot add dynamic properties
    uint8_t is_final;          // Cannot be subclassed
} AS3Class;

typedef struct Trait {
    const char* name;
    TraitKind kind;            // Slot, Method, Getter, Setter, Class, Const
    AS3Namespace* ns;

    union {
        struct {
            uint32_t slot_id;
            AS3Value* value;   // For constants
        } slot;

        struct {
            AS3Function* func;
        } method;

        struct {
            AS3Class* cls;
        } klass;
    } data;
} Trait;
```

### Function Representation

```c
typedef struct AS3Function {
    const char* name;
    FunctionBody* body;        // Bytecode or native function
    uint32_t param_count;
    AS3Value** default_values;
    uint8_t has_rest;          // Has ...rest parameter
    AS3Value** closure_vars;   // Captured variables (compiler-managed)
    uint32_t closure_count;
    uint32_t refcount;
} AS3Function;

typedef struct FunctionBody {
    uint8_t* code;             // AVM2 bytecode
    uint32_t code_length;
    uint32_t max_stack;
    uint32_t max_regs;         // Number of local registers
    uint32_t scope_depth;      // Initial scope depth
    ExceptionHandler* exceptions;
    uint32_t exception_count;
} FunctionBody;
```

### Memory Management

Simple reference counting (same as AS1/2):

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
            case TYPE_FUNCTION:
                release_function(v->value.func);
                break;
            // ... etc
        }
        free(v);
    }
}
```

**Note:** Reference cycles (A→B→A) require manual breaking or an occasional mark-sweep GC pass. For most Flash content, simple refcounting is sufficient.

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

    AS3Object* global_object;
    HashMap* global_properties;

    ExceptionFrame* exception_frame;  // For try/catch
} AVM2Context;
```

---

## Implementation Phases

### Phase 1: ABC Parser

**Goal:** Parse DoABC tags and extract all ABC structures.

**Components:**

1. **ABC File Format Parser** (`abc_parser.c`)
   - Read minor_version, major_version
   - Parse constant pools:
     - int_pool (32-bit signed integers)
     - uint_pool (32-bit unsigned integers)
     - double_pool (64-bit floats)
     - string_pool (UTF-8 strings)
     - namespace_pool
     - ns_set_pool (namespace sets)
     - multiname_pool
   - Parse method_info array
   - Parse metadata_info array
   - Parse instance_info and class_info arrays
   - Parse script_info array
   - Parse method_body_info array
   - Parse trait structures

2. **Data Structures** (`abc_types.h`)
   - Define all ABC format structures
   - Constant pool management
   - Method, class, and script information

3. **Validation**
   - Verify ABC version compatibility (typically 46.16)
   - Validate constant pool indices
   - Check for malformed structures

**Testing:**
- Parse simple AS3 SWF files
- Extract and verify constant pools
- Print ABC data structures

### Phase 2: Code Generation Framework

**Goal:** Generate C code from ABC bytecode with stub implementations.

**Components:**

1. **C Code Generator** (`abc_codegen.c`)
   - Generate C functions for each AS3 method
   - Generate structures for each AS3 class
   - Generate constant data tables
   - Generate initialization code

2. **Opcode Translation** (`abc_opcodes.c`)
   - Translate each opcode to C function call
   - Handle control flow (jumps, branches)
   - Start with stub implementations (no-ops)

3. **Build System Integration**
   - Detect AS3 vs AS1/2 in SWFRecomp
   - Route DoABC tags to ABC parser
   - Generate appropriate Makefiles

**Testing:**
- Generate C code for simple AS3 methods
- Verify generated code compiles
- Run with stub opcodes

### Phase 3: Type System & Core Opcodes

**Goal:** Implement AS3 value system and basic operations.

**Components:**

1. **Value Implementation** (`avm2_types.c`)
   - `AS3Value` structure and operations
   - Type checking functions (`isNumber`, `isString`, etc.)
   - Type conversion functions:
     - `toNumber` (ECMA-262 §9.3)
     - `toString` (ECMA-262 §9.8)
     - `toPrimitive` (ECMA-262 §9.1)
     - `toBoolean` (ECMA-262 §9.2)
     - `toInt32` (ECMA-262 §9.4)
     - `toUint32` (ECMA-262 §9.5)
   - Value creation/destruction
   - Reference counting

2. **Core Opcodes** (30-40 opcodes)
   - Stack operations: `dup`, `pop`, `swap`, `nop`
   - Push constants: `pushbyte`, `pushshort`, `pushint`, `pushuint`, `pushdouble`, `pushstring`, `pushtrue`, `pushfalse`, `pushnull`, `pushundefined`, `pushnan`
   - Locals: `getlocal`, `setlocal`, `getlocal_0-3`, `setlocal_0-3`
   - Arithmetic: `add`, `subtract`, `multiply`, `divide`, `modulo`, `negate`
   - Integer arithmetic: `add_i`, `subtract_i`, `multiply_i`, `negate_i`
   - Comparison: `equals`, `strictequals`, `lessthan`, `lessequals`, `greaterthan`, `greaterequals`
   - Logical: `not`
   - Control: `jump`, `iftrue`, `iffalse`, `ifeq`, `ifne`, `iflt`, `ifle`, `ifgt`, `ifge`, `returnvalue`, `returnvoid`

3. **Testing**
   - Simple arithmetic tests
   - Type conversion tests
   - Control flow tests

### Phase 4: Object Model

**Goal:** Implement AS3 objects, classes, and inheritance.

**Components:**

1. **Object System** (`avm2_object.c`)
   - `AS3Object` implementation
   - Property access (get/set)
   - Prototype chain traversal
   - Dynamic properties (for non-sealed classes)
   - Slot access (for sealed classes)

2. **Class System** (`avm2_class.c`)
   - `AS3Class` implementation
   - Class instantiation
   - Inheritance
   - Trait resolution
   - Constructor calls

3. **Namespace System** (`avm2_namespace.c`)
   - Namespace types: public, private, protected, internal, custom
   - Multiname resolution:
     - QName (qualified name with single namespace)
     - Multiname (runtime namespace set)
     - RTQName (runtime qualified name)
     - RTQNameL (late-bound runtime qualified name)
     - MultinameL (late-bound multiname)
   - Visibility checking based on caller context

4. **Object Opcodes** (30-40 opcodes)
   - Property access: `getproperty`, `setproperty`, `initproperty`, `deleteproperty`, `getsuper`, `setsuper`
   - Slot access: `getslot`, `setslot`, `getglobalslot`, `setglobalslot`
   - Lexical access: `getlex`, `findproperty`, `findpropstrict`
   - Scope: `pushscope`, `popscope`, `getglobalscope`, `getscopeobject`, `pushwith`
   - Object creation: `newobject`, `newarray`, `newclass`, `newactivation`
   - Method calls: `call`, `callmethod`, `callproperty`, `callproplex`, `callpropvoid`, `callstatic`, `callsuper`, `callsupervoid`, `construct`, `constructprop`, `constructsuper`

5. **Testing**
   - Object creation and property access
   - Class instantiation
   - Inheritance tests
   - Method calls and dispatch

### Phase 5: Built-in Classes

**Goal:** Implement Flash/AS3 built-in classes.

**Components:**

1. **Core Classes** (`avm2_builtins.c`)
   - `Object` - Base class with `toString()`, `valueOf()`, `hasOwnProperty()`, etc.
   - `Array` - Dynamic arrays with push, pop, shift, unshift, slice, splice, etc.
   - `String` - Immutable strings with substring, indexOf, charAt, split, etc.
   - `Number`, `int`, `uint` - Numeric types
   - `Boolean` - Boolean type
   - `Function` - Function type with apply, call
   - `Class` - Class type
   - `Math` - Math functions (sin, cos, sqrt, abs, floor, ceil, etc.)
   - `Date` - Date/time operations
   - `RegExp` - Regular expressions
   - `Error`, `TypeError`, `RangeError`, `ReferenceError` - Error types

2. **Flash Display Classes** (minimal subset for rendering)
   - `DisplayObject` - Base display class
   - `Sprite` - Container with graphics
   - `MovieClip` - Timeline-based animation
   - `Shape` - Simple graphics container
   - `Graphics` - Drawing API (lineTo, curveTo, beginFill, etc.)
   - `Bitmap`, `BitmapData` - Raster graphics
   - `TextField`, `TextFormat` - Text rendering

3. **Flash Event Classes**
   - `Event` - Base event class
   - `EventDispatcher` - Event handling
   - `MouseEvent` - Mouse interaction
   - `KeyboardEvent` - Keyboard input

4. **Testing**
   - Built-in class instantiation
   - Method calls on built-ins
   - Flash display tests

### Phase 6: Advanced Features

**Goal:** Implement remaining opcodes and advanced features.

**Components:**

1. **Remaining Opcodes** (60-80 opcodes)
   - Type conversion: `convert_i`, `convert_u`, `convert_d`, `convert_b`, `convert_s`, `convert_o`
   - Type coercion: `coerce`, `coerce_a`, `coerce_s`, `coerce_i`, `coerce_u`, `coerce_d`, `coerce_b`, `coerce_o`
   - Type checking: `astype`, `astypelate`, `istype`, `istypelate`, `instanceof`, `typeof`, `in`, `checkfilter`
   - Bitwise: `bitand`, `bitor`, `bitxor`, `bitnot`, `lshift`, `rshift`, `urshift`
   - Increment/decrement: `increment`, `increment_i`, `decrement`, `decrement_i`, `inclocal`, `inclocal_i`, `declocal`, `declocal_i`
   - Advanced control: `lookupswitch`, `ifstricteq`, `ifstrictne`
   - Iteration: `hasnext`, `hasnext2`, `nextname`, `nextvalue`
   - XML operations: E4X support (if needed)
   - Debug: `debug`, `debugfile`, `debugline`, `bkpt`

2. **Exception Handling** (`avm2_exceptions.c`)
   - `throw` opcode
   - Try/catch/finally blocks
   - Exception propagation
   - Stack unwinding with setjmp/longjmp

3. **Advanced Object Features**
   - Getters and setters (trait system)
   - Property attributes (enumerable, configurable)
   - Object sealing/freezing
   - Proxy objects (`flash.utils.Proxy`)

4. **Alchemy/Memory Operations** (if needed)
   - Low-level memory access: `li8`, `li16`, `li32`, `lf32`, `lf64`
   - Memory writes: `si8`, `si16`, `si32`, `sf32`, `sf64`
   - Sign extension: `sxi1`, `sxi8`, `sxi16`

5. **Testing**
   - Exception handling tests
   - Advanced opcode tests
   - Complex control flow

### Phase 7: Integration & Optimization

**Goal:** Integrate with SWFModernRuntime and optimize.

**Components:**

1. **Runtime Integration**
   - Connect to rendering backend (Vulkan/Canvas2D/WebGL)
   - Event system integration
   - Asset loading (images, sounds, fonts)
   - Sound support

2. **Optimization**
   - Profile hot paths (type checking, property access)
   - Reduce allocations (object pooling)
   - Optimize generated code
   - Inline frequently-used functions

3. **Testing**
   - Real Flash games/apps
   - Performance benchmarks
   - Memory leak detection (Valgrind, AddressSanitizer)
   - Stress testing

4. **Documentation**
   - API documentation
   - Architecture documentation
   - User guide for recompiling AS3 SWFs

---

## Technical Specifications

### ABC File Format

**Header:**
```c
typedef struct {
    uint16_t minor_version;  // Typically 16
    uint16_t major_version;  // Typically 46
} ABCHeader;
```

**Constant Pool:**
```c
typedef struct {
    // Integers (signed)
    int32_t* int_pool;
    uint32_t int_count;

    // Unsigned integers
    uint32_t* uint_pool;
    uint32_t uint_count;

    // Doubles (IEEE 754)
    double* double_pool;
    uint32_t double_count;

    // Strings (UTF-8)
    char** string_pool;
    uint32_t string_count;

    // Namespaces
    Namespace* namespace_pool;
    uint32_t namespace_count;

    // Namespace sets
    NamespaceSet* ns_set_pool;
    uint32_t ns_set_count;

    // Multinames
    Multiname* multiname_pool;
    uint32_t multiname_count;
} ConstantPool;
```

**Method Info:**
```c
typedef struct {
    uint32_t param_count;
    uint32_t return_type;      // Multiname index
    uint32_t* param_types;     // Multiname indices
    uint32_t name_index;       // String index
    uint8_t flags;             // NEED_ARGUMENTS, NEED_ACTIVATION, NEED_REST, HAS_OPTIONAL, etc.
    uint32_t option_count;
    OptionDetail* options;     // Default parameter values
    uint32_t* param_names;     // String indices
} MethodInfo;
```

**Class Info:**
```c
typedef struct {
    uint32_t name;             // Multiname index
    uint32_t super_name;       // Multiname index
    uint8_t flags;             // SEALED, FINAL, INTERFACE, PROTECTED_NS
    uint32_t protected_ns;     // Namespace index
    uint32_t interface_count;
    uint32_t* interfaces;      // Multiname indices
    uint32_t iinit;            // Instance initializer (method index)
    Trait* traits;
    uint32_t trait_count;
} InstanceInfo;

typedef struct {
    uint32_t cinit;            // Class initializer (method index)
    Trait* traits;
    uint32_t trait_count;
} ClassInfo;
```

**Method Body:**
```c
typedef struct {
    uint32_t method;           // Method index
    uint32_t max_stack;
    uint32_t max_regs;         // Number of local registers
    uint32_t scope_depth;      // Initial scope depth
    uint32_t max_scope_depth;  // Maximum scope depth
    uint32_t code_length;
    uint8_t* code;             // AVM2 bytecode
    uint32_t exception_count;
    ExceptionInfo* exceptions;
    Trait* traits;
    uint32_t trait_count;
} MethodBodyInfo;
```

**Exception Info:**
```c
typedef struct {
    uint32_t from;             // Start PC of try block
    uint32_t to;               // End PC of try block
    uint32_t target;           // PC of catch block
    uint32_t exc_type;         // Multiname index
    uint32_t var_name;         // Multiname index
} ExceptionInfo;
```

**Trait:**
```c
typedef enum {
    TRAIT_SLOT = 0,
    TRAIT_METHOD = 1,
    TRAIT_GETTER = 2,
    TRAIT_SETTER = 3,
    TRAIT_CLASS = 4,
    TRAIT_FUNCTION = 5,
    TRAIT_CONST = 6,
} TraitKind;

typedef struct {
    uint32_t name;             // Multiname index
    TraitKind kind;
    uint8_t attributes;        // FINAL, OVERRIDE, METADATA

    union {
        struct {
            uint32_t slot_id;
            uint32_t type_name;    // Multiname index
            uint32_t vindex;       // Constant pool index
            uint8_t vkind;         // Constant pool kind
        } slot;

        struct {
            uint32_t disp_id;
            uint32_t method;       // Method index
        } method;

        struct {
            uint32_t slot_id;
            uint32_t classi;       // Class index
        } klass;

        struct {
            uint32_t slot_id;
            uint32_t function;     // Method index
        } function;
    } data;

    uint32_t metadata_count;
    uint32_t* metadata;        // Metadata indices
} TraitInfo;
```

### Complete Opcode List (~150 Implemented Opcodes)

#### Stack Manipulation (5)
```
0x29  pop
0x2A  dup
0x2B  swap
0x30  pushscope
0x1D  popscope
```

#### Push Constants (13)
```
0x20  pushnull
0x21  pushundefined
0x24  pushbyte
0x25  pushshort
0x26  pushtrue
0x27  pushfalse
0x28  pushnan
0x2C  pushstring
0x2D  pushint
0x2E  pushuint
0x2F  pushdouble
0x31  pushnamespace
```

#### Locals (14)
```
0x62  getlocal
0x63  setlocal
0xD0  getlocal_0
0xD1  getlocal_1
0xD2  getlocal_2
0xD3  getlocal_3
0xD4  setlocal_0
0xD5  setlocal_1
0xD6  setlocal_2
0xD7  setlocal_3
0x92  inclocal
0x94  declocal
0xC2  inclocal_i
0xC3  declocal_i
```

#### Arithmetic (16)
```
0xA0  add
0xA1  subtract
0xA2  multiply
0xA3  divide
0xA4  modulo
0xA5  lshift
0xA6  rshift
0xA7  urshift
0xA8  bitand
0xA9  bitor
0xAA  bitxor
0x90  negate
0xC0  increment
0xC1  decrement
0xC4  add_i
0xC5  subtract_i
0xC6  multiply_i
0xC7  negate_i
0x91  bitnot
```

#### Comparison (11)
```
0xAB  equals
0xAC  strictequals
0xAD  lessthan
0xAE  lessequals
0xAF  greaterthan
0xB0  greaterequals
0xB1  instanceof
0xB2  istype
0xB3  istypelate
0xB4  in
0x95  typeof
0x96  not
```

#### Type Conversion (19)
```
0x70  convert_s
0x73  convert_i
0x74  convert_u
0x75  convert_d
0x76  convert_b
0x77  convert_o
0x78  checkfilter
0x80  coerce
0x81  coerce_b
0x82  coerce_a
0x83  coerce_i
0x84  coerce_d
0x85  coerce_s
0x86  astype
0x87  astypelate
0x89  coerce_u
0x8A  coerce_o
0xC8  increment_i
0xC9  decrement_i
```

#### Property Access (10)
```
0x66  getproperty
0x61  setproperty
0x68  initproperty
0x6A  deleteproperty
0x04  getsuper
0x05  setsuper
0x60  getlex
0x5E  findproperty
0x5D  findpropstrict
0x59  getdescendants
```

#### Slot Access (4)
```
0x6C  getslot
0x6D  setslot
0x6E  getglobalslot
0x6F  setglobalslot
```

#### Scope Access (5)
```
0x1C  pushwith
0x1D  popscope
0x30  pushscope
0x64  getglobalscope
0x65  getscopeobject
```

#### Object/Array Construction (5)
```
0x55  newobject
0x56  newarray
0x58  newclass
0x5A  newactivation
0x42  construct
```

#### Function/Method Calls (11)
```
0x41  call
0x43  callmethod
0x46  callproperty
0x4C  callproplex
0x4F  callpropvoid
0x44  callstatic
0x45  callsuper
0x4E  callsupervoid
0x49  constructsuper
0x4A  constructprop
0x40  newfunction
```

#### Control Flow (13)
```
0x03  throw
0x10  jump
0x11  iftrue
0x12  iffalse
0x13  ifeq
0x14  ifne
0x15  iflt
0x16  ifle
0x17  ifgt
0x18  ifge
0x19  ifstricteq
0x1A  ifstrictne
0x1B  lookupswitch
```

#### Return (2)
```
0x47  returnvoid
0x48  returnvalue
```

#### Iteration (4)
```
0x1F  hasnext
0x32  hasnext2
0x1E  nextname
0x23  nextvalue
```

#### XML/E4X (4)
```
0x71  esc_xelem
0x72  esc_xattr
0x06  dxns
0x07  dxnslate
```

#### Debug (5)
```
0x01  bkpt
0x02  nop
0xEF  debug
0xF0  debugline
0xF1  debugfile
```

#### Alchemy/Memory (13)
```
0x35  li8      (load int 8-bit)
0x36  li16     (load int 16-bit)
0x37  li32     (load int 32-bit)
0x38  lf32     (load float 32-bit)
0x39  lf64     (load float 64-bit)
0x3A  si8      (store int 8-bit)
0x3B  si16     (store int 16-bit)
0x3C  si32     (store int 32-bit)
0x3D  sf32     (store float 32-bit)
0x3E  sf64     (store float 64-bit)
0x50  sxi1     (sign extend 1-bit)
0x51  sxi8     (sign extend 8-bit)
0x52  sxi16    (sign extend 16-bit)
```

#### Other (3)
```
0x08  kill     (kill local variable)
0x09  label    (label for debugger)
0x53  applytype
```

---

## Code Examples

### Generated Code for Simple AS3

**Input AS3:**
```actionscript
package {
    public class Hello {
        public function Hello() {
        }

        public function sayHello():void {
            trace("Hello from AS3!");
        }
    }
}
```

**Generated C Header (`as3_classes.h`):**
```c
#ifndef AS3_CLASSES_H
#define AS3_CLASSES_H

#include "avm2_runtime.h"

// Forward declarations
extern AS3Class Hello_class;

// Hello class instance structure
typedef struct {
    AS3Object base;  // Inherits from Object
} Hello_instance;

// Constructor
AS3Value* Hello_constructor(AVM2Context* ctx);

// Methods
AS3Value* Hello_sayHello(AVM2Context* ctx, AS3Value* this_obj);

#endif
```

**Generated C Implementation (`as3_Hello.c`):**
```c
#include "as3_classes.h"
#include "avm2_runtime.h"
#include <stdlib.h>

// Hello class definition
AS3Class Hello_class = {
    .name = "Hello",
    .super_class = &Object_class,
    .ns = NULL,  // Default namespace
    .traits = NULL,
    .trait_count = 1,
    .constructor = Hello_constructor,
    .slot_count = 0,
    .is_sealed = 1,
    .is_final = 0,
};

// Constructor implementation
AS3Value* Hello_constructor(AVM2Context* ctx) {
    // Allocate instance
    Hello_instance* inst = malloc(sizeof(Hello_instance));

    // Initialize base object
    initObject(&inst->base, &Hello_class);

    AS3Value* obj = createObject(&inst->base);
    return obj;
}

// sayHello method implementation
AS3Value* Hello_sayHello(AVM2Context* ctx, AS3Value* this_obj) {
    // ABC bytecode for sayHello:
    // 0x00: findpropstrict QName("trace")
    // 0x05: pushstring "Hello from AS3!"
    // 0x0A: callpropvoid QName("trace"), 1
    // 0x0F: returnvoid

    // DESIGN NOTE: The recompiler handles QName resolution at compile-time.
    // String constants are inlined, and since findpropstrict/callpropvoid
    // deal with QNames (known at compile-time), the recompiler generates
    // direct function calls rather than runtime property lookups.

    // Generated code (simplified):
    trace("Hello from AS3!");

    return createUndefined();
}

// Alternative: If runtime lookup is needed (for dynamic properties):
AS3Value* Hello_sayHello_dynamic(AVM2Context* ctx, AS3Value* this_obj) {
    // This approach is only needed when QNames aren't known at compile-time
    AS3Value* trace_scope = findpropstrict(ctx, "trace", NULL);
    AS3Value* str = createString("Hello from AS3!");
    AS3Value* trace_func = getproperty(ctx, trace_scope, "trace", NULL);
    callFunction(ctx, trace_func, 1, &str);

    release(str);
    release(trace_scope);
    release(trace_func);

    return createUndefined();
}
```

### Type Conversion Implementation

**ECMA-262 Section 9.3: ToNumber**
```c
double toNumber(AS3Value* input) {
    switch (input->type) {
        case TYPE_UNDEFINED:
            return NAN;

        case TYPE_NULL:
            return 0.0;

        case TYPE_BOOLEAN:
            return input->value.b ? 1.0 : 0.0;

        case TYPE_INT:
            return (double)input->value.i;

        case TYPE_UINT:
            return (double)input->value.ui;

        case TYPE_NUMBER:
            return input->value.d;

        case TYPE_STRING:
            return stringToNumber(input->value.s);

        case TYPE_OBJECT: {
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

**String to Number Parsing:**
```c
double stringToNumber(const char* str) {
    // Trim leading whitespace
    while (isspace(*str)) str++;

    // Empty string → 0
    if (*str == '\0') return 0.0;

    // Check for Infinity
    if (strcmp(str, "Infinity") == 0 || strcmp(str, "+Infinity") == 0) {
        return INFINITY;
    }
    if (strcmp(str, "-Infinity") == 0) {
        return -INFINITY;
    }

    // Check for hex (0x prefix)
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        long long val = strtoll(str, NULL, 16);
        return (double)val;
    }

    // Parse as decimal/scientific notation
    char* endptr;
    double val = strtod(str, &endptr);

    // Skip trailing whitespace
    while (isspace(*endptr)) endptr++;

    // If didn't consume entire string, it's NaN
    if (*endptr != '\0') {
        return NAN;
    }

    return val;
}
```

**ECMA-262 Section 9.8: ToString**
```c
char* toString(AS3Value* input) {
    switch (input->type) {
        case TYPE_UNDEFINED:
            return strdup("undefined");

        case TYPE_NULL:
            return strdup("null");

        case TYPE_BOOLEAN:
            return strdup(input->value.b ? "true" : "false");

        case TYPE_INT: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", input->value.i);
            return strdup(buf);
        }

        case TYPE_UINT: {
            char buf[32];
            snprintf(buf, sizeof(buf), "%u", input->value.ui);
            return strdup(buf);
        }

        case TYPE_NUMBER:
            return numberToString(input->value.d);

        case TYPE_STRING:
            return strdup(input->value.s);

        case TYPE_OBJECT: {
            AS3Value* prim = toPrimitive(input, HINT_STRING);
            char* result = toString(prim);
            release(prim);
            return result;
        }

        default:
            return strdup("");
    }
}
```

**Number to String Formatting:**
```c
char* numberToString(double d) {
    // ECMA-262 section 9.8.1: ToString Applied to the Number Type
    // Flash Player implementation: printf("%.15g", d)
    // This handles NaN, Infinity, -0, exponential notation automatically

    char buf[32];
    snprintf(buf, sizeof(buf), "%.15g", d);
    return strdup(buf);

    // Note: Integer types (int, uint) may need special handling
    // This matches Flash Player behavior for Number type
}
```

### Opcode Implementation: `add`

```c
// 0xA0: add - The infamous complex opcode
void opcode_add(AVM2Context* ctx) {
    // Pop operands
    AS3Value* value2 = pop(ctx);
    AS3Value* value1 = pop(ctx);
    AS3Value* value3 = NULL;

    // 1. Both numbers → numeric addition
    if ((value1->type == TYPE_NUMBER || value1->type == TYPE_INT ||
         value1->type == TYPE_UINT) &&
        (value2->type == TYPE_NUMBER || value2->type == TYPE_INT ||
         value2->type == TYPE_UINT)) {

        double n1 = toNumber(value1);
        double n2 = toNumber(value2);
        value3 = createNumber(n1 + n2);
    }
    // 2. String or Date → string concatenation
    else if (value1->type == TYPE_STRING || value2->type == TYPE_STRING) {
        char* s1 = toString(value1);
        char* s2 = toString(value2);

        size_t len1 = strlen(s1);
        size_t len2 = strlen(s2);
        char* result = malloc(len1 + len2 + 1);
        strcpy(result, s1);
        strcat(result, s2);

        value3 = createString(result);

        free(s1);
        free(s2);
        free(result);
    }
    // 3. Both XML → XMLList
    else if (value1->type == TYPE_XML && value2->type == TYPE_XML) {
        value3 = createXMLList();
        xmlListAppend(value3->value.obj, value1);
        xmlListAppend(value3->value.obj, value2);
    }
    // 4. ToPrimitive conversion
    else {
        AS3Value* prim1 = toPrimitive(value1, HINT_NONE);
        AS3Value* prim2 = toPrimitive(value2, HINT_NONE);

        if (prim1->type == TYPE_STRING || prim2->type == TYPE_STRING) {
            // String concatenation
            char* s1 = toString(prim1);
            char* s2 = toString(prim2);

            size_t len1 = strlen(s1);
            size_t len2 = strlen(s2);
            char* result = malloc(len1 + len2 + 1);
            strcpy(result, s1);
            strcat(result, s2);

            value3 = createString(result);

            free(s1);
            free(s2);
            free(result);
        } else {
            // Numeric addition
            double n1 = toNumber(prim1);
            double n2 = toNumber(prim2);
            value3 = createNumber(n1 + n2);
        }

        release(prim1);
        release(prim2);
    }

    // Push result
    push(ctx, value3);

    // Cleanup
    release(value1);
    release(value2);
    release(value3);
}
```

---

## Future: Patching and Modding

Once the core AS3 implementation is complete, SWFRecomp will support patching and modding of Flash content.

### How Patching Works

**Directory Structure:**
```
project/
├── game.swf              # Original game SWF
├── patches/              # Patch directory
│   ├── MyPatch.as        # ActionScript patch file
│   └── Makefile          # Builds patches.swf
└── config.toml           # SWFRecomp configuration
```

**Patch Workflow:**
1. Create ActionScript files in `patches/` directory
2. Makefile compiles AS files to `patches.swf`
3. SWFRecomp recompiles both `game.swf` and `patches.swf`
4. Patches take priority over original code
5. Generated C code includes both original and patched classes

### Patching Use Cases

**1. Blind Patching (No Source Required)**

Patch a function without knowing its implementation:

```actionscript
// patches/RemoveDRM.as
package {
    public class Game {
        // Replace URL check function to always return true
        public function isValidURL():Boolean {
            return true;
        }
    }
}
```

**2. Full Function Replacement (With Source)**

If you have the original source, you can modify specific behavior:

```actionscript
// Original function in game
public function calculateDamage(attacker:Unit, defender:Unit):Number {
    return attacker.power - defender.defense;
}

// patches/BalanceTweak.as
public function calculateDamage(attacker:Unit, defender:Unit):Number {
    var baseDamage:Number = attacker.power - defender.defense;
    // Add critical hit mechanic
    if (Math.random() < attacker.critChance) {
        baseDamage *= 2.0;
    }
    return Math.max(0, baseDamage);
}
```

**3. Modding Framework**

For projects like Archipelago randomizers:

```actionscript
// patches/ArchipelagoIntegration.as
package {
    public class Seedling {
        // Hook into item collection
        public function collectItem(itemId:int):void {
            // Send to Archipelago server
            ArchipelagoClient.sendItemCollected(itemId);

            // Original item collection logic
            originalCollectItem(itemId);
        }
    }
}
```

### Access to SWF Data Without Source

Even without original source code, patches can access:
- Sprite names and properties
- Timeline data
- Embedded assets
- Class and function names

ActionScript's dynamic nature allows patching based on runtime inspection:

```actionscript
// Access sprite by name
var sprite:Sprite = getChildByName("player") as Sprite;

// Call unknown function by name
var obj:Object = getSomeObject();
obj["unknownFunction"](arg1, arg2);

// Check if property exists
if (obj.hasOwnProperty("health")) {
    obj.health = 100;
}
```

---

## References

### Specifications

1. **AVM2 Overview** (Adobe, 2007)
   - https://www.adobe.com/content/dam/acom/en/devnet/pdf/avm2overview.pdf
   - Official specification for AVM2 bytecode and execution model

2. **ABC File Format** (Adobe)
   - https://www.adobe.com/content/dam/acom/en/devnet/pdf/abcfileformat.pdf
   - Complete ABC file format specification with all structures

3. **ECMA-262** (ECMAScript Language Specification)
   - Section 9.1: ToPrimitive conversion
   - Section 9.2: ToBoolean conversion
   - Section 9.3: ToNumber conversion
   - Section 9.4: ToInteger conversion
   - Section 9.5: ToUint32 conversion
   - Section 9.8: ToString conversion
   - Section 11.6: Additive operators (+, -)
   - Section 11.9: Equality operators (==, ===, !=, !==)

4. **ECMA-357** (ECMAScript for XML - E4X)
   - For XML/XMLList operations if E4X support is needed

### Reference Implementations

1. **Tamarin** (Mozilla/Adobe)
   - https://github.com/adobe/avmplus
   - Original AS3 VM implementation (C++)
   - Best reference for opcode semantics and edge cases

2. **Ruffle** (Rust)
   - https://github.com/ruffle-rs/ruffle
   - Modern Flash emulator with growing AS3 support
   - Good reference for practical implementation approaches

3. **JPEXS Free Flash Decompiler** (FFDec)
   - https://github.com/jindrapetrik/jpexs-decompiler
   - ABC parser and decompiler (Java)
   - Useful for understanding ABC format in practice

### Tools

1. **RABCDAsm**
   - https://github.com/CyberShadow/RABCDAsm
   - ABC assembler/disassembler
   - Essential for testing and debugging bytecode

2. **Valgrind**
   - https://valgrind.org/
   - Memory leak detection for C
   - Critical for finding memory management bugs

3. **AddressSanitizer (ASan)**
   - Part of Clang/GCC
   - Buffer overflow and use-after-free detection
   - Faster than Valgrind for development

4. **Flash Player Projector**
   - Official Flash Player standalone
   - Use for testing expected behavior

### Additional Resources

1. **SWF File Format Specification**
   - https://www.adobe.com/content/dam/acom/en/devnet/pdf/swf-file-format-spec.pdf
   - For understanding SWF structure and tags

2. **ActionScript 3.0 Language Reference**
   - https://help.adobe.com/en_US/FlashPlatform/reference/actionscript/3/
   - Documentation for all built-in classes and functions

---

## Conclusion

Implementing AS3 support in SWFRecomp is a substantial undertaking, but the pure C approach is the right choice for this project. The complexity comes from the AS3/AVM2 specification itself, not from the implementation language.

**Key Takeaways:**

1. **Pure C is necessary** for small binaries and fast execution in WASM
2. **The compiler simplifies the bytecode** - we don't need to handle all OOP complexity
3. **Focus on the specification** - ECMA-262 algorithms and AVM2 opcodes are the real challenge
4. **Incremental implementation** - Build and test phase by phase
5. **Memory management is critical** - Use Valgrind and ASan extensively

**Next Steps:**

1. Complete AS1/2 stabilization and testing
2. Create simple AS3 test SWFs for validation
3. Begin Phase 1: ABC Parser implementation
4. Establish testing infrastructure early
5. Document as you go - future maintainers will thank you

This implementation will enable preservation of thousands of AS3-based Flash games and applications, bringing them to modern platforms through static recompilation.
