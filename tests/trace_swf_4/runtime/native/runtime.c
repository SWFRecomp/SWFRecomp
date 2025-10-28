#include "recomp.h"
#include <stdio.h>
#include <stdlib.h>

// Global variables
char stack[4096];
u32 sp = 0;
int quit_swf = 0;
int manual_next_frame = 0;
int next_frame = 0;

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
    // Stub - would need variable storage
    printf("GetVariable not implemented\n");
}

void actionSetVariable(char* stack, u32* sp_ptr) {
    // Stub - would need variable storage
    printf("SetVariable not implemented\n");
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
