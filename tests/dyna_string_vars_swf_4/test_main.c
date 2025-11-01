#include "runtime/native/include/recomp.h"
#include "RecompiledScripts/out.h"

int main() {
    // Execute the script
    script_0(stack, &sp);

    // Cleanup
    freeAllVariables();

    return 0;
}
