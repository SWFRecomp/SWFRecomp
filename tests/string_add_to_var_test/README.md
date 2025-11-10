# StringAdd to Variable Test

This test demonstrates that StringAdd results can now be stored in variables.

## Test Flow

The ActionScript in this test does:

```actionscript
// Concatenate two strings
result = "hello" + "world"

// Retrieve and trace the variable
trace(result)
```

## Expected Output

```
helloworld
```

## Implementation Details

This test validates the Copy-on-Store implementation where:
1. `actionStringAdd` concatenates strings on the stack (efficient)
2. `actionSetVariable` copies the stack string to heap memory
3. `actionGetVariable` copies the heap string back to the stack
4. `actionTrace` consumes the stack string

## Memory Management

- The variable "result" owns a heap-allocated string "helloworld"
- When the program exits, `freeAllVariables()` releases the heap memory
- No memory leaks occur (verifiable with valgrind)
