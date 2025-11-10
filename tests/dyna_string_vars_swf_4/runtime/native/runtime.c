#include "recomp.h"
#include <stdio.h>
#include <stdlib.h>

// Global variables
char stack[4096];
u32 sp = 0;
int quit_swf = 0;
int manual_next_frame = 0;
int next_frame = 0;

// Variable storage
#define MAX_VARIABLES 256

typedef struct {
    char* name;
    VarValue value;
    bool in_use;
} Variable;

static Variable variables[MAX_VARIABLES];
static int num_variables = 0;

// Helper to pop a string from the stack
static char* pop_string(char* stack, u32* sp_ptr) {
    u32 len = 0;
    u32 current_sp = *sp_ptr;

    // Find the null terminator going backwards
    while (current_sp > 0 && stack[current_sp - 1] != '\0') {
        current_sp--;
        len++;
    }

    if (current_sp > 0 && stack[current_sp - 1] == '\0') {
        current_sp--; // Skip the null terminator
    }

    // Now go back to find the start
    while (current_sp > 0 && stack[current_sp - 1] != '\0') {
        current_sp--;
    }

    *sp_ptr = current_sp;
    return stack + current_sp;
}

// Helper to pop a float
static float pop_float(char* stack, u32* sp_ptr) {
    *sp_ptr -= sizeof(float);
    return *((float*)(stack + *sp_ptr));
}

// Variable management implementation
VarValue* getVariable(const char* name) {
    // Search for existing variable
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (variables[i].in_use && strcmp(variables[i].name, name) == 0) {
            return &variables[i].value;
        }
    }

    // Variable not found, create new one
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (!variables[i].in_use) {
            variables[i].name = strdup(name);
            variables[i].in_use = true;
            variables[i].value.type = ACTION_STACK_VALUE_UNSET;
            num_variables++;
            return &variables[i].value;
        }
    }

    fprintf(stderr, "ERROR: Maximum number of variables exceeded\n");
    return NULL;
}

void setVariableFloat(const char* name, float value) {
    VarValue* var = getVariable(name);
    if (!var) return;

    // Free old string if exists
    if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_value.owns_memory) {
        free(var->data.string_value.heap_ptr);
    }

    var->type = ACTION_STACK_VALUE_F32;
    var->data.f32_value = value;
}

void setVariableString(const char* name, const char* stack_str, size_t len) {
    VarValue* var = getVariable(name);
    if (!var) return;

    // Free old string if exists
    if (var->type == ACTION_STACK_VALUE_STRING && var->data.string_value.owns_memory) {
        free(var->data.string_value.heap_ptr);
    }

    // Allocate new heap string
    char* heap_str = malloc(len + 1);
    if (!heap_str) {
        fprintf(stderr, "ERROR: Failed to allocate string for variable %s\n", name);
        return;
    }

    memcpy(heap_str, stack_str, len);
    heap_str[len] = '\0';

    // Update variable
    var->type = ACTION_STACK_VALUE_STRING;
    var->data.string_value.heap_ptr = heap_str;
    var->data.string_value.length = len;
    var->data.string_value.owns_memory = true;
}

void getVariableToStack(const char* name, char* stack, u32* sp) {
    VarValue* var = getVariable(name);
    if (!var) {
        fprintf(stderr, "ERROR: Variable %s not found\n", name);
        return;
    }

    if (var->type == ACTION_STACK_VALUE_STRING) {
        size_t len = var->data.string_value.length;
        memcpy(stack + *sp, var->data.string_value.heap_ptr, len);
        *sp += len;
        stack[(*sp)++] = '\0';
    } else if (var->type == ACTION_STACK_VALUE_F32) {
        *((float*)(stack + *sp)) = var->data.f32_value;
        *sp += sizeof(float);
    }
}

void freeAllVariables() {
    for (int i = 0; i < MAX_VARIABLES; i++) {
        if (variables[i].in_use) {
            if (variables[i].value.type == ACTION_STACK_VALUE_STRING &&
                variables[i].value.data.string_value.owns_memory) {
                free(variables[i].value.data.string_value.heap_ptr);
            }
            free(variables[i].name);
            variables[i].in_use = false;
        }
    }
    num_variables = 0;
}

