# StringAdd Variable Storage - Next Steps

**Status:** Implementation Complete ✅
**Completed:** 2025-11-01

## Key Discovery

**The SWFRecomp code generation is already correct!** ✅

Looking at the existing code:
```cpp
// SWFRecomp generates:
temp_val = getVariable((char*) STACK_SECOND_TOP_VALUE, STACK_SECOND_TOP_N);
SET_VAR(temp_val, STACK_TOP_TYPE, STACK_TOP_N, STACK_TOP_VALUE);
POP_2();
```

This uses runtime-defined macros - **exactly as it should!**

## The Real Problem

**SWFModernRuntime's `SET_VAR` macro is naive:**
```c
// Current implementation just copies pointer:
#define SET_VAR(p, t, n, v) \
    p->type = t; \
    p->str_size = n; \
    p->value = v;  // <-- Stack address becomes invalid!
```

When the value is a `STR_LIST` (LittleCube's optimization), the list lives on the stack and gets overwritten!

## Solution: Materialize Strings for Variables

When `SET_VAR` is called with a string/string-list:
1. Detect it's a string type
2. Materialize the STR_LIST into a single heap-allocated string
3. Store the heap pointer in the variable
4. Mark variable as owning the memory
5. Free on reassignment/cleanup

## Implementation Location

### ❌ NOT in SWFRecomp
- Code generation is already correct
- No changes needed!

### ✅ In SWFModernRuntime

**Files to Modify:**

1. `include/actionmodern/variables.h`
   - Update `ActionVar` structure with string ownership

2. `src/actionmodern/variables.c`
   - Add `materializeStringList()` - concatenates STR_LIST to heap string
   - Add `setVariableWithValue()` - smart variable setter
   - Update `freeMap()` - free heap strings on cleanup

3. `include/actionmodern/action.h`
   - Update `SET_VAR` macro to call `setVariableWithValue()`

## Code Changes Required

### 1. Update ActionVar Structure

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

### 2. Add String Materialization

**File:** `src/actionmodern/variables.c`

```c
char* materializeStringList(char* stack, u32 sp) {
    if (stack[sp] == ACTION_STACK_VALUE_STR_LIST) {
        // Get the string list
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
        // Single string - duplicate it
        char* src = (char*) VAL(u64, &stack[sp + 16]);
        return strdup(src);
    }

    return NULL;
}

void setVariableWithValue(ActionVar* var, char* stack, u32 sp) {
    // Free old string if exists
    if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_data.owns_memory) {
        free(var->data.string_data.heap_ptr);
    }

    ActionStackValueType type = stack[sp];

    if (type == ACTION_STACK_VALUE_STRING || type == ACTION_STACK_VALUE_STR_LIST) {
        // Materialize string to heap
        char* heap_str = materializeStringList(stack, sp);
        if (!heap_str) {
            // Handle allocation failure
            return;
        }

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
```

### 3. Update SET_VAR Macro

**File:** `include/actionmodern/action.h`

```c
// OLD:
#define SET_VAR(p, t, n, v) \
    p->type = t; \
    p->str_size = n; \
    p->value = v;

// NEW:
#define SET_VAR(p, t, n, v) setVariableWithValue(p, stack, *sp)
```

### 4. Update Cleanup

**File:** `src/actionmodern/variables.c`

```c
static int free_variable_callback(any_t unused, any_t item) {
    ActionVar* var = (ActionVar*) item;
    if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_data.owns_memory) {
        free(var->data.string_data.heap_ptr);
    }
    free(var);
    return MAP_OK;
}

void freeMap() {
    if (var_map) {
        hashmap_iterate(var_map, free_variable_callback, NULL);
        hashmap_free(var_map);
    }
}
```

### 5. Update pushVar for String Variables

**File:** `src/actionmodern/action.c`

```c
void pushVar(char* stack, u32* sp, ActionVar* var) {
    switch (var->type) {
        case ACTION_STACK_VALUE_F32:
        case ACTION_STACK_VALUE_F64:
        {
            PUSH(var->type, var->data.numeric_value);
            break;
        }

        case ACTION_STACK_VALUE_STRING:
        {
            // Use heap pointer if variable owns memory
            char* str_ptr = var->data.string_data.owns_memory ?
                var->data.string_data.heap_ptr :
                (char*) var->data.numeric_value;  // Stack address (legacy)

            PUSH_STR(str_ptr, var->str_size);
            break;
        }
    }
}
```

## Testing Plan

1. **Test with existing SWF files**
   - `dyna_string_vars_swf_4` should work
   - String concatenation tests should work

2. **Create new test cases**
   - StringAdd → variable → trace
   - Multiple variables
   - Variable reassignment
   - Mixed numeric and string variables

3. **Memory leak testing**
   - Run valgrind to verify no leaks
   - Test variable reassignment (should free old value)

4. **Performance testing**
   - Verify no regression for non-variable StringAdd
   - STR_LIST optimization still works for direct trace

## Benefits of This Approach

✅ **No SWFRecomp changes needed** - code generation already correct
✅ **Preserves STR_LIST optimization** - only materializes for variables
✅ **Clean separation** - runtime handles runtime concerns
✅ **Backward compatible** - existing generated code still works
✅ **Type-safe** - proper handling of all value types

## Estimated Implementation Time

- **1-2 hours:** Core implementation (materializeStringList, setVariableWithValue)
- **30 min:** Update ActionVar structure
- **30 min:** Update cleanup code
- **30 min:** Update pushVar
- **1 hour:** Testing and debugging

**Total: 3-4 hours**

## What We Learned

1. **SWFModernRuntime already has sophisticated stack handling**
   - 24-byte stack entries
   - Multiple types (F32, F64, STRING, STR_LIST)
   - Aligned memory access

2. **STR_LIST is LittleCube's string optimization**
   - Stores pointers, not copies
   - Works great for immediate consumption (trace)
   - Breaks when stored to variables (pointer becomes invalid)

3. **The architecture is excellent**
   - Clear separation between compiler and runtime
   - Runtime owns the API (macros, functions)
   - Compiler just generates calls to runtime API

4. **The fix is runtime-only**
   - No code generation changes needed
   - Just update the runtime to materialize strings for variables
   - Preserves optimization for non-variable case

## Implementation Summary

### ✅ Completed (2025-11-01)

All core implementation tasks have been completed:

1. **Updated ActionVar Structure** (`include/actionmodern/variables.h`)
   - Added union with `numeric_value` and `string_data` fields
   - Added `owns_memory` flag for heap-allocated strings

2. **Implemented String Materialization** (`src/actionmodern/variables.c`)
   - `materializeStringList()` - Concatenates STR_LIST to heap string
   - `setVariableWithValue()` - Smart variable setter with proper memory management
   - `free_variable_callback()` - Updated for new hashmap API and string cleanup

3. **Updated SET_VAR Macro** (`include/actionmodern/action.h`)
   - Now calls `setVariableWithValue()` instead of naive assignment

4. **Updated pushVar Function** (`src/actionmodern/action.c`)
   - Handles heap-allocated strings correctly
   - Fixed all references from old `value` field to `data.numeric_value`

5. **Built Successfully**
   - SWFModernRuntime library compiled without errors
   - All compilation issues resolved (hashmap API updates, ActionVar structure changes)

### ✅ Valgrind Testing (2025-11-01)

Ran valgrind on the dyna_string_vars_swf_4 test:
```
==2460211== HEAP SUMMARY:
==2460211==     in use at exit: 0 bytes in 0 blocks
==2460211==   total heap usage: 5 allocs, 5 frees, 4,137 bytes allocated
==2460211==
==2460211== All heap blocks were freed -- no leaks are possible
==2460211==
==2460211== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

**Result:** ✅ No memory leaks detected

The test exercises:
- Variable assignment
- Variable reassignment (tests proper freeing of old values)
- Variable retrieval

**Note:** This test uses the local stub runtime (`runtime/native/runtime.c`), not the full SWFModernRuntime. The stub runtime has simpler variable handling but still demonstrates proper memory management for basic string variables.

### ✅ Comprehensive C Unit Test Suite (2025-11-01)

Created `test_variables_simple.c` - a comprehensive test suite for the string variable implementation.

**Test Coverage:**
- ✅ Basic string variable storage
- ✅ STR_LIST materialization to heap strings
- ✅ Variable reassignment (memory leak test)
- ✅ Mixed type variables (numeric + string)
- ✅ Multiple independent variables
- ✅ Empty string handling
- ✅ Long string handling (1023 bytes)
- ✅ materializeStringList() function directly
- ✅ STR_LIST with many strings (10 parts)

**Results:**
```
Total:  24 tests
Passed: 24 tests
Failed: 0 tests
```

**Valgrind Results:**
```
HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 28 allocs, 28 frees, 8,394,927 bytes allocated

All heap blocks were freed -- no leaks are possible

ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

**Build:**
```bash
cd SWFModernRuntime
make -f Makefile.test_simple        # Build
make -f Makefile.test_simple test   # Run tests
make -f Makefile.test_simple valgrind  # Run with valgrind
```

**Bug Fixed:** During testing, discovered uninitialized `owns_memory` field in newly created variables. Fixed by properly initializing the struct fields in `getVariable()`.

### Remaining Tasks

1. **Performance testing**
   - Verify no regression for non-variable StringAdd
   - STR_LIST optimization still works for direct trace

4. **Consider porting to test stub runtime**
   - For consistency across runtimes

## Files Modified

All changes were made in **SWFModernRuntime** (not SWFRecomp):

- `include/actionmodern/variables.h`
- `src/actionmodern/variables.c`
- `include/actionmodern/action.h`
- `src/actionmodern/action.c`

## Next Actions

1. Install valgrind and verify memory management
2. Create comprehensive test suite
3. Update any documentation references
4. Consider porting to other runtimes if applicable

---

**Key Takeaway:** The code generation in SWFRecomp is already correct. We only need to update SWFModernRuntime's variable storage to properly handle string ownership.
