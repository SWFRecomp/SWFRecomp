#include "recomp.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// Export for JavaScript to call
EMSCRIPTEN_KEEPALIVE
void runSWF() {
    printf("Starting SWF execution from JavaScript...\n");
    swfStart(frame_funcs);
}
#endif

int main() {
    printf("WASM SWF Runtime Loaded!\n");
    printf("This is a recompiled Flash SWF running in WebAssembly.\n\n");

#ifndef __EMSCRIPTEN__
    // Native mode - run immediately
    swfStart(frame_funcs);
#else
    // WASM mode - wait for JavaScript to call runSWF()
    printf("Call runSWF() from JavaScript to execute the SWF.\n");
#endif

    return 0;
}
