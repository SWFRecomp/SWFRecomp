#include <stdio.h>
#include <emscripten.h>

// Function that will be called from JavaScript
EMSCRIPTEN_KEEPALIVE
void sayHello() {
    printf("Hello from WebAssembly!\n");
}

// Function with a parameter
EMSCRIPTEN_KEEPALIVE
int add(int a, int b) {
    printf("C function called: %d + %d = %d\n", a, b, a + b);
    return a + b;
}

int main() {
    printf("WASM module loaded successfully!\n");
    printf("Call sayHello() or add(a, b) from JavaScript.\n");
    return 0;
}
