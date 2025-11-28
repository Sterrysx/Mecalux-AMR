# ==============================================================================
# Layer 2 (Fleet Manager) Makefile
# ==============================================================================
# Dependencies: Layer 1 (must be built first)
# 
# Usage:
#   make           - Build Layer 2 test executable
#   make run       - Run the test
#   make clean     - Clean build artifacts
#   make layer1    - Build Layer 1 first (if needed)
# ==============================================================================

# Compiler and Flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g

# Directories
LAYER2_DIR := .
LAYER1_DIR := ../layer1
COMMON_DIR := ../common
BUILD_DIR := build

# Include paths
INCLUDES := -I$(LAYER2_DIR)/include \
            -I$(LAYER1_DIR)/include \
            -I$(COMMON_DIR)/include

# Source files
LAYER2_SOURCES := $(wildcard $(LAYER2_DIR)/src/*.cc)
MAIN_SOURCE := main.cc

# Object files for Layer 2
LAYER2_OBJECTS := $(patsubst $(LAYER2_DIR)/src/%.cc,$(BUILD_DIR)/%.o,$(LAYER2_SOURCES))
MAIN_OBJECT := $(BUILD_DIR)/main.o

# Layer 1 objects (pre-built, link against them)
# We need to list the Layer 1 objects that Layer 2 depends on
LAYER1_BUILD := $(LAYER1_DIR)/build
LAYER1_OBJECTS := $(LAYER1_BUILD)/StaticBitMap.o \
                  $(LAYER1_BUILD)/InflatedBitMap.o \
                  $(LAYER1_BUILD)/DynamicBitMap.o \
                  $(LAYER1_BUILD)/NavMesh.o \
                  $(LAYER1_BUILD)/NavMeshGenerator.o \
                  $(LAYER1_BUILD)/AbstractGrid.o \
                  $(LAYER1_BUILD)/DynamicObstacle.o \
                  $(LAYER1_BUILD)/DynamicObstacleGenerator.o \
                  $(LAYER1_BUILD)/POIRegistry.o

# Common objects
COMMON_OBJECTS := $(LAYER1_BUILD)/common_Coordinates.o \
                  $(LAYER1_BUILD)/common_Resolution.o

# All objects for final linking
ALL_OBJECTS := $(LAYER2_OBJECTS) $(MAIN_OBJECT) $(LAYER1_OBJECTS) $(COMMON_OBJECTS)

# Target executable
TARGET := test_layer2

# ==============================================================================
# Rules
# ==============================================================================

.PHONY: all clean run layer1 debug assets

all: assets $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

# Create build directory and assets directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

assets:
	mkdir -p assets

# Ensure Layer 1 is built first
layer1:
	@echo "Building Layer 1 dependencies..."
	$(MAKE) -C $(LAYER1_DIR) all

# Link all objects into executable
$(BUILD_DIR)/$(TARGET): $(LAYER2_OBJECTS) $(MAIN_OBJECT) | layer1
	@echo "Linking Layer 2 executable..."
	$(CXX) $(CXXFLAGS) -o $@ $(ALL_OBJECTS)

# Compile Layer 2 source files
$(BUILD_DIR)/%.o: $(LAYER2_DIR)/src/%.cc | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile main.cc
$(BUILD_DIR)/main.o: $(MAIN_SOURCE) | $(BUILD_DIR)
	@echo "Compiling main.cc..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the test executable
run: $(BUILD_DIR)/$(TARGET)
	@echo "Running Layer 2 tests..."
	./$(BUILD_DIR)/$(TARGET)

# Build and run algorithm comparison benchmark
compare: $(BUILD_DIR)/algorithm_comparison
	@echo "Running algorithm comparison..."
	./$(BUILD_DIR)/algorithm_comparison

# Compile algorithm comparison executable
$(BUILD_DIR)/algorithm_comparison: algorithm_comparison.cc $(LAYER2_OBJECTS) | layer1 $(BUILD_DIR)
	@echo "Building algorithm comparison..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ algorithm_comparison.cc $(LAYER2_OBJECTS) $(LAYER1_OBJECTS) $(COMMON_OBJECTS)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f assets/tasks.json

# Clean everything including Layer 1
cleanall: clean
	$(MAKE) -C $(LAYER1_DIR) clean

# ==============================================================================
# Debug: Print variables
# ==============================================================================
debug:
	@echo "=== Layer 2 Build Configuration ==="
	@echo "LAYER2_SOURCES: $(LAYER2_SOURCES)"
	@echo "LAYER2_OBJECTS: $(LAYER2_OBJECTS)"
	@echo "LAYER1_OBJECTS: $(LAYER1_OBJECTS)"
	@echo "COMMON_OBJECTS: $(COMMON_OBJECTS)"
	@echo "ALL_OBJECTS: $(ALL_OBJECTS)"
	@echo "TARGET: $(BUILD_DIR)/$(TARGET)"
