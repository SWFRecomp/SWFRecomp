# SWFRecomp vs SWFModernRuntime - Separation of Concerns

**Date:** 2025-10-31
**Purpose:** Define what belongs in each project

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                      SWFRecomp                               │
│  Static Compiler - Translates SWF → C Code                  │
│  - Parses SWF file format                                    │
│  - Extracts shapes, bitmaps, ActionScript bytecode          │
│  - Generates C source files                                  │
│  - NO runtime execution logic                                │
└──────────────────┬───────────────────────────────────────────┘
                   │
                   │ Generates C Files
                   ▼
┌──────────────────────────────────────────────────────────────┐
│             Generated C Code (Output)                        │
│  - RecompiledTags/*.c (frame functions, data arrays)         │
│  - RecompiledScripts/*.c (ActionScript execution)           │
│  - Uses API defined by SWFModernRuntime                      │
└──────────────────┬───────────────────────────────────────────┘
                   │
                   │ Links Against
                   ▼
┌──────────────────────────────────────────────────────────────┐
│                  SWFModernRuntime                            │
│  Runtime Library - Executes Generated Code                   │
│  - ActionScript operation implementations                    │
│  - Variable storage (hashmap)                                │
│  - GPU rendering (Vulkan)                                    │
│  - Window management (SDL3)                                  │
└──────────────────────────────────────────────────────────────┘
```

## Current State Analysis

### SWFModernRuntime Has

**Sophisticated Stack Implementation:**
- Stack entries are **24 bytes** (type + old_sp + size + value)
- Aligned to 8-byte boundaries
- Stack grows downward from 8MB
- Supports multiple types: STRING, F32, F64, STR_LIST

**String List Optimization:**
- `ACTION_STACK_VALUE_STR_LIST` type for chained StringAdd
- Stores **pointers to strings** rather than copying
- This is LittleCube's "unorthodox" optimization!
- Example:
  ```c
  str_list[0] = num_strings (count)
  str_list[1] = pointer to "hello"
  str_list[2] = pointer to "world"
  // Trace iterates through list and prints each
  ```

**Existing Variable Storage:**
- Already has `variables.c` and `variables.h`!
- Uses c-hashmap library (O(1) lookup)
- `ActionVar` structure exists but is **incomplete**:
  ```c
  typedef struct {
      ActionStackValueType type;
      u32 str_size;
      u64 value;  // Can be pointer or direct value
  } ActionVar;
  ```

**Problem:** `ActionVar` stores string as **pointer** (stack address), which becomes invalid!

### What We Need to Add to SWFModernRuntime

1. **String Ownership in ActionVar**
2. **Heap Allocation for Variable Strings**
3. **Cleanup/Free Functions**

## Detailed Separation

### SWFRecomp Responsibilities

**What SWFRecomp SHOULD Do:**
- ✅ Parse SWF file format
- ✅ Extract and triangulate shapes
- ✅ Decompress (zlib/LZMA)
- ✅ Translate ActionScript bytecode to C function calls
- ✅ Generate frame function code
- ✅ Generate shape data arrays
- ✅ Generate string constants
- ✅ **Generate code that CALLS runtime functions** (not implements them)

**What SWFRecomp Should NOT Do:**
- ❌ Implement ActionScript operations (that's runtime)
- ❌ Implement variable storage (that's runtime)
- ❌ Implement rendering (that's runtime)
- ❌ Define stack layout (that's runtime)
- ❌ Memory management of runtime values (that's runtime)

**Code Generation Example:**
```cpp
// SWFRecomp generates THIS:
case SWF_ACTION_SET_VARIABLE:
{
    out_script << "\t" << "// SetVariable" << endl
               << "\t" << "temp_val = getVariable((char*) STACK_SECOND_TOP_VALUE, STACK_SECOND_TOP_N);" << endl
               << "\t" << "SET_VAR(temp_val, STACK_TOP_TYPE, STACK_TOP_N, STACK_TOP_VALUE);" << endl
               << "\t" << "POP_2();" << endl;
    break;
}
```

This is **CORRECT** - it generates code using macros defined by the runtime!

### SWFModernRuntime Responsibilities

**What SWFModernRuntime SHOULD Do:**
- ✅ Define all stack macros (`PUSH`, `POP`, `STACK_TOP_VALUE`, etc.)
- ✅ Implement all action functions (`actionAdd`, `actionStringAdd`, etc.)
- ✅ Implement variable storage (hashmap)
- ✅ Manage heap allocation for variables
- ✅ Provide cleanup functions
- ✅ GPU rendering
- ✅ Window/input management

**What SWFModernRuntime Should NOT Do:**
- ❌ Parse SWF files
- ❌ Generate C code
- ❌ Know about the SWF format

## The StringAdd Variable Storage Problem

### Current Situation in SWFModernRuntime

**StringAdd creates a STR_LIST:**
```c
// actionStringAdd (action.c:626-698)
void actionStringAdd(char* stack, u32* sp, char* a_str, char* b_str) {
    // ... converts strings to STR_LIST ...
    PUSH_STR_LIST(size, num_strings);
    str_list[0] = num_strings;
    str_list[1] = pointer_to_string_b;
    str_list[2] = pointer_to_string_a;
    // ... etc ...
}
```

**The STR_LIST lives on the stack** (efficient!)

**Trace handles STR_LIST:**
```c
// actionTrace (action.c:700-742)
case ACTION_STACK_VALUE_STR_LIST:
{
    u64* str_list = (u64*) &STACK_TOP_VALUE;
    for (u64 i = 0; i < str_list[0]; ++i) {
        printf("%s", (char*) str_list[i + 1]);
    }
    printf("\n");
    break;
}
```

**Problem: SET_VAR macro just copies the pointer!**
```c
#define SET_VAR(p, t, n, v) \
    p->type = t; \
    p->str_size = n; \
    p->value = v;  // <-- Just copies stack address!
```

When stack is reused, the STR_LIST becomes invalid!

### Solution: Materialize Strings for Variables

When storing a STR_LIST to a variable, we need to:
1. **Iterate through the string list**
2. **Allocate heap memory for concatenated result**
3. **Copy all strings into heap buffer**
4. **Store heap pointer in variable**
5. **Free heap memory when variable is reassigned or destroyed**

## Implementation Plan for SWFModernRuntime

### 1. Update `ActionVar` Structure

**File:** `include/actionmodern/variables.h`

```c
typedef struct {
    ActionStackValueType type;
    u32 str_size;
    union {
        u64 numeric_value;  // For F32/F64
        struct {
            char* heap_ptr;
            bool owns_memory;
        } string_data;
    } data;
} ActionVar;
```

### 2. Add Helper Functions

**File:** `src/actionmodern/variables.c`

```c
// Materialize a STR_LIST into a heap-allocated string
char* materializeStringList(char* stack, u32 sp) {
    if (stack[sp] == ACTION_STACK_VALUE_STR_LIST) {
        u64* str_list = (u64*) &stack[sp + 16];
        u64 num_strings = str_list[0];
        u32 total_size = VAL(u32, &stack[sp + 8]);

        // Allocate heap memory
        char* result = malloc(total_size + 1);
        if (!result) return NULL;

        // Concatenate all strings
        char* dest = result;
        for (u64 i = 0; i < num_strings; i++) {
            char* src = (char*) str_list[i + 1];
            strcpy(dest, src);
            dest += strlen(src);
        }
        *dest = '\0';

        return result;
    }
    else if (stack[sp] == ACTION_STACK_VALUE_STRING) {
        // Already a single string, duplicate it
        char* src = (char*) VAL(u64, &stack[sp + 16]);
        return strdup(src);
    }

    return NULL;
}

// Set variable with proper string handling
void setVariableWithValue(ActionVar* var, char* stack, u32 sp) {
    // Free old string if exists
    if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_data.owns_memory) {
        free(var->data.string_data.heap_ptr);
    }

    ActionStackValueType type = stack[sp];

    if (type == ACTION_STACK_VALUE_STRING || type == ACTION_STACK_VALUE_STR_LIST) {
        // Materialize string to heap
        char* heap_str = materializeStringList(stack, sp);

        var->type = ACTION_STACK_VALUE_STRING;
        var->str_size = strlen(heap_str);
        var->data.string_data.heap_ptr = heap_str;
        var->data.string_data.owns_memory = true;
    }
    else {
        // Numeric types - store directly
        var->type = type;
        var->str_size = VAL(u32, &stack[sp + 8]);
        var->data.numeric_value = VAL(u64, &stack[sp + 16]);
    }
}

// Enhanced freeMap to clean up strings
void freeMap() {
    // Iterate through hashmap and free all string variables
    hashmap_iterate(var_map, free_variable_callback, NULL);
    hashmap_free(var_map);
}

static int free_variable_callback(any_t unused, any_t item) {
    ActionVar* var = (ActionVar*) item;
    if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_data.owns_memory) {
        free(var->data.string_data.heap_ptr);
    }
    free(var);
    return MAP_OK;
}
```

### 3. Update Code Generation in SWFRecomp

**The current code generation is actually CORRECT!**

The generated code:
```c
temp_val = getVariable((char*) STACK_SECOND_TOP_VALUE, STACK_SECOND_TOP_N);
SET_VAR(temp_val, STACK_TOP_TYPE, STACK_TOP_N, STACK_TOP_VALUE);
```

We just need to change `SET_VAR` macro to call a function:

**Option A: Change the macro (runtime change only)**
```c
// In action.h:
#define SET_VAR(p, t, n, v) setVariableWithValue(p, stack, *sp)
```

**Option B: Generate different code (requires SWFRecomp change)**
```c
// In SWFRecomp action.cpp:
out_script << "\t" << "setVariableWithValue(temp_val, stack, *sp);" << endl;
```

**Recommendation: Option A** - Only runtime changes needed!

## File Location Matrix

| Component | Project | File Location |
|-----------|---------|---------------|
| **ActionScript Operations** |
| `actionAdd()` | SWFModernRuntime | `src/actionmodern/action.c` |
| `actionStringAdd()` | SWFModernRuntime | `src/actionmodern/action.c` |
| `actionTrace()` | SWFModernRuntime | `src/actionmodern/action.c` |
| **Variable Storage** |
| `ActionVar` struct | SWFModernRuntime | `include/actionmodern/variables.h` |
| `getVariable()` | SWFModernRuntime | `src/actionmodern/variables.c` |
| `setVariableWithValue()` | SWFModernRuntime | `src/actionmodern/variables.c` (NEW) |
| `materializeStringList()` | SWFModernRuntime | `src/actionmodern/variables.c` (NEW) |
| `freeMap()` | SWFModernRuntime | `src/actionmodern/variables.c` (UPDATE) |
| **Stack Macros** |
| `PUSH_STR()` | SWFModernRuntime | `include/actionmodern/action.h` |
| `POP()` | SWFModernRuntime | `include/actionmodern/action.h` |
| `STACK_TOP_VALUE` | SWFModernRuntime | `include/actionmodern/action.h` |
| `SET_VAR()` | SWFModernRuntime | `include/actionmodern/action.h` (UPDATE) |
| **Code Generation** |
| SetVariable case | SWFRecomp | `src/action/action.cpp` |
| GetVariable case | SWFRecomp | `src/action/action.cpp` |
| Push case | SWFRecomp | `src/action/action.cpp` |
| **String Constants** |
| `str_N` declarations | SWFRecomp (generates) | `RecompiledScripts/script_defs.c` |

## Key Insight: SWFRecomp's Current Code Gen is Good!

Looking at the existing code generation:

```cpp
case SWF_ACTION_GET_VARIABLE:
{
    out_script << "\t" << "// GetVariable" << endl
               << "\t" << "temp_val = getVariable((char*) STACK_TOP_VALUE, STACK_TOP_N);" << endl
               << "\t" << "POP();" << endl
               << "\t" << "PUSH_VAR(temp_val);" << endl;
    break;
}

case SWF_ACTION_SET_VARIABLE:
{
    out_script << "\t" << "// SetVariable" << endl
               << "\t" << "temp_val = getVariable((char*) STACK_SECOND_TOP_VALUE, STACK_SECOND_TOP_N);" << endl
               << "\t" << "SET_VAR(temp_val, STACK_TOP_TYPE, STACK_TOP_N, STACK_TOP_VALUE);" << endl
               << "\t" << "POP_2();" << endl;
    break;
}
```

This is **EXCELLENT** because:
1. ✅ Uses runtime-defined macros (`STACK_TOP_VALUE`, etc.)
2. ✅ Calls runtime functions (`getVariable`, `PUSH_VAR`, `SET_VAR`)
3. ✅ Doesn't know about stack layout
4. ✅ Doesn't know about variable storage
5. ✅ Doesn't know about string materialization

**All we need to do is update the `SET_VAR` macro in SWFModernRuntime!**

## Changes Needed

### ✅ Keep in SWFRecomp (No Changes Needed!)

The test stub runtime changes we made were **prototyping only**.

For production:
- **DO NOT** modify `src/action/action.cpp` for variable storage
- Current code generation is already correct
- Uses runtime macros as intended

### ✅ Changes in SWFModernRuntime

**Priority 1: Essential Changes**

1. Update `ActionVar` structure with string ownership (`variables.h`)
2. Add `materializeStringList()` helper (`variables.c`)
3. Add `setVariableWithValue()` function (`variables.c`)
4. Update `SET_VAR` macro to call `setVariableWithValue()` (`action.h`)
5. Update `freeMap()` to free string memory (`variables.c`)

**Priority 2: Performance Optimizations (Later)**

1. Add type checking in `pushVar()` for better error messages
2. Add memory pool for variable strings (reduce fragmentation)
3. Add variable lifetime tracking
4. Add telemetry/debugging for variable operations

## Testing Strategy

### Test Cases (SWFModernRuntime)

1. **Basic string variable:**
   ```actionscript
   x = "hello"
   trace(x)  // "hello"
   ```

2. **StringAdd to variable:**
   ```actionscript
   result = "hello" + "world"
   trace(result)  // "helloworld"
   ```

3. **Chained StringAdd to variable:**
   ```actionscript
   result = "a" + "b" + "c" + "d"
   trace(result)  // "abcd"
   ```

4. **Variable reassignment:**
   ```actionscript
   x = "first"
   x = "second"  // Should free "first"
   trace(x)  // "second"
   ```

5. **Mixed types:**
   ```actionscript
   x = 42
   y = "hello"
   trace(x)  // "42"
   trace(y)  // "hello"
   ```

## Summary

### SWFRecomp
- **Purpose:** Static compiler (SWF → C code)
- **Changes Needed:** ✅ **NONE!** Current code gen is correct
- **Reason:** Already uses runtime-defined macros properly

### SWFModernRuntime
- **Purpose:** Runtime library (executes generated code)
- **Changes Needed:** Update variable storage for string ownership
- **Files to Modify:**
  1. `include/actionmodern/variables.h` - Add string ownership to `ActionVar`
  2. `src/actionmodern/variables.c` - Add `materializeStringList()` and `setVariableWithValue()`
  3. `include/actionmodern/action.h` - Update `SET_VAR` macro
  4. `src/actionmodern/variables.c` - Update `freeMap()` cleanup

### Test Stub Runtime
- **Purpose:** Educational/prototype only
- **Status:** Completed its purpose
- **Fate:** Can be left as-is for reference, or updated to match SWFModernRuntime

### Key Architectural Insight

The separation is clean:
- **SWFRecomp:** Knows about SWF format, generates C calls to runtime API
- **SWFModernRuntime:** Defines the runtime API, implements ActionScript semantics

This is **excellent architecture** because:
1. Runtime can change implementation without regenerating code
2. Code generator doesn't need to know runtime internals
3. Clear boundary between compile-time and runtime concerns
4. Easy to test each component independently

The current code generation already follows this perfectly!
