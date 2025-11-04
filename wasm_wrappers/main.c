#include <recomp.h>
#include <swf.h>

// Create SWFAppContext
// In NO_GRAPHICS mode, only frame_funcs is needed
// In graphics mode, all fields are required
static SWFAppContext app_context = {
    .frame_funcs = NULL  // Will be set in main()
#ifndef NO_GRAPHICS
    ,
    .width = 800,
    .height = 600,
    .stage_to_ndc = NULL,
    .bitmap_count = 0,
    .bitmap_highest_w = 0,
    .bitmap_highest_h = 0,
    .shape_data = NULL,
    .shape_data_size = 0,
    .transform_data = NULL,
    .transform_data_size = 0,
    .color_data = NULL,
    .color_data_size = 0,
    .uninv_mat_data = NULL,
    .uninv_mat_data_size = 0,
    .gradient_data = NULL,
    .gradient_data_size = 0,
    .bitmap_data = NULL,
    .bitmap_data_size = 0
#endif
};

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Export for JavaScript to call
EMSCRIPTEN_KEEPALIVE
void runSWF() {
    printf("Starting SWF execution from JavaScript...\n");
    swfStart(&app_context);
}
#endif

int main() {
    printf("WASM SWF Runtime Loaded!\n");
    printf("This is a recompiled Flash SWF running in WebAssembly.\n\n");

    // Initialize app context with frame functions
    app_context.frame_funcs = frame_funcs;

#ifndef __EMSCRIPTEN__
    // Native mode - run immediately
    swfStart(&app_context);
#else
    // WASM mode - initialize and wait for JavaScript to call runSWF()
    initTime();  // Initialize SWFModernRuntime timer
    printf("Call runSWF() from JavaScript to execute the SWF.\n");
#endif

    return 0;
}
