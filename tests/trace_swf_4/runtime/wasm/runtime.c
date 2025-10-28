#include "recomp.h"

// Global variables
char stack[4096];
u32 sp = 0;
int quit_swf = 0;
int manual_next_frame = 0;
int next_frame = 0;

// Helper function to pop string from stack
static char* pop_string(char* stack, u32* sp_ptr) {
    // Find the null terminator
    u32 current = *sp_ptr;
    while (current > 0 && stack[current - 1] != '\0') {
        current--;
    }

    // Move past the null terminator
    if (current > 0) {
        current--;
    }

    // Find the start of the string
    u32 start = current;
    while (start > 0 && stack[start - 1] != '\0') {
        start--;
    }

    *sp_ptr = start;
    return &stack[start];
}

// ActionScript functions
void actionTrace(char* stack, u32* sp_ptr) {
    char* str = pop_string(stack, sp_ptr);
    printf("%s\n", str);
}

// Tag functions (stubs - no rendering needed)
void tagSetBackgroundColor(u8 r, u8 g, u8 b) {
    printf("[Tag] SetBackgroundColor(%d, %d, %d)\n", r, g, b);
}

void tagShowFrame() {
    printf("[Tag] ShowFrame()\n");
}

// tagInit() is defined in tagMain.c

// Runtime entry point
void swfStart(frame_func* funcs) {
    printf("=== SWF Execution Started ===\n");

    tagInit();

    int current_frame = 0;
    while (!quit_swf && current_frame < 100) { // Safety limit
        printf("\n[Frame %d]\n", current_frame);

        if (funcs[current_frame]) {
            funcs[current_frame]();
        } else {
            printf("No function for frame %d, stopping.\n", current_frame);
            break;
        }

        if (manual_next_frame) {
            current_frame = next_frame;
            manual_next_frame = 0;
        } else {
            current_frame++;
        }
    }

    printf("\n=== SWF Execution Completed ===\n");
}
