# ==============================================================================
# Layer 1 (Mapping) Makefile
# ==============================================================================

# Compiler and Flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

# Directories
LAYER1_DIR := .
COMMON_DIR := ../common
BUILD_DIR := build

# Include paths (flat structure)
INCLUDES := -I$(LAYER1_DIR)/include -I$(COMMON_DIR)/include

# Source files
LAYER1_SOURCES := $(wildcard $(LAYER1_DIR)/src/*.cc)
COMMON_SOURCES := $(wildcard $(COMMON_DIR)/src/*.cc)
MAIN_SOURCE := main.cc

# Object files (all go into build/)
LAYER1_OBJECTS := $(patsubst $(LAYER1_DIR)/src/%.cc,$(BUILD_DIR)/%.o,$(LAYER1_SOURCES))
COMMON_OBJECTS := $(patsubst $(COMMON_DIR)/src/%.cc,$(BUILD_DIR)/common_%.o,$(COMMON_SOURCES))
MAIN_OBJECT := $(BUILD_DIR)/main.o

ALL_OBJECTS := $(LAYER1_OBJECTS) $(COMMON_OBJECTS) $(MAIN_OBJECT)

# Target executable (inside build directory)
TARGET := test_layer1

# ==============================================================================
# Rules
# ==============================================================================

.PHONY: all clean run

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link all objects into executable (output in build/)
$(BUILD_DIR)/$(TARGET): $(ALL_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -pthread

# Compile layer1 source files
$(BUILD_DIR)/%.o: $(LAYER1_DIR)/src/%.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile common source files (with prefix to avoid name collision)
$(BUILD_DIR)/common_%.o: $(COMMON_DIR)/src/%.cc | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile main.cc
$(BUILD_DIR)/main.o: $(MAIN_SOURCE) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the test executable
run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# ==============================================================================
# Debug: Print variables
# ==============================================================================
.PHONY: debug
debug:
	@echo "LAYER1_SOURCES: $(LAYER1_SOURCES)"
	@echo "COMMON_SOURCES: $(COMMON_SOURCES)"
	@echo "LAYER1_OBJECTS: $(LAYER1_OBJECTS)"
	@echo "COMMON_OBJECTS: $(COMMON_OBJECTS)"
	@echo "ALL_OBJECTS: $(ALL_OBJECTS)"
	@echo "TARGET: $(BUILD_DIR)/$(TARGET)"
