# StringAdd Variable Storage Implementation Plan

**Date:** 2025-10-31
**Status:** Design Document
**Related Issue:** StringAdd results cannot be stored in variables

## Problem Statement

The current `actionStringAdd` implementation uses an efficient stack-based optimization where strings are concatenated directly on the stack. This works perfectly for immediate consumption (e.g., passing to `trace`), but breaks when the concatenated string needs to be stored in a variable.

### Current Behavior

```c
// runtime.c:114-123
void actionStringAdd(char* stack, u32* sp_ptr) {
    char* b = pop_string(stack, sp_ptr);  // Pop second string
    char* a = pop_string(stack, sp_ptr);  // Pop first string
    int len_a = strlen(a);
    int len_b = strlen(b);
    memcpy(stack + *sp_ptr, a, len_a);           // Copy a to stack
    memcpy(stack + *sp_ptr + len_a, b, len_b);   // Copy b after a
    *sp_ptr += len_a + len_b;
    stack[(*sp_ptr)++] = '\0';                   // Null terminate
}
```

**Why this optimization is good:**
- Zero heap allocations
- Cache-friendly (stack is hot)
- Multiple chained StringAdds don't repeatedly copy prefixes
- Fast and simple

**Why this breaks with variables:**
- Stack values are ephemeral (get overwritten by subsequent operations)
- Variables need persistent storage across operations and frames
- `SetVariable` currently has no mechanism to preserve stack-based strings

## Requirements

1. **Preserve the existing optimization** for non-variable usage (direct consumption)
2. **Support storing StringAdd results in variables** with proper lifetime management
3. **Minimize memory overhead** - only allocate when necessary
4. **Prevent memory leaks** - ensure all allocated strings are freed
5. **Maintain compatibility** with existing generated code where possible

## Proposed Solutions

### Option A: Copy-on-Store (Recommended)

**Complexity:** Low
**Memory Overhead:** Medium
**Performance:** Good
**Implementation Time:** 2-3 days

#### Description

Allocate heap memory only when storing a string to a variable. Keep the current stack-based optimization for all other cases.

#### Architecture

```
┌─────────────────────────────────────────────────────────┐
│  StringAdd: "hello" + "world"                           │
│  → Result on stack: "helloworld\0"                      │
└──────────────────┬──────────────────────────────────────┘
                   │
         ┌─────────┴──────────┐
         ▼                    ▼
  ┌─────────────┐      ┌────────────────┐
  │   Trace     │      │  SetVariable   │
  │  (direct)   │      │   (storage)    │
  └──────┬──────┘      └────────┬───────┘
         │                      │
         ▼                      ▼
  Uses stack ptr      Allocates heap copy
  (efficient)         and stores in var
```

#### Data Structures

**Enhanced Variable Structure:**

```c
// New variable value structure
typedef struct {
    ActionStackValueType type;    // STRING, F32, etc.
    union {
        float f32_value;
        struct {
            char* heap_ptr;       // Heap-allocated string
            size_t length;        // String length (optimization)
            bool owns_memory;     // True if we need to free it
        } string_value;
        u64 raw_value;            // For other types
    } data;
} VarValue;

// Variable entry in hashmap
typedef struct {
    char* name;                   // Variable name
    VarValue value;               // Variable value
} Variable;
```

#### Implementation Steps

##### 1. Runtime Changes (SWFModernRuntime)

**File: `src/actionmodern/variables.c`**

```c
// New function: Store string variable with heap allocation
void setVariableString(const char* var_name, const char* stack_str, size_t len) {
    Variable* var = getOrCreateVariable(var_name);

    // Free existing string if owned
    if (var->value.type == ACTION_STACK_VALUE_STRING &&
        var->value.data.string_value.owns_memory) {
        free(var->value.data.string_value.heap_ptr);
    }

    // Allocate new heap string
    char* heap_str = malloc(len + 1);
    if (!heap_str) {
        // Handle allocation failure
        fprintf(stderr, "ERROR: Failed to allocate string for variable %s\n", var_name);
        return;
    }

    memcpy(heap_str, stack_str, len);
    heap_str[len] = '\0';

    // Update variable
    var->value.type = ACTION_STACK_VALUE_STRING;
    var->value.data.string_value.heap_ptr = heap_str;
    var->value.data.string_value.length = len;
    var->value.data.string_value.owns_memory = true;
}

// Enhanced: Get string variable and copy to stack
void getVariableString(const char* var_name, char* stack, u32* sp_ptr) {
    Variable* var = findVariable(var_name);
    if (!var || var->value.type != ACTION_STACK_VALUE_STRING) {
        // Handle error: variable not found or wrong type
        return;
    }

    size_t len = var->value.data.string_value.length;
    memcpy(stack + *sp_ptr, var->value.data.string_value.heap_ptr, len);
    *sp_ptr += len;
    stack[(*sp_ptr)++] = '\0';
}

// New: Free all variable strings on cleanup
void freeAllVariables() {
    // Iterate through all variables in hashmap
    // For each string variable with owns_memory=true, free the heap_ptr
    // Then clear the hashmap
}
```

