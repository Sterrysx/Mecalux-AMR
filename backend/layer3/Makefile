# ==============================================================================
# Layer 3 (Robot Driver) Makefile
# ==============================================================================
# 
# This Makefile builds Layer 3 which depends on:
# - Layer 1 (Mapping): StaticBitMap, InflatedBitMap, NavMesh, etc.
# - Common: Coordinates, Resolution
#
# Usage:
#   make          - Build Layer 3 library and test executable
#   make run      - Build and run the integration test
#   make clean    - Remove all build artifacts
#
# ==============================================================================

# Compiler and Flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O2

# Directories
LAYER3_DIR := .
LAYER1_DIR := ../layer1
COMMON_DIR := ../common
BUILD_DIR := build

# Include paths
INCLUDES := -I$(LAYER3_DIR)/include \
            -I$(LAYER1_DIR)/include \
            -I$(COMMON_DIR)/include

# ==============================================================================
# Source Files
# ==============================================================================

# Layer 3 sources (organized by package)
PATHFINDING_SOURCES := $(wildcard $(LAYER3_DIR)/src/Pathfinding/*.cc)
PHYSICS_SOURCES := $(wildcard $(LAYER3_DIR)/src/Physics/*.cc)
CORE_SOURCES := $(wildcard $(LAYER3_DIR)/src/Core/*.cc)

LAYER3_SOURCES := $(PATHFINDING_SOURCES) $(PHYSICS_SOURCES) $(CORE_SOURCES)

# Main test source
MAIN_SOURCE := main.cc

# ==============================================================================
# Object Files
# ==============================================================================

# Layer 3 objects (flattened into build/)
PATHFINDING_OBJECTS := $(patsubst $(LAYER3_DIR)/src/Pathfinding/%.cc,$(BUILD_DIR)/Pathfinding_%.o,$(PATHFINDING_SOURCES))
PHYSICS_OBJECTS := $(patsubst $(LAYER3_DIR)/src/Physics/%.cc,$(BUILD_DIR)/Physics_%.o,$(PHYSICS_SOURCES))
CORE_OBJECTS := $(patsubst $(LAYER3_DIR)/src/Core/%.cc,$(BUILD_DIR)/Core_%.o,$(CORE_SOURCES))

LAYER3_OBJECTS := $(PATHFINDING_OBJECTS) $(PHYSICS_OBJECTS) $(CORE_OBJECTS)

# Main object
MAIN_OBJECT := $(BUILD_DIR)/main.o

# Layer 1 objects (pre-built)
LAYER1_OBJECTS := $(wildcard $(LAYER1_DIR)/build/*.o)

# ==============================================================================
# Targets
# ==============================================================================

TARGET := $(BUILD_DIR)/test_layer3

.PHONY: all clean run layer1 help

all: layer1 $(BUILD_DIR) $(TARGET)
	@echo "[Layer 3] Build complete: $(TARGET)"

# Build Layer 1 dependencies first
layer1:
	@echo "[Layer 3] Building Layer 1 dependencies..."
	$(MAKE) -C $(LAYER1_DIR) all

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ==============================================================================
# Linking
# ==============================================================================

$(TARGET): $(LAYER3_OBJECTS) $(MAIN_OBJECT) | layer1
	@echo "[Layer 3] Linking $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ \
		$(LAYER3_OBJECTS) \
		$(MAIN_OBJECT) \
		$(LAYER1_DIR)/build/StaticBitMap.o \
		$(LAYER1_DIR)/build/InflatedBitMap.o \
		$(LAYER1_DIR)/build/DynamicBitMap.o \
		$(LAYER1_DIR)/build/NavMesh.o \
		$(LAYER1_DIR)/build/NavMeshGenerator.o \
		$(LAYER1_DIR)/build/AbstractGrid.o \
		$(LAYER1_DIR)/build/DynamicObstacle.o \
		$(LAYER1_DIR)/build/DynamicObstacleGenerator.o \
		$(LAYER1_DIR)/build/POIRegistry.o \
		$(LAYER1_DIR)/build/common_Coordinates.o \
		$(LAYER1_DIR)/build/common_Resolution.o \
		-pthread

# ==============================================================================
# Compilation Rules
# ==============================================================================

# Pathfinding package
$(BUILD_DIR)/Pathfinding_%.o: $(LAYER3_DIR)/src/Pathfinding/%.cc | $(BUILD_DIR)
	@echo "[Layer 3] Compiling Pathfinding/$*.cc..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Physics package
$(BUILD_DIR)/Physics_%.o: $(LAYER3_DIR)/src/Physics/%.cc | $(BUILD_DIR)
	@echo "[Layer 3] Compiling Physics/$*.cc..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Core package
$(BUILD_DIR)/Core_%.o: $(LAYER3_DIR)/src/Core/%.cc | $(BUILD_DIR)
	@echo "[Layer 3] Compiling Core/$*.cc..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Main test
$(BUILD_DIR)/main.o: $(LAYER3_DIR)/main.cc | $(BUILD_DIR)
	@echo "[Layer 3] Compiling main.cc..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# ==============================================================================
# Utility Targets
# ==============================================================================

run: all
	@echo ""
	@echo "╔═══════════════════════════════════════════════════════════════════════════╗"
	@echo "║                     RUNNING LAYER 3 INTEGRATION TEST                      ║"
	@echo "╚═══════════════════════════════════════════════════════════════════════════╝"
	@echo ""
	./$(TARGET)

clean:
	@echo "[Layer 3] Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)

help:
	@echo "Layer 3 (Robot Driver) Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all    - Build Layer 3 library and test executable (default)"
	@echo "  run    - Build and run the integration test"
	@echo "  clean  - Remove all build artifacts"
	@echo "  layer1 - Build Layer 1 dependencies only"
	@echo "  help   - Show this help message"
	@echo ""
	@echo "Structure:"
	@echo "  include/Vector2.hh          - 2D Vector math"
	@echo "  include/Pathfinding/        - Theta* pathfinding"
	@echo "  include/Physics/            - ORCA collision avoidance"
	@echo "  include/Core/               - RobotDriver, FastLoopManager"
	@echo ""

# ==============================================================================
# Dependencies (auto-generated would go here)
# ==============================================================================

# Pathfinding
$(BUILD_DIR)/Pathfinding_ThetaStarSolver.o: \
	$(LAYER3_DIR)/include/Pathfinding/ThetaStarSolver.hh \
	$(COMMON_DIR)/include/Coordinates.hh \
	$(LAYER1_DIR)/include/InflatedBitMap.hh

$(BUILD_DIR)/Pathfinding_PathfindingService.o: \
	$(LAYER3_DIR)/include/Pathfinding/PathfindingService.hh \
	$(LAYER3_DIR)/include/Pathfinding/ThetaStarSolver.hh

# Physics
$(BUILD_DIR)/Physics_ORCASolver.o: \
	$(LAYER3_DIR)/include/Physics/ORCASolver.hh \
	$(LAYER3_DIR)/include/Physics/ObstacleData.hh \
	$(LAYER3_DIR)/include/Vector2.hh

# Core
$(BUILD_DIR)/Core_RobotDriver.o: \
	$(LAYER3_DIR)/include/Core/RobotDriver.hh \
	$(LAYER3_DIR)/include/Pathfinding/PathfindingService.hh \
	$(LAYER3_DIR)/include/Physics/ORCASolver.hh \
	$(LAYER1_DIR)/include/NavMesh.hh

$(BUILD_DIR)/Core_FastLoopManager.o: \
	$(LAYER3_DIR)/include/Core/FastLoopManager.hh \
	$(LAYER3_DIR)/include/Core/RobotDriver.hh
