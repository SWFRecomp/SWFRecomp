# StringAdd Variable Storage - Next Steps

**Status:** Analysis Complete
**Next:** Implement in SWFModernRuntime

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

## Next Actions

1. Implement changes in SWFModernRuntime (files listed above)
2. Build and test with existing SWF files
3. Verify memory management with valgrind
4. Update documentation
5. Consider porting back to test stub runtime for consistency

---

**Key Takeaway:** The code generation in SWFRecomp is already correct. We only need to update SWFModernRuntime's variable storage to properly handle string ownership.