**File: `include/actionmodern/variables.h`**

```c
// Add new function declarations
void setVariableString(const char* var_name, const char* stack_str, size_t len);
void getVariableString(const char* var_name, char* stack, u32* sp_ptr);
void freeAllVariables();
```

**File: `src/libswf/swf.c`**

```c
// Add cleanup call
void swfCleanup() {
    freeAllVariables();
    // ... other cleanup
}
```

##### 2. Code Generation Changes (SWFRecomp)

**File: `src/action/action.cpp`**

Update the `SWF_ACTION_SET_VARIABLE` case:

```cpp
case SWF_ACTION_SET_VARIABLE:
{
    out_script << "\t" << "// SetVariable" << endl
               << "\t" << "temp_val = getVariable((char*) STACK_TOP_VALUE, STACK_TOP_N);" << endl;

    // Check if storing a string - needs special handling
    out_script << "\t" << "if (STACK_TOP_TYPE == ACTION_STACK_VALUE_STRING) {" << endl
               << "\t" << "\t" << "// Allocate heap storage for string variable" << endl
               << "\t" << "\t" << "setVariableString((char*) STACK_SECOND_TOP_VALUE, "
               << "(char*) STACK_TOP_VALUE, STACK_TOP_N);" << endl
               << "\t" << "} else {" << endl
               << "\t" << "\t" << "// Non-string types use existing mechanism" << endl
               << "\t" << "\t" << "SET_VAR(temp_val, STACK_TOP_TYPE, STACK_TOP_N, STACK_TOP_VALUE);" << endl
               << "\t" << "}" << endl;

    out_script << "\t" << "POP_2();" << endl;

    break;
}
```

Update the `SWF_ACTION_GET_VARIABLE` case:

```cpp
case SWF_ACTION_GET_VARIABLE:
{
    out_script << "\t" << "// GetVariable" << endl
               << "\t" << "temp_val = getVariable((char*) STACK_TOP_VALUE, STACK_TOP_N);" << endl
               << "\t" << "POP();" << endl;

    // Check if retrieving a string variable
    out_script << "\t" << "if (temp_val.type == ACTION_STACK_VALUE_STRING) {" << endl
               << "\t" << "\t" << "// Copy heap string to stack" << endl
               << "\t" << "\t" << "getVariableString((char*) temp_val.value, stack, sp);" << endl
               << "\t" << "} else {" << endl
               << "\t" << "\t" << "// Non-string types use existing mechanism" << endl
               << "\t" << "\t" << "PUSH_VAR(temp_val);" << endl
               << "\t" << "}" << endl;

    break;
}
```

##### 3. Testing

**Test Cases to Create:**

1. **Basic string variable storage:**
   ```actionscript
   var = "hello" add "world"
   trace(var)
   // Expected: "helloworld"
   ```

2. **String variable reassignment:**
   ```actionscript
   var = "first"
   var = "second"  // Should free "first"
   trace(var)
   // Expected: "second"
   ```

3. **Multiple string variables:**
   ```actionscript
   var1 = "hello"
   var2 = "world"
   var3 = var1 add var2
   trace(var3)
   // Expected: "helloworld"
   ```

4. **String variable across frames:**
   ```actionscript
   // Frame 1
   var = "persistent"

   // Frame 2
   trace(var)
   // Expected: "persistent"
   ```

5. **Chained StringAdd to variable:**
   ```actionscript
   var = "a" add "b" add "c" add "d"
   trace(var)
   // Expected: "abcd"
   ```

#### Memory Management Strategy

**Allocation Points:**
- Only in `setVariableString()` when storing to a variable
- Never in `actionStringAdd()` (keeps optimization)

**Deallocation Points:**
1. Variable reassignment (free old value if string)
2. Frame cleanup (optional - for temporary variables)
3. Application exit (`freeAllVariables()`)

**Memory Leak Prevention:**
- Always check `owns_memory` before freeing
- Free old value before allocating new one in reassignment
- Register cleanup function with runtime

#### Performance Characteristics

| Operation | Current | With Variables | Impact |
|-----------|---------|----------------|--------|
| StringAdd → Trace | Stack only | Stack only | None |
| StringAdd → SetVariable | N/A (broken) | malloc + memcpy | New feature |
| GetVariable (string) | N/A | memcpy to stack | New feature |
| Variable reassignment | N/A | free + malloc | New feature |

**Key insight:** This approach has **zero performance impact** on the existing StringAdd optimization for non-variable usage.

---

### Option B: String Pool

**Complexity:** Medium
**Memory Overhead:** Low
**Performance:** Better
**Implementation Time:** 4-5 days

#### Description

Use an arena allocator to manage all variable strings in a pool. Reduces fragmentation and simplifies cleanup.

#### Architecture

```c
// String pool structure
typedef struct {
    char* buffer;           // Large pre-allocated buffer
    size_t capacity;        // Total buffer size
    size_t used;            // Bytes currently used
    size_t num_strings;     // Number of strings allocated
} StringPool;

StringPool global_string_pool;
```

