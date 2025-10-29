# ActionScript 3 (AS3) Implementation Plan for SWFRecomp - Pure C Approach

**Document Version:** 1.0
**Date:** October 28, 2025
**Status:** Planning Phase
**Approach:** Pure C Implementation (per LittleCube's guidance)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Why Pure C (LittleCube's Perspective)](#why-pure-c-littlecubes-perspective)
3. [The Real Complexity: AS3 Semantics](#the-real-complexity-as3-semantics)
4. [Architecture Overview](#architecture-overview)
5. [Implementation Roadmap](#implementation-roadmap)
6. [Technical Specifications](#technical-specifications)
7. [Code Examples](#code-examples)
8. [Effort Estimates](#effort-estimates)
9. [Risk Assessment](#risk-assessment)
10. [References](#references)

---

## Executive Summary

This document outlines a **pure C implementation** of ActionScript 3 (AS3) support for SWFRecomp, based on feedback from LittleCube (the upstream maintainer).

### Key Points from LittleCube's Feedback

> "we shouldn't use C++, we can't afford the bloat/overhead, we need the raw power of pure C"

> "C++ doesn't actually help that much with making the implementation simpler either. We'd need to implement a lot of things manually either way, like toNumber and toString"

> "the point being that the complexity of these instructions is not something C++ can help us with, and will most likely only get in the way unfortunately"

### Key Findings

- **The complexity is in AS3's semantics, not in the implementation language**
- **Example: The `add` opcode** has 4 different behaviors based on operand types (Number, String, Date, XML, XMLList, primitives with ToPrimitive conversion)
- **Manual implementation required**: Even in C++, we'd need to manually implement `toNumber`, `toString`, `ToPrimitive`, type coercion, etc.
- **C++ overhead not justified**: Smart pointers, exceptions, virtual functions add bloat without simplifying the core logic
- **Keep existing architecture**: Current C codebase is clean, fast, and maintainable

### Estimated Effort

- **14-20 months** (1800-2800 hours) with pure C
- **Major components**: ABC parser, DoABC tag handler, C runtime library, type system, 164 opcode implementations
- **Complexity comes from**: AS3 specification, not from C vs C++

---

## Why Pure C (LittleCube's Perspective)

### The `add` Opcode: A Case Study in AS3 Complexity

Here's what AS3 requires for the simple `add` instruction:

```
add_d instruction (0xA0):

1. If value1 and value2 are both Numbers, then set value3 to the
   result of adding the two numbers according to ECMA-262 section 11.6.3.

2. If value1 or value2 is a String or a Date, convert both values to
   String using ToString algorithm (ECMA-262 section 9.8).
   Concatenate string value of value2 to value1, set value3 to
   concatenated String.

3. If value1 and value2 are both XML or XMLList, construct a new
   XMLList object, call [[Append]](value1), then [[Append]](value2).
   Set value3 to the new XMLList (ECMA-357 section 9.2.1.6).

4. If none of the above apply, convert value1 and value2 to primitives
   using ToPrimitive with no hint. This results in value1_primitive and
   value2_primitive. If either is a String, convert both to Strings using
   ToString (ECMA-262 9.8), concatenate, set value3 to concatenated String.
   Otherwise convert both to Numbers using ToNumber (ECMA-262 9.3), add,
   set value3 to result.
```

### What C++ Doesn't Solve

**Problem:** We still need to implement all of this logic manually:
- `ToString` algorithm (ECMA-262 section 9.8)
- `ToNumber` algorithm (ECMA-262 section 9.3)
- `ToPrimitive` algorithm (ECMA-262 section 9.1)
- Type checking for String, Date, XML, XMLList, Number
- String concatenation
- Numeric addition with IEEE 754 rules
- XMLList construction and `[[Append]]` method

**In C++:**
```cpp
AS3Value* add(AS3Value* v1, AS3Value* v2) {
    // Still need to manually implement all the type checking
    if (isNumber(v1) && isNumber(v2)) {
        return createNumber(toNumber(v1) + toNumber(v2));
    } else if (isString(v1) || isString(v2) || isDate(v1) || isDate(v2)) {
        std::string s1 = toString(v1);  // Still manual
        std::string s2 = toString(v2);  // Still manual
        return createString(s1 + s2);
    } else if (isXML(v1) && isXML(v2)) {
        // ... manual XMLList construction
    } else {
        AS3Value* p1 = toPrimitive(v1);  // Still manual
        AS3Value* p2 = toPrimitive(v2);  // Still manual
        // ... rest of logic
    }
}
```

**In C:**
```c
AS3Value* add(AS3Value* v1, AS3Value* v2) {
    // Exactly the same logic, just different syntax
    if (isNumber(v1) && isNumber(v2)) {
        return createNumber(toNumber(v1) + toNumber(v2));
    } else if (isString(v1) || isString(v2) || isDate(v1) || isDate(v2)) {
        char* s1 = toString(v1);  // Still manual
        char* s2 = toString(v2);  // Still manual
        return createString(concat(s1, s2));
        free(s1); free(s2);
    } else if (isXML(v1) && isXML(v2)) {
        // ... manual XMLList construction
    } else {
        AS3Value* p1 = toPrimitive(v1);  // Still manual
        AS3Value* p2 = toPrimitive(v2);  // Still manual
        // ... rest of logic
    }
}
```

**Conclusion:** C++ doesn't make this simpler. The complexity is in **understanding and implementing ECMA-262 sections 9.1, 9.3, 9.8, and 11.6.3**, not in the implementation language.

### Where C++ Adds Overhead Without Benefit

1. **Virtual function calls**: Each `toString()` call goes through vtable lookup (slower than direct function call)
2. **Smart pointers**: `std::shared_ptr` has atomic reference counting (slower than simple refcount)
3. **Exception handling**: Unwinding overhead even when not throwing
4. **Template bloat**: Binary size increases significantly
5. **RTTI overhead**: Runtime type information increases binary size

**None of these help with the AS3 specification complexity.**

### Advantages of Pure C

1. **Smaller binaries**: Critical for WASM (every KB matters)
2. **Predictable performance**: No hidden vtable lookups, no atomic ops
3. **Simpler toolchain**: Single language, no C++ runtime dependencies
4. **Matches existing codebase**: SWFModernRuntime is already C17
5. **Easier to debug**: No template errors, no vtable confusion

---

## The Real Complexity: AS3 Semantics

### Type Conversion Algorithms (ECMA-262)

These need manual implementation regardless of language:

#### Section 9.1: ToPrimitive

```c
AS3Value* toPrimitive(AS3Value* input, ConversionHint hint) {
    // If already primitive, return as-is
    if (input->type == TYPE_NUMBER || input->type == TYPE_STRING ||
        input->type == TYPE_BOOLEAN || input->type == TYPE_NULL ||
        input->type == TYPE_UNDEFINED) {
        return input;
    }

    // Object - call [[DefaultValue]] with hint
    return defaultValue(input, hint);
}

AS3Value* defaultValue(AS3Value* obj, ConversionHint hint) {
    // Complex algorithm involving valueOf() and toString() methods
    // See ECMA-262 section 8.6.2.6

    if (hint == HINT_STRING) {
        AS3Value* str = callMethod(obj, "toString");
        if (isPrimitive(str)) return str;

        AS3Value* val = callMethod(obj, "valueOf");
        if (isPrimitive(val)) return val;

        throw_error("TypeError");
    } else {
        AS3Value* val = callMethod(obj, "valueOf");
        if (isPrimitive(val)) return val;

        AS3Value* str = callMethod(obj, "toString");
        if (isPrimitive(str)) return str;

        throw_error("TypeError");
    }
}
```

#### Section 9.3: ToNumber

```c
double toNumber(AS3Value* input) {
    switch (input->type) {
        case TYPE_UNDEFINED:
            return NAN;

        case TYPE_NULL:
            return 0.0;

        case TYPE_BOOLEAN:
            return input->value.b ? 1.0 : 0.0;

        case TYPE_NUMBER:
            return input->value.d;

        case TYPE_STRING:
            // Parse string to number (complex rules)
            return parseStringToNumber(input->value.s);

        case TYPE_OBJECT:
            // Convert to primitive with number hint, then convert
            AS3Value* prim = toPrimitive(input, HINT_NUMBER);
            return toNumber(prim);

        default:
            return NAN;
    }
}

double parseStringToNumber(const char* str) {
    // ECMA-262 section 9.3.1: StringNumericLiteral grammar
    // Must handle:
    // - Empty string → 0
    // - Whitespace trimming
    // - "Infinity", "-Infinity", "+Infinity"
    // - Decimal literals
    // - Hex literals (0x prefix)
    // - Scientific notation (1e10)
    // - Invalid → NaN

    // 100+ lines of parsing logic...
}
```

#### Section 9.8: ToString

```c
char* toString(AS3Value* input) {
    switch (input->type) {
        case TYPE_UNDEFINED:
            return strdup("undefined");

        case TYPE_NULL:
            return strdup("null");

        case TYPE_BOOLEAN:
            return strdup(input->value.b ? "true" : "false");

        case TYPE_NUMBER: {
            double d = input->value.d;

            // NaN
            if (isnan(d)) return strdup("NaN");

            // +0, -0
            if (d == 0.0) return strdup("0");

            // Infinity
            if (isinf(d)) {
                return strdup(d > 0 ? "Infinity" : "-Infinity");
            }

            // Regular number - complex formatting rules
            return formatNumber(d);
        }

        case TYPE_STRING:
            return strdup(input->value.s);

        case TYPE_OBJECT:
            // Convert to primitive with string hint, then convert
            AS3Value* prim = toPrimitive(input, HINT_STRING);
            return toString(prim);

        default:
            return strdup("");
    }
}

char* formatNumber(double d) {
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

### More Complex Opcodes

The `add` instruction is just one example. Here are others:

#### `multiply` (0xA2)

Similar complexity to `add`:
```
1. Convert value1 to Number using ToNumber
2. Convert value2 to Number using ToNumber
3. Multiply using IEEE 754 rules
4. Push result

Special cases:
- NaN * anything = NaN
- Infinity * 0 = NaN
- Infinity * Infinity = Infinity
- Sign rules for negative numbers
```

#### `equals` (0xAB)

```
ECMA-262 section 11.9.3 - Abstract Equality Comparison:

1. If Type(x) == Type(y):
   a. If Undefined or Null: return true
   b. If Number:
      - If x is NaN: return false
      - If y is NaN: return false
      - If same value: return true
      - If x is +0 and y is -0: return true
      - If x is -0 and y is +0: return true
      - Otherwise: return false
   c. If String: compare sequences
   d. If Boolean: compare values
   e. If Object: return true if same object

2. If x is Null and y is Undefined: return true
3. If x is Undefined and y is Null: return true

4. If Type(x) is Number and Type(y) is String:
   return x == ToNumber(y)

5. If Type(x) is String and Type(y) is Number:
   return ToNumber(x) == y

6. If Type(x) is Boolean:
   return ToNumber(x) == y

7. If Type(y) is Boolean:
   return x == ToNumber(y)

8. If Type(x) is String or Number, and Type(y) is Object:
   return x == ToPrimitive(y)

9. If Type(x) is Object, and Type(y) is String or Number:
   return ToPrimitive(x) == y

10. Otherwise: return false
```

**Implementation (same in C and C++):**
```c
int equals(AS3Value* x, AS3Value* y) {
    // 50+ lines implementing all the above rules
    // No difference between C and C++
}
```

#### `getproperty` (0x66)

```
1. Pop multiname index from stack
2. Pop object from stack
3. Resolve multiname to actual property name (namespace resolution)
4. Look up property on object:
   a. Check object's own properties
   b. Check prototype chain
   c. Check dynamic properties
   d. Check traits (for sealed classes)
5. If property is a getter, call the getter function
6. Push result
```

**Namespace resolution alone is complex:**
```c
const char* resolveMultiname(Multiname* mn, AS3Object* obj) {
    // Multiname can be:
    // - QName (qualified name with single namespace)
    // - Multiname (runtime namespaces)
    // - RTQName (runtime qualified name)
    // - RTQNameL (late-bound runtime qualified name)
    // - MultinameL (late-bound multiname)

    // Each type has different resolution rules
    // Namespaces can be: public, private, protected, internal, custom
    // Must check visibility rules based on caller context

    // 100+ lines of logic...
}
```

---

## Architecture Overview

### Core Philosophy

**Keep it simple, keep it C.**

- Use same architecture as existing AS1/2 implementation
- Tagged union for values (just like current code)
- Manual memory management with clear ownership rules
- Function pointers for method dispatch (no vtables)
- Explicit type checking (no RTTI)

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

### Object Representation

```c
typedef struct AS3Object {
    AS3Class* klass;           // Object's class
    AS3Object* prototype;      // Prototype chain
    HashMap* properties;       // Dynamic properties (name → AS3Value*)
    AS3Value** slots;          // Fixed slots for sealed classes
    uint32_t slot_count;
    uint32_t refcount;
} AS3Object;

typedef struct AS3Class {
    const char* name;
    AS3Class* super_class;
    AS3Namespace* ns;
    Trait* traits;             // Array of traits (properties, methods)
    uint32_t trait_count;
    AS3Function* constructor;
    uint32_t slot_count;       // Number of fixed slots
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
    AS3Value** default_values; // Default parameter values
    uint8_t has_rest;          // Has ...rest parameter
    AS3Value** closure_vars;   // Captured variables
    uint32_t closure_count;
    uint32_t refcount;
} AS3Function;

typedef struct FunctionBody {
    uint8_t* code;             // Bytecode
    uint32_t code_length;
    uint32_t max_stack;
    uint32_t local_count;
    uint32_t scope_depth;
    ExceptionHandler* exceptions;
    uint32_t exception_count;
} FunctionBody;
```

### Memory Management

```c
// Simple reference counting (same as AS1/2)

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

            case TYPE_ARRAY:
                release_array(v->value.arr);
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

**Note:** Reference cycles (A→B→A) require manual breaking or a mark-sweep GC pass. For most Flash content, simple refcounting is sufficient.

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

    AS3Object* global_object;  // Global object
    HashMap* global_properties;

    ExceptionFrame* exception_frame;  // For try/catch
} AVM2Context;
```

---

## Implementation Roadmap

### Phase 1: ABC Parser (3-5 months)

**Goal:** Parse DoABC tags and extract all ABC structures.

**Components:**
1. **ABC file format parser** (`abc_parser.c`)
   - Read minor_version, major_version
   - Parse constant pools (int, uint, double, string, namespace, ns_set, multiname)
   - Parse method_info array
   - Parse metadata_info array
   - Parse instance_info array
   - Parse class_info array
   - Parse script_info array
   - Parse method_body_info array
   - Parse trait structures

2. **Data structures** (`abc_types.h`)
   - `ABCFile` structure
   - `ConstantPool` structure
   - `MethodInfo` structure
   - `ClassInfo` structure
   - `ScriptInfo` structure
   - `MethodBodyInfo` structure
   - `ExceptionInfo` structure
   - `MultinameInfo` structure

3. **Validation**
   - Verify ABC version compatibility
   - Validate constant pool indices
   - Check for malformed structures

**Testing:**
- Parse simple AS3 SWF files
- Extract and print all ABC data
- Verify constant pools are correct

**Effort:** 300-600 hours

### Phase 2: Code Generation Framework (2-3 months)

**Goal:** Generate C code from ABC bytecode (stub implementations).

**Components:**
1. **C code generator** (`abc_codegen.c`)
   - Generate C functions for each AS3 method
   - Generate structures for each AS3 class
   - Generate constant data tables
   - Generate initialization code

2. **Opcode translation** (`abc_opcodes.c`)
   - Start with stub implementations
   - Translate each opcode to C function call
   - Handle control flow (jumps, branches)

3. **Build system integration**
   - Modify SWFRecomp to detect AS3 vs AS1/2
   - Route DoABC tags to ABC parser
   - Generate appropriate build files

**Testing:**
- Generate C code for simple AS3 methods
- Verify generated code compiles
- Run with stub opcodes (no-ops)

**Effort:** 200-400 hours

### Phase 3: Type System & Core Opcodes (3-4 months)

**Goal:** Implement AS3 value system and basic opcodes.

**Components:**
1. **Value implementation** (`avm2_types.c`)
   - `AS3Value` structure and operations
   - Type checking functions
   - Type conversion functions (ToNumber, ToString, ToPrimitive, ToBoolean, ToInt32, ToUint32)
   - Value creation/destruction
   - Reference counting

2. **Core opcodes** (20-30 opcodes)
   - Stack operations: `dup`, `pop`, `swap`, `nop`
   - Push constants: `pushbyte`, `pushshort`, `pushint`, `pushuint`, `pushdouble`, `pushstring`, `pushtrue`, `pushfalse`, `pushnull`, `pushundefined`, `pushnan`
   - Locals: `getlocal`, `setlocal`, `getlocal_0-3`, `setlocal_0-3`
   - Arithmetic: `add`, `add_i`, `subtract`, `subtract_i`, `multiply`, `multiply_i`, `divide`, `modulo`, `negate`, `negate_i`
   - Comparison: `equals`, `strictequals`, `lessthan`, `lessequals`, `greaterthan`, `greaterequals`
   - Logical: `not`
   - Control: `jump`, `iftrue`, `iffalse`, `returnvalue`, `returnvoid`

3. **Testing**
   - Simple arithmetic tests
   - Type conversion tests
   - Control flow tests

**Effort:** 300-500 hours

### Phase 4: Object Model (3-5 months)

**Goal:** Implement AS3 objects, classes, and inheritance.

**Components:**
1. **Object system** (`avm2_object.c`)
   - `AS3Object` implementation
   - Property access (get/set)
   - Prototype chain traversal
   - Dynamic properties
   - Slot access

2. **Class system** (`avm2_class.c`)
   - `AS3Class` implementation
   - Class instantiation
   - Inheritance
   - Trait resolution
   - Constructor calls

3. **Namespace system** (`avm2_namespace.c`)
   - Namespace resolution
   - Multiname resolution
   - Visibility checking (public, private, protected, internal)

4. **Object opcodes** (30-40 opcodes)
   - Property access: `getproperty`, `setproperty`, `initproperty`, `deleteproperty`, `getsuper`, `setsuper`
   - Slot access: `getslot`, `setslot`, `getglobalslot`, `setglobalslot`
   - Lexical access: `getlex`, `findproperty`, `findpropstrict`
   - Scope: `pushscope`, `popscope`, `getglobalscope`, `getscopeobject`
   - Object creation: `newobject`, `newarray`, `newclass`, `newactivation`
   - Method calls: `call`, `callmethod`, `callproperty`, `callproplex`, `callpropvoid`, `callstatic`, `callsuper`, `callsupervoid`, `construct`, `constructprop`, `constructsuper`

5. **Testing**
   - Object creation and property access
   - Class instantiation
   - Inheritance tests
   - Method calls

**Effort:** 400-700 hours

### Phase 5: Built-in Classes (2-4 months)

**Goal:** Implement Flash/AS3 built-in classes.

**Components:**
1. **Core classes** (`avm2_builtins.c`)
   - `Object` - Base class
   - `Array` - Dynamic arrays
   - `String` - Immutable strings
   - `Number`, `int`, `uint` - Numeric types
   - `Boolean` - Boolean type
   - `Function` - Function type
   - `Class` - Class type
   - `Math` - Math functions
   - `Date` - Date/time
   - `RegExp` - Regular expressions
   - `Error`, `TypeError`, `RangeError`, etc. - Error types

2. **Flash Display classes** (minimal subset)
   - `DisplayObject`
   - `Sprite`
   - `MovieClip`
   - `Shape`
   - `Graphics`
   - `Bitmap`, `BitmapData`
   - `TextField`, `TextFormat`

3. **Flash Event classes**
   - `Event`
   - `EventDispatcher`
   - `MouseEvent`
   - `KeyboardEvent`

4. **Testing**
   - Built-in class instantiation
   - Method calls on built-ins
   - Flash display tests

**Effort:** 300-600 hours

### Phase 6: Advanced Features (3-4 months)

**Goal:** Implement remaining opcodes and advanced features.

**Components:**
1. **Remaining opcodes** (60-80 opcodes)
   - Type conversion: `convert_i`, `convert_u`, `convert_d`, `convert_b`, `convert_s`, `convert_o`, `coerce`, `coerce_a`, `coerce_s`, `astype`, `astypelate`, `istype`, `istypelate`, `instanceof`
   - Bitwise: `bitand`, `bitor`, `bitxor`, `bitnot`, `lshift`, `rshift`, `urshift`
   - Increment/decrement: `increment`, `increment_i`, `decrement`, `decrement_i`, `inclocal`, `inclocal_i`, `declocal`, `declocal_i`
   - Advanced control: `lookupswitch`
   - Advanced iteration: `hasnext`, `hasnext2`, `nextname`, `nextvalue`
   - Type checking: `typeof`, `in`, `checkfilter`
   - XML operations: E4X support if needed
   - Debug operations: `debug`, `debugfile`, `debugline`

2. **Exception handling** (`avm2_exceptions.c`)
   - `throw` opcode
   - Try/catch/finally blocks
   - Exception propagation
   - Stack unwinding

3. **Advanced object features**
   - Getters and setters
   - Property attributes (enumerable, configurable)
   - Object sealing/freezing
   - Proxy objects (for flash.utils.Proxy)

4. **Testing**
   - Exception handling tests
   - Advanced opcode tests
   - Complex control flow

**Effort:** 400-600 hours

### Phase 7: Integration & Optimization (2-3 months)

**Goal:** Integrate with SWFModernRuntime, optimize, and test.

**Components:**
1. **Runtime integration**
   - Connect to Vulkan rendering
   - Event system integration
   - Asset loading
   - Sound support

2. **Optimization**
   - Optimize hot paths (type checking, property access)
   - Reduce allocations
   - Optimize generated code
   - Profile and tune

3. **Testing**
   - Real Flash games/apps
   - Performance benchmarks
   - Memory leak detection
   - Stress testing

4. **Documentation**
   - API documentation
   - Architecture documentation
   - User guide for recompiling AS3 SWFs

**Effort:** 300-500 hours

---

## Technical Specifications

### ABC File Format

**Header:**
```c
typedef struct {
    uint16_t minor_version;
    uint16_t major_version;
} ABCHeader;
```

**Constant Pool:**
```c
typedef struct {
    // Integers
    int32_t* int_pool;
    uint32_t int_count;

    // Unsigned integers
    uint32_t* uint_pool;
    uint32_t uint_count;

    // Doubles
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
    uint32_t return_type;      // Index into multiname pool
    uint32_t* param_types;     // Indices into multiname pool
    const char* name;          // Index into string pool
    uint8_t flags;             // NEED_ARGUMENTS, NEED_ACTIVATION, NEED_REST, etc.
    uint32_t option_count;     // Default parameter values
    OptionDetail* options;
} MethodInfo;
```

**Class Info:**
```c
typedef struct {
    const char* name;          // Multiname index
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

**Script Info:**
```c
typedef struct {
    uint32_t init;             // Script initializer (method index)
    Trait* traits;
    uint32_t trait_count;
} ScriptInfo;
```

**Method Body:**
```c
typedef struct {
    uint32_t method;           // Method index this body is for
    uint32_t max_stack;
    uint32_t local_count;
    uint32_t init_scope_depth;
    uint32_t max_scope_depth;
    uint32_t code_length;
    uint8_t* code;             // Bytecode
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
    uint32_t exc_type;         // Multiname index for exception type
    uint32_t var_name;         // Multiname index for exception variable
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
    const char* name;          // Multiname index
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

### All 164 AS3 Opcodes

Here's the complete list organized by category:

#### Stack Manipulation (5)
```
0x29  pop
0x2A  dup
0x2B  swap
0x1D  popscope
0x30  pushscope
```

#### Push Constants (14)
```
0x20  pushnull
0x21  pushundefined
0x22  pushconstant  (unused)
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

#### Locals (20)
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

#### Arithmetic (19)
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

#### Comparison (12)
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

#### Type Conversion (23)
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

#### Property Access (12)
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

#### Slot Access (8)
```
0x6C  getslot
0x6D  setslot
0x6E  getglobalslot
0x6F  setglobalslot
```

#### Scope Access (4)
```
0x64  getglobalscope
0x65  getscopeobject
0x1D  popscope
0x30  pushscope
```

#### Object/Array Construction (5)
```
0x55  newobject
0x56  newarray
0x58  newclass
0x5A  newactivation
0x42  construct
```

#### Function/Method Calls (12)
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

#### XML/E4X (7)
```
0x71  esc_xelem
0x72  esc_xattr
0x06  dxns
0x07  dxnslate
```

#### Debug (6)
```
0x01  bkpt
0x02  nop
0xEF  debug
0xF0  debugline
0xF1  debugfile
0xF2  timestamp
```

#### Alchemy/Memory (14)
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

#### Other (5)
```
0x08  kill     (kill local variable)
0x09  label    (label for debugger)
0x53  applytype
0x93  pushwith
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

**Generated C code:**

**File: `as3_classes.h`**
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

**File: `as3_Hello.c`**
```c
#include "as3_classes.h"
#include "avm2_runtime.h"
#include <stdio.h>
#include <stdlib.h>

// Hello class definition
AS3Class Hello_class = {
    .name = "Hello",
    .super_class = &Object_class,  // Inherits from Object
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

    // No instance variables to initialize

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

**File: `as3_main.c`**
```c
#include "avm2_runtime.h"
#include "as3_classes.h"

void as3_script_0(AVM2Context* ctx) {
    // ABC bytecode for script initialization:
    // 0x00: getlocal_0
    // 0x01: pushscope
    // 0x02: findpropstrict QName("Hello")
    // 0x07: getproperty QName("Hello")
    // 0x0C: dup
    // 0x0D: pushscope
    // 0x0E: newclass Hello_class
    // 0x13: popscope
    // 0x14: initproperty QName("Hello")
    // 0x19: returnvoid

    // DESIGN NOTE: For class registration, the recompiler could optimize this
    // to direct global property initialization since the QName "Hello" is known.
    // However, this example shows the explicit bytecode translation approach.

    // getlocal_0 - get global object
    AS3Value* global = getlocal(ctx, 0);

    // pushscope
    pushscope(ctx, global);

    // Optimized version (if recompiler handles QNames at compile-time):
    // initGlobalProperty(ctx, "Hello", createClass(&Hello_class));

    // Explicit translation (for illustration):
    AS3Value* scope = findpropstrict(ctx, "Hello", NULL);
    AS3Value* hello_class_val = createClass(&Hello_class);
    initproperty(ctx, scope, "Hello", NULL, hello_class_val);

    release(global);
    release(scope);
    release(hello_class_val);
}

int main() {
    // Initialize AVM2 runtime
    AVM2Context* ctx = createAVM2Context();

    // Run script initialization
    as3_script_0(ctx);

    // Example: Create Hello instance and call sayHello
    AS3Value* hello_class = getGlobalProperty(ctx, "Hello");
    AS3Value* hello_inst = constructClass(ctx, hello_class, 0, NULL);
    AS3Value* result = Hello_sayHello(ctx, hello_inst);

    release(hello_class);
    release(hello_inst);
    release(result);

    // Cleanup
    destroyAVM2Context(ctx);

    return 0;
}
```

### Type Conversion Implementation

**File: `avm2_convert.c`**
```c
#include "avm2_runtime.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// ECMA-262 Section 9.1: ToPrimitive
AS3Value* toPrimitive(AS3Value* input, ConversionHint hint) {
    // If already primitive, return as-is
    switch (input->type) {
        case TYPE_UNDEFINED:
        case TYPE_NULL:
        case TYPE_BOOLEAN:
        case TYPE_INT:
        case TYPE_UINT:
        case TYPE_NUMBER:
        case TYPE_STRING:
            return retain(input);

        case TYPE_OBJECT:
            return defaultValue(input->value.obj, hint);

        default:
            return createUndefined();
    }
}

// ECMA-262 Section 8.6.2.6: [[DefaultValue]]
AS3Value* defaultValue(AS3Object* obj, ConversionHint hint) {
    AS3Value* result;

    if (hint == HINT_STRING) {
        // Try toString first
        result = callMethod(obj, "toString", 0, NULL);
        if (isPrimitive(result)) {
            return result;
        }
        release(result);

        // Try valueOf
        result = callMethod(obj, "valueOf", 0, NULL);
        if (isPrimitive(result)) {
            return result;
        }
        release(result);

        // TypeError
        throwError("TypeError: Cannot convert object to primitive");
        return createUndefined();
    } else {
        // Try valueOf first
        result = callMethod(obj, "valueOf", 0, NULL);
        if (isPrimitive(result)) {
            return result;
        }
        release(result);

        // Try toString
        result = callMethod(obj, "toString", 0, NULL);
        if (isPrimitive(result)) {
            return result;
        }
        release(result);

        // TypeError
        throwError("TypeError: Cannot convert object to primitive");
        return createUndefined();
    }
}

// ECMA-262 Section 9.3: ToNumber
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

// ECMA-262 Section 9.3.1: Parse string to number
double stringToNumber(const char* str) {
    // Trim whitespace
    while (isspace(*str)) str++;

    if (*str == '\0') {
        return 0.0;  // Empty string → 0
    }

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

    // If didn't consume entire string (and remainder not whitespace), it's NaN
    if (*endptr != '\0') {
        return NAN;
    }

    return val;
}

// ECMA-262 Section 9.8: ToString
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

// ECMA-262 Section 9.8.1: Number to string conversion
char* numberToString(double d) {
    // NaN
    if (isnan(d)) {
        return strdup("NaN");
    }

    // +0, -0
    if (d == 0.0) {
        return strdup("0");
    }

    // Infinity
    if (isinf(d)) {
        return strdup(d > 0 ? "Infinity" : "-Infinity");
    }

    // Regular number
    // Use exponential notation for very large/small numbers
    if (fabs(d) >= 1e21 || (fabs(d) < 1e-6 && d != 0)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.15e", d);
        return strdup(buf);
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.15g", d);
        return strdup(buf);
    }
}

// ECMA-262 Section 9.2: ToBoolean
int toBoolean(AS3Value* input) {
    switch (input->type) {
        case TYPE_UNDEFINED:
        case TYPE_NULL:
            return 0;

        case TYPE_BOOLEAN:
            return input->value.b;

        case TYPE_INT:
            return input->value.i != 0;

        case TYPE_UINT:
            return input->value.ui != 0;

        case TYPE_NUMBER: {
            double d = input->value.d;
            return !isnan(d) && d != 0.0;
        }

        case TYPE_STRING:
            return input->value.s[0] != '\0';  // Empty string → false

        case TYPE_OBJECT:
            return 1;  // All objects are truthy

        default:
            return 0;
    }
}

// ECMA-262 Section 9.4: ToInteger
int32_t toInt32(AS3Value* input) {
    double d = toNumber(input);

    // NaN, ±Infinity, ±0 → 0
    if (isnan(d) || isinf(d) || d == 0.0) {
        return 0;
    }

    // Apply modulo 2^32
    int64_t i64 = (int64_t)d;
    uint32_t u32 = (uint32_t)i64;

    // Interpret as signed 32-bit
    return (int32_t)u32;
}

// ECMA-262 Section 9.5: ToUint32
uint32_t toUint32(AS3Value* input) {
    double d = toNumber(input);

    // NaN, ±Infinity, ±0 → 0
    if (isnan(d) || isinf(d) || d == 0.0) {
        return 0;
    }

    // Apply modulo 2^32
    int64_t i64 = (int64_t)d;
    return (uint32_t)i64;
}
```

### Opcode Implementation: `add`

**File: `avm2_opcodes.c`**
```c
#include "avm2_runtime.h"
#include <string.h>

// 0xA0: add
// This is the infamous complex opcode from LittleCube's example
void opcode_add(AVM2Context* ctx) {
    // Pop two values from stack
    AS3Value* value2 = pop(ctx);
    AS3Value* value1 = pop(ctx);
    AS3Value* value3 = NULL;

    // 1. If both are Numbers, add them
    if ((value1->type == TYPE_NUMBER || value1->type == TYPE_INT ||
         value1->type == TYPE_UINT) &&
        (value2->type == TYPE_NUMBER || value2->type == TYPE_INT ||
         value2->type == TYPE_UINT)) {

        double n1 = toNumber(value1);
        double n2 = toNumber(value2);
        value3 = createNumber(n1 + n2);
    }
    // 2. If either is String or Date, concatenate as strings
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
    // 3. If both are XML or XMLList, create new XMLList
    else if (value1->type == TYPE_XML && value2->type == TYPE_XML) {
        // Create new XMLList, append both values
        // (Simplified - real implementation would construct XMLList)
        value3 = createXMLList();
        xmlListAppend(value3->value.obj, value1);
        xmlListAppend(value3->value.obj, value2);
    }
    // 4. Otherwise, use ToPrimitive and decide based on result
    else {
        AS3Value* prim1 = toPrimitive(value1, HINT_NONE);
        AS3Value* prim2 = toPrimitive(value2, HINT_NONE);

        // If either primitive is String, concatenate
        if (prim1->type == TYPE_STRING || prim2->type == TYPE_STRING) {
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
        }
        // Otherwise, convert to numbers and add
        else {
            double n1 = toNumber(prim1);
            double n2 = toNumber(prim2);
            value3 = createNumber(n1 + n2);
        }

        release(prim1);
        release(prim2);
    }

    // Push result
    push(ctx, value3);

    // Release temporaries
    release(value1);
    release(value2);
    release(value3);
}

// 0xA1: subtract
void opcode_subtract(AVM2Context* ctx) {
    AS3Value* value2 = pop(ctx);
    AS3Value* value1 = pop(ctx);

    double n1 = toNumber(value1);
    double n2 = toNumber(value2);
    AS3Value* result = createNumber(n1 - n2);

    push(ctx, result);

    release(value1);
    release(value2);
    release(result);
}

// 0xA2: multiply
void opcode_multiply(AVM2Context* ctx) {
    AS3Value* value2 = pop(ctx);
    AS3Value* value1 = pop(ctx);

    double n1 = toNumber(value1);
    double n2 = toNumber(value2);
    AS3Value* result = createNumber(n1 * n2);

    push(ctx, result);

    release(value1);
    release(value2);
    release(result);
}

// 0xAB: equals (Abstract Equality Comparison)
void opcode_equals(AVM2Context* ctx) {
    AS3Value* value2 = pop(ctx);
    AS3Value* value1 = pop(ctx);

    int result = abstractEquals(value1, value2);
    AS3Value* bool_val = createBoolean(result);

    push(ctx, bool_val);

    release(value1);
    release(value2);
    release(bool_val);
}

// ECMA-262 Section 11.9.3: Abstract Equality Comparison
int abstractEquals(AS3Value* x, AS3Value* y) {
    // 1. If same type, use strict equality
    if (x->type == y->type) {
        return strictEquals(x, y);
    }

    // 2. null == undefined
    if ((x->type == TYPE_NULL && y->type == TYPE_UNDEFINED) ||
        (x->type == TYPE_UNDEFINED && y->type == TYPE_NULL)) {
        return 1;
    }

    // 3. Number == String: convert String to Number
    if ((x->type == TYPE_NUMBER || x->type == TYPE_INT || x->type == TYPE_UINT) &&
        y->type == TYPE_STRING) {
        double nx = toNumber(x);
        double ny = toNumber(y);
        return nx == ny && !isnan(nx) && !isnan(ny);
    }

    // 4. String == Number: convert String to Number
    if (x->type == TYPE_STRING &&
        (y->type == TYPE_NUMBER || y->type == TYPE_INT || y->type == TYPE_UINT)) {
        double nx = toNumber(x);
        double ny = toNumber(y);
        return nx == ny && !isnan(nx) && !isnan(ny);
    }

    // 5. Boolean == anything: convert Boolean to Number
    if (x->type == TYPE_BOOLEAN) {
        AS3Value* nx = createNumber(toNumber(x));
        int result = abstractEquals(nx, y);
        release(nx);
        return result;
    }

    // 6. Anything == Boolean: convert Boolean to Number
    if (y->type == TYPE_BOOLEAN) {
        AS3Value* ny = createNumber(toNumber(y));
        int result = abstractEquals(x, ny);
        release(ny);
        return result;
    }

    // 7. (String or Number) == Object: convert Object to primitive
    if ((x->type == TYPE_STRING || x->type == TYPE_NUMBER ||
         x->type == TYPE_INT || x->type == TYPE_UINT) &&
        y->type == TYPE_OBJECT) {
        AS3Value* py = toPrimitive(y, HINT_NONE);
        int result = abstractEquals(x, py);
        release(py);
        return result;
    }

    // 8. Object == (String or Number): convert Object to primitive
    if (x->type == TYPE_OBJECT &&
        (y->type == TYPE_STRING || y->type == TYPE_NUMBER ||
         y->type == TYPE_INT || y->type == TYPE_UINT)) {
        AS3Value* px = toPrimitive(x, HINT_NONE);
        int result = abstractEquals(px, y);
        release(px);
        return result;
    }

    // 9. Otherwise, not equal
    return 0;
}

// 0xAC: strictequals
void opcode_strictequals(AVM2Context* ctx) {
    AS3Value* value2 = pop(ctx);
    AS3Value* value1 = pop(ctx);

    int result = strictEquals(value1, value2);
    AS3Value* bool_val = createBoolean(result);

    push(ctx, bool_val);

    release(value1);
    release(value2);
    release(bool_val);
}

// ECMA-262 Section 11.9.6: Strict Equality Comparison
int strictEquals(AS3Value* x, AS3Value* y) {
    // Different types → not equal
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case TYPE_UNDEFINED:
        case TYPE_NULL:
            return 1;

        case TYPE_BOOLEAN:
            return x->value.b == y->value.b;

        case TYPE_INT:
            return x->value.i == y->value.i;

        case TYPE_UINT:
            return x->value.ui == y->value.ui;

        case TYPE_NUMBER: {
            double dx = x->value.d;
            double dy = y->value.d;

            // NaN !== NaN
            if (isnan(dx) || isnan(dy)) {
                return 0;
            }

            // +0 === -0
            if (dx == 0.0 && dy == 0.0) {
                return 1;
            }

            return dx == dy;
        }

        case TYPE_STRING:
            return strcmp(x->value.s, y->value.s) == 0;

        case TYPE_OBJECT:
            // Same object reference
            return x->value.obj == y->value.obj;

        default:
            return 0;
    }
}
```

---

## Effort Estimates

### Summary

| Phase | Duration | Hours | Cumulative |
|-------|----------|-------|------------|
| 1. ABC Parser | 3-5 months | 300-600h | 300-600h |
| 2. Code Generation | 2-3 months | 200-400h | 500-1000h |
| 3. Type System & Core Opcodes | 3-4 months | 300-500h | 800-1500h |
| 4. Object Model | 3-5 months | 400-700h | 1200-2200h |
| 5. Built-in Classes | 2-4 months | 300-600h | 1500-2800h |
| 6. Advanced Features | 3-4 months | 400-600h | 1900-3400h |
| 7. Integration & Optimization | 2-3 months | 300-500h | 2200-3900h |
| **TOTAL** | **18-28 months** | **2200-3900h** | |

### Comparison with C++ Approach

| Aspect | Pure C | C++ (from previous doc) |
|--------|--------|-------------------------|
| **Duration** | 18-28 months | 10-16 months |
| **Total Hours** | 2200-3900h | 1350-2350h |
| **Binary Size** | Smaller (WASM ~500KB) | Larger (WASM ~800KB+) |
| **Performance** | Faster (no vtables) | Slightly slower |
| **Complexity** | Same (ECMA-262 is hard) | Same (ECMA-262 is hard) |
| **Maintainability** | Good (familiar to team) | Mixed (two languages) |

### Why Pure C Takes Longer

Even though the algorithmic complexity is the same, pure C requires more time because:

1. **Manual data structures** - Need to implement our own dynamic arrays, hash maps, object systems (C++ has STL)
2. **Manual memory management** - Every allocation needs matching deallocation, harder to track (C++ has smart pointers)
3. **More boilerplate** - Type checking, casting, function dispatch all manual (C++ has templates, virtual functions)
4. **Exception handling** - setjmp/longjmp is more complex to implement correctly (C++ has native exceptions)
5. **Testing overhead** - More prone to memory leaks, need more debugging time (C++ catches more errors at compile time)

**But:** The result is smaller, faster, and more predictable.

### Where Time is Spent

**Same in C and C++ (1500-2000h):**
- Understanding ECMA-262 specification: 200-300h
- Understanding ABC format: 100-150h
- Implementing type conversion algorithms (ToNumber, ToString, ToPrimitive, etc.): 300-400h
- Implementing 164 opcodes (logic itself): 400-600h
- Implementing Flash API classes: 300-500h
- Testing and debugging: 200-300h

**Extra time in C (700-1900h):**
- Building data structure library (dynamic arrays, hash maps, etc.): 200-300h
- Manual memory management and debugging leaks: 200-400h
- Implementing exception handling with setjmp/longjmp: 100-200h
- Writing more boilerplate (type checking, casting, etc.): 200-400h
- Additional testing for memory safety: 100-200h

---

## Risk Assessment

### Technical Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Memory leaks** | High | Valgrind, AddressSanitizer, extensive testing |
| **ABC format changes** | Low | Format is stable (finalized in 2006) |
| **ECMA-262 complexity** | High | Reference Flash Player behavior, thorough testing |
| **Performance** | Medium | Profile, optimize hot paths, use inline functions |
| **Binary size** | Low | Pure C produces small binaries |

### Schedule Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Underestimated complexity** | High | Phase-based approach, stop after each milestone |
| **Scope creep** | Medium | Focus on core AS3, defer advanced features |
| **Testing overhead** | High | Incremental testing, automated test suite |

### Team Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| **Single developer** | High | Good documentation, modular architecture |
| **Burnout** | Medium | Break work into manageable milestones |
| **Context switching** | Medium | Focus on one phase at a time |

---

## References

### Specifications

1. **AVM2 Overview** (Adobe, 2007)
   - https://www.adobe.com/content/dam/acom/en/devnet/pdf/avm2overview.pdf
   - Official specification for AVM2 bytecode

2. **ABC File Format** (Adobe)
   - https://www.adobe.com/content/dam/acom/en/devnet/pdf/abcfileformat.pdf
   - Complete ABC file format specification

3. **ECMA-262** (ECMAScript Language Specification)
   - Section 9.1: ToPrimitive
   - Section 9.2: ToBoolean
   - Section 9.3: ToNumber
   - Section 9.8: ToString
   - Section 11.6: Additive Operators
   - Section 11.9: Equality Operators

4. **ECMA-357** (ECMAScript for XML - E4X)
   - For XML/XMLList operations

### Existing Implementations

1. **Tamarin** (Mozilla/Adobe)
   - https://github.com/adobe/avmplus
   - Original AS3 VM (C++)
   - Reference for opcode semantics

2. **Ruffle** (Rust)
   - https://github.com/ruffle-rs/ruffle
   - Modern Flash emulator
   - Good reference for AS3 implementation

3. **JPEXS Free Flash Decompiler**
   - https://github.com/jindrapetrik/jpexs-decompiler
   - ABC parser and decompiler (Java)
   - Useful for understanding ABC format

### Tools

1. **RABCDAsm**
   - https://github.com/CyberShadow/RABCDAsm
   - ABC assembler/disassembler
   - Essential for testing and debugging

2. **Valgrind**
   - Memory leak detection
   - Essential for C development

3. **AddressSanitizer**
   - Buffer overflow detection
   - Faster than Valgrind

---

## Conclusion

**Pure C implementation is the right choice for SWFRecomp.**

LittleCube's feedback is correct: **C++ doesn't help with the real complexity**, which comes from the AS3 specification itself (ECMA-262, ABC format, Flash APIs). We'd need to manually implement `toNumber`, `toString`, `ToPrimitive`, type coercion, and all the crazy opcode semantics regardless of language.

**Advantages of pure C:**
- Smaller binaries (critical for WASM)
- Faster execution (no vtable overhead)
- Predictable performance
- Simpler toolchain
- Matches existing codebase

**Disadvantages:**
- Takes longer to implement (~8 months more)
- More prone to memory leaks (mitigated with testing)
- More boilerplate code

**Recommendation:** Proceed with pure C implementation. The extra development time (8 months) is worth it for the performance and size benefits, especially for WASM targets.

**Next steps:**
1. Complete AS1/2 stabilization first
2. Create simple AS3 test SWF
3. Start Phase 1: ABC Parser
4. Incremental testing after each phase
