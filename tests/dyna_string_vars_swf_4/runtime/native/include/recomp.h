#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Type definitions
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// ActionScript value types
typedef enum {
    ACTION_STACK_VALUE_STRING = 0,
    ACTION_STACK_VALUE_F32 = 1,
    ACTION_STACK_VALUE_UNSET = 15
} ActionStackValueType;

// Variable value structure
typedef struct {
    ActionStackValueType type;
    union {
        float f32_value;
        struct {
            char* heap_ptr;
            size_t length;
            bool owns_memory;
        } string_value;
        u64 raw_value;
    } data;
} VarValue;

// Global variables for SWF execution
extern char stack[4096];
extern u32 sp;
extern int quit_swf;
extern int manual_next_frame;
extern int next_frame;

// Frame function pointer type
typedef void (*frame_func)();
extern frame_func frame_funcs[];

// String definitions
extern char* str_0;

// Push macros
#define PUSH_STR(str, len) do { \
    memcpy(stack + (*sp), str, len); \
    (*sp) += len; \
    stack[(*sp)++] = '\0'; \
} while(0)

#define PUSH_FLOAT(val) do { \
    *((float*)(stack + (*sp))) = val; \
    (*sp) += sizeof(float); \
} while(0)

#define PUSH_U32(val) do { \
    *((u32*)(stack + (*sp))) = val; \
    (*sp) += sizeof(u32); \
} while(0)

// Variable management
VarValue* getVariable(const char* name);
void setVariableFloat(const char* name, float value);
void setVariableString(const char* name, const char* stack_str, size_t len);
void getVariableToStack(const char* name, char* stack, u32* sp);
void freeAllVariables();

// Action function stubs
void actionTrace(char* stack, u32* sp);
void actionAdd(char* stack, u32* sp);
void actionSubtract(char* stack, u32* sp);
void actionMultiply(char* stack, u32* sp);
void actionDivide(char* stack, u32* sp);
void actionEquals(char* stack, u32* sp);
void actionLess(char* stack, u32* sp);
void actionAnd(char* stack, u32* sp);
void actionOr(char* stack, u32* sp);
void actionNot(char* stack, u32* sp);
void actionStringEquals(char* stack, u32* sp);
void actionStringLength(char* stack, u32* sp);
void actionStringAdd(char* stack, u32* sp);
void actionPop(char* stack, u32* sp);
void actionGetVariable(char* stack, u32* sp);
void actionSetVariable(char* stack, u32* sp);
void actionGetTime(char* stack, u32* sp);

// Tag function stubs
void tagShowFrame();
void tagSetBackgroundColor(u8 r, u8 g, u8 b);
void tagPlaceObject2(u16 depth, u16 char_id, u32 transform_id);
void tagDefineShape(u16 shape_id, u32 start_index, u32 end_index);
void defineBitmap(u32 start, u32 size, u32 w, u32 h);

// Main entry point
void swfStart(frame_func* funcs);