#### Advantages
- Less fragmentation than individual malloc/free
- Faster allocation (bump allocator)
- Simpler cleanup (free entire pool at once)
- Better cache locality

#### Disadvantages
- More complex implementation
- Need to handle pool growth/overflow
- Can't free individual strings
- May waste memory if pool is too large

#### When to Use
- If memory fragmentation becomes an issue
- If variable reassignment is very common
- If you want better performance characteristics

---

### Option C: Reference Counting

**Complexity:** High
**Memory Overhead:** Lowest
**Performance:** Best (no duplicate strings)
**Implementation Time:** 7-10 days

#### Description

Use reference counting to share string data between variables.

#### Architecture

```c
typedef struct {
    char* data;
    size_t length;
    int ref_count;
} RefCountedString;
```

#### Advantages
- Multiple variables can share the same string
- No duplicate allocations for copied variables
- Optimal memory usage

#### Disadvantages
- Much more complex implementation
- Easy to introduce bugs (ref count errors)
- Need to handle circular references (unlikely but possible)
- Overkill for most use cases

#### When to Use
- If memory is extremely constrained
- If variable copying is very common
- After profiling shows this is necessary

---

## Recommended Implementation Plan

### Phase 1: Foundation (Week 1)
1. Implement basic `VarValue` structure with string support
2. Add `setVariableString()` and `getVariableString()` functions
3. Add memory cleanup in `freeAllVariables()`
4. Write unit tests for string allocation/deallocation

### Phase 2: Code Generation (Week 1-2)
1. Update `SWF_ACTION_SET_VARIABLE` code generation
2. Update `SWF_ACTION_GET_VARIABLE` code generation
3. Ensure backward compatibility with non-string variables

### Phase 3: Testing (Week 2)
1. Create test SWFs with string variables
2. Test basic storage and retrieval
3. Test reassignment and memory cleanup
4. Test cross-frame persistence
5. Test chained StringAdd → variable

### Phase 4: Integration (Week 2-3)
1. Test with SWFModernRuntime
2. Verify no memory leaks (valgrind)
3. Performance testing (ensure no regression)
4. Update documentation

### Phase 5: Optimization (Week 3+)
1. Profile memory usage
2. Consider string pool if needed
3. Optimize allocation patterns
4. Add telemetry/debugging support

## Testing Strategy

### Unit Tests
- String allocation/deallocation
- Variable reassignment
- Memory leak detection (valgrind)
- Edge cases (empty strings, very long strings)

### Integration Tests
- SWF with string variables
- Multi-frame string persistence
- Complex StringAdd chains
- Mixed string and numeric variables

### Performance Tests
- Benchmark StringAdd (ensure no regression)
- Memory usage profiling
- Stress test (many variables)

## Risk Analysis

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Memory leaks | Medium | High | Thorough testing with valgrind |
| Performance regression | Low | Medium | Keep existing optimization for non-variables |
| API breaking changes | Low | Medium | Careful versioning and testing |
| String lifetime bugs | Medium | High | Clear ownership semantics, good tests |

## Success Criteria

1. ✅ StringAdd results can be stored in variables
2. ✅ Variable strings persist correctly across operations
3. ✅ No memory leaks detected by valgrind
4. ✅ No performance regression for non-variable StringAdd
5. ✅ All existing tests still pass
6. ✅ New string variable tests pass

## Alternative Approaches Considered

### 1. Always Allocate (Rejected)
Change StringAdd to always allocate on heap.

**Why rejected:** Destroys the existing optimization for the common case (StringAdd → Trace).

### 2. Lookahead Optimization (Future Work)
Detect at code generation time whether a StringAdd result will be stored in a variable.

**Why deferred:** More complex, can be added later as optimization.

### 3. Copy-on-Write (Rejected)
Share string data until modification.

**Why rejected:** Strings are immutable in ActionScript, so no benefit over ref counting.

## Open Questions

1. **Should we pool strings?** Start with malloc/free, optimize later if needed.
2. **How to handle very long strings?** Set a maximum size limit (e.g., 64KB)?
3. **Should we clear variables between frames?** Depends on ActionScript semantics.
4. **How to debug string lifetime issues?** Add debug logging for allocations?

## References

- Current StringAdd implementation: `tests/trace_swf_4/runtime/native/runtime.c:114-123`
- Variable declaration: `src/action/action.cpp:348-356`
- SetVariable code gen: `src/action/action.cpp:223-230`
- GetVariable code gen: `src/action/action.cpp:213-220`
- SWFModernRuntime variable storage: `src/actionmodern/variables.c` (future)

## Conclusion

**Recommended approach:** Option A (Copy-on-Store)

This approach:
- Preserves the existing optimization for non-variable usage
- Adds minimal complexity
- Provides clear ownership semantics
- Can be implemented incrementally
- Leaves room for future optimizations (string pool, etc.)

The key insight is that **variable storage is the minority case** - most StringAdd operations go directly to Trace or other immediate consumers, where the stack-based optimization remains fully effective.
