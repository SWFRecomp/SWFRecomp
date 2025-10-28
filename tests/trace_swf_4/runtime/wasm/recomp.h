#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Type definitions
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

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

// Action function declarations
void actionTrace(char* stack, u32* sp);

// Tag function declarations (stubs for non-graphics tests)
void tagSetBackgroundColor(u8 r, u8 g, u8 b);
void tagShowFrame();
void tagInit();

// Runtime function
void swfStart(frame_func* funcs);