// Action implementations
void actionTrace(char* stack, u32* sp_ptr) {
    char* str = pop_string(stack, sp_ptr);
    printf("%s\n", str);
    fflush(stdout);
}

void actionAdd(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = a + b;
    *sp_ptr += sizeof(float);
}

void actionSubtract(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = a - b; *sp_ptr += sizeof(float);
}

void actionMultiply(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = a * b; *sp_ptr += sizeof(float);
}

void actionDivide(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = a / b; *sp_ptr += sizeof(float);
}

void actionEquals(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (a == b) ? 1.0f : 0.0f; *sp_ptr += sizeof(float);
}

void actionLess(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (a < b) ? 1.0f : 0.0f; *sp_ptr += sizeof(float);
}

void actionAnd(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (a != 0.0f && b != 0.0f) ? 1.0f : 0.0f; *sp_ptr += sizeof(float);
}

void actionOr(char* stack, u32* sp_ptr) {
    float b = pop_float(stack, sp_ptr);
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (a != 0.0f || b != 0.0f) ? 1.0f : 0.0f; *sp_ptr += sizeof(float);
}

void actionNot(char* stack, u32* sp_ptr) {
    float a = pop_float(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (a == 0.0f) ? 1.0f : 0.0f; *sp_ptr += sizeof(float);
}

void actionStringEquals(char* stack, u32* sp_ptr) {
    char* b = pop_string(stack, sp_ptr);
    char* a = pop_string(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (strcmp(a, b) == 0) ? 1.0f : 0.0f; *sp_ptr += sizeof(float);
}

void actionStringLength(char* stack, u32* sp_ptr) {
    char* str = pop_string(stack, sp_ptr);
    *((float*)(stack + *sp_ptr)) = (float)strlen(str); *sp_ptr += sizeof(float);
}

void actionStringAdd(char* stack, u32* sp_ptr) {
    char* b = pop_string(stack, sp_ptr);
    char* a = pop_string(stack, sp_ptr);
    int len_a = strlen(a);
    int len_b = strlen(b);
    memcpy(stack + *sp_ptr, a, len_a);
    memcpy(stack + *sp_ptr + len_a, b, len_b);
    *sp_ptr += len_a + len_b;
    stack[(*sp_ptr)++] = '\0';
}

void actionPop(char* stack, u32* sp_ptr) {
    // Simple pop - just decrement SP (implementation may vary)
    if (*sp_ptr > 0) (*sp_ptr)--;
}

void actionGetVariable(char* stack, u32* sp_ptr) {
    // Pop variable name from stack
    char* var_name = pop_string(stack, sp_ptr);

    // Push variable value to stack
    getVariableToStack(var_name, stack, sp_ptr);
}

void actionSetVariable(char* stack, u32* sp_ptr) {
    // Stack layout: [value] [name] <- sp
    // We need to determine the type of value

    // For now, we'll need to peek at what's on the stack
    // This is a simplified implementation - proper implementation would
    // need type information from the stack

    // Pop the value (we'll assume it's a string for now - this needs refinement)
    char* value_str = pop_string(stack, sp_ptr);
    size_t value_len = strlen(value_str);

    // Pop variable name
    char* var_name = pop_string(stack, sp_ptr);

    // Store the string variable
    setVariableString(var_name, value_str, value_len);
}

void actionGetTime(char* stack, u32* sp_ptr) {
    // Stub - would return current time
    *((float*)(stack + *sp_ptr)) = 0.0f; *sp_ptr += sizeof(float);
}

// Tag implementations
void tagShowFrame() {
    // Display the current frame
}

void tagSetBackgroundColor(u8 r, u8 g, u8 b) {
    // Set background color (ignored in console mode)
}

void tagPlaceObject2(u16 depth, u16 char_id, u32 transform_id) {
    // Place object on display list
}

void tagDefineShape(u16 shape_id, u32 start_index, u32 end_index) {
    // Define a shape
}

void defineBitmap(u32 start, u32 size, u32 w, u32 h) {
    // Define a bitmap
}

// Main entry point
void swfStart(frame_func* funcs) {
    int current_frame = 0;
    while (!quit_swf) {
        if (funcs[current_frame]) {
            funcs[current_frame]();
        }

        if (manual_next_frame) {
            current_frame = next_frame;
            manual_next_frame = 0;
        } else {
            current_frame++;
        }
    }
}
