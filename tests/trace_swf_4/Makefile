CC = gcc
CFLAGS = -Wall -Wno-unused-variable -Iinclude -std=c17
BUILD_DIR = build/native
RUNTIME_DIR = runtime/native

# Marker file to track if setup has been done
SETUP_MARKER = $(BUILD_DIR)/.setup_done

# Source files (in build directory after copying)
SRCS = $(BUILD_DIR)/main.c \
       $(BUILD_DIR)/runtime.c \
       $(BUILD_DIR)/tagMain.c \
       $(BUILD_DIR)/constants.c \
       $(BUILD_DIR)/draws.c \
       $(BUILD_DIR)/script_0.c \
       $(BUILD_DIR)/script_defs.c

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = $(BUILD_DIR)/TestSWFRecompiled

.PHONY: all clean run

all: $(TARGET)

# Setup: copy all source files to build directory
$(SETUP_MARKER):
	@echo "Setting up build directory..."
	@mkdir -p $(BUILD_DIR)/include
	@# Copy runtime files
	@cp $(RUNTIME_DIR)/main.c $(BUILD_DIR)/
	@cp $(RUNTIME_DIR)/runtime.c $(BUILD_DIR)/
	@cp $(RUNTIME_DIR)/include/*.h $(BUILD_DIR)/include/
	@# Copy generated files
	@cp RecompiledScripts/*.c $(BUILD_DIR)/ 2>/dev/null || true
	@cp RecompiledScripts/*.h $(BUILD_DIR)/ 2>/dev/null || true
	@cp RecompiledTags/*.c $(BUILD_DIR)/ 2>/dev/null || true
	@cp RecompiledTags/*.h $(BUILD_DIR)/ 2>/dev/null || true
	@touch $(SETUP_MARKER)
	@echo "✓ Build directory prepared"

$(OBJS): $(SETUP_MARKER)

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $^
	@echo "✅ Build complete!"

$(BUILD_DIR)/%.o: $(BUILD_DIR)/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -I$(BUILD_DIR)/include -I$(BUILD_DIR) -c -o $@ $<

clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

run: $(TARGET)
	@echo "Running $(TARGET)..."
	@./$(TARGET)
