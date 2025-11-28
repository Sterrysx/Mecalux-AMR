# ==============================================================================
# Mecalux AMR Fleet Management System - Root Makefile
# ==============================================================================
#
# This Makefile builds the complete system by linking all layers:
# - Layer 1: Maps, NavMesh, POIs (static infrastructure)
# - Layer 2: VRP Solver, Task Logic (planning)
# - Layer 3: Pathfinding, Physics (simulation)
# - FleetManager: System orchestrator
#
# Usage:
#   make              - Build all layers and the fleet manager
#   make run          - Run the fleet manager
#   make test         - Run with sample tasks for 30 seconds
#   make clean        - Clean all build artifacts
#   make layer1       - Build only Layer 1
#   make layer2       - Build only Layer 2
#   make layer3       - Build only Layer 3
#
# ==============================================================================

# Compiler and Flags
CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O2 -pthread

# Directories
ROOT_DIR := .
COMMON_DIR := common
LAYER1_DIR := layer1
LAYER2_DIR := layer2
LAYER3_DIR := layer3
INCLUDE_DIR := include
SRC_DIR := src
BUILD_DIR := build

# Include paths
INCLUDES := -I$(INCLUDE_DIR) \
            -I$(COMMON_DIR)/include \
            -I$(LAYER1_DIR)/include \
            -I$(LAYER2_DIR)/include \
            -I$(LAYER3_DIR)/include

# ==============================================================================
# Source Files
# ==============================================================================

# FleetManager sources
FLEETMANAGER_SOURCES := $(wildcard $(SRC_DIR)/*.cc)
MAIN_SOURCE := main.cc

# ==============================================================================
# Object Files
# ==============================================================================

# FleetManager objects
FLEETMANAGER_OBJECTS := $(patsubst $(SRC_DIR)/%.cc,$(BUILD_DIR)/%.o,$(FLEETMANAGER_SOURCES))
MAIN_OBJECT := $(BUILD_DIR)/main.o

# Layer 1 objects (pre-built)
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

# Layer 2 objects (explicitly listed to avoid wildcard timing issues)
LAYER2_BUILD := $(LAYER2_DIR)/build
LAYER2_OBJECTS := $(LAYER2_BUILD)/ALNS.o \
                  $(LAYER2_BUILD)/CostMatrixProvider.o \
                  $(LAYER2_BUILD)/HillClimbing.o \
                  $(LAYER2_BUILD)/IVRPSolver.o \
                  $(LAYER2_BUILD)/SimulatedAnnealing.o \
                  $(LAYER2_BUILD)/TabuSearch.o \
                  $(LAYER2_BUILD)/TaskLoader.o

# Layer 3 objects (explicitly listed to avoid wildcard timing issues)
LAYER3_BUILD := $(LAYER3_DIR)/build
LAYER3_OBJECTS := $(LAYER3_BUILD)/Core_FastLoopManager.o \
                  $(LAYER3_BUILD)/Core_RobotDriver.o \
                  $(LAYER3_BUILD)/Pathfinding_PathfindingService.o \
                  $(LAYER3_BUILD)/Pathfinding_ThetaStarSolver.o \
                  $(LAYER3_BUILD)/Physics_ORCASolver.o

# All objects for final linking
ALL_OBJECTS := $(FLEETMANAGER_OBJECTS) \
               $(MAIN_OBJECT) \
               $(LAYER1_OBJECTS) \
               $(COMMON_OBJECTS) \
               $(LAYER2_OBJECTS) \
               $(LAYER3_OBJECTS)

# Target executable
TARGET := fleet_manager

# ==============================================================================
# Rules
# ==============================================================================

.PHONY: all clean run test layer1 layer2 layer3 layers debug info

# Default target: build everything
all: layers $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)
	@echo ""
	@echo "╔═══════════════════════════════════════════════════════════════╗"
	@echo "║           BUILD COMPLETE: $(BUILD_DIR)/$(TARGET)                    ║"
	@echo "╚═══════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "Usage:"
	@echo "  make run      - Run the fleet manager"
	@echo "  make test     - Run a 30-second test"
	@echo ""

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build all layers
layers: layer1 layer2 layer3

# Build Layer 1
layer1:
	@echo ""
	@echo "═══════════════ Building Layer 1 ═══════════════"
	$(MAKE) -C $(LAYER1_DIR) all

# Build Layer 2
layer2: layer1
	@echo ""
	@echo "═══════════════ Building Layer 2 ═══════════════"
	$(MAKE) -C $(LAYER2_DIR) all

# Build Layer 3
layer3: layer1
	@echo ""
	@echo "═══════════════ Building Layer 3 ═══════════════"
	$(MAKE) -C $(LAYER3_DIR) all

# Link all objects into executable
$(BUILD_DIR)/$(TARGET): $(FLEETMANAGER_OBJECTS) $(MAIN_OBJECT) | layers
	@echo ""
	@echo "═══════════════ Linking Fleet Manager ═══════════════"
	$(CXX) $(CXXFLAGS) -o $@ $(ALL_OBJECTS) -pthread

# Compile FleetManager source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc | $(BUILD_DIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile main.cc
$(BUILD_DIR)/main.o: $(MAIN_SOURCE) | $(BUILD_DIR)
	@echo "Compiling main.cc..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run the fleet manager
run: $(BUILD_DIR)/$(TARGET)
	@echo ""
	@echo "Starting Fleet Manager..."
	./$(BUILD_DIR)/$(TARGET)

# Run a test (30 seconds)
test: $(BUILD_DIR)/$(TARGET)
	@echo ""
	@echo "Running 30-second test..."
	./$(BUILD_DIR)/$(TARGET) --duration 30

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(LAYER1_DIR) clean
	$(MAKE) -C $(LAYER2_DIR) clean
	$(MAKE) -C $(LAYER3_DIR) clean
	@echo "Clean complete."

# Clean only root build
clean-root:
	rm -rf $(BUILD_DIR)

# Debug info
info:
	@echo "FleetManager Sources: $(FLEETMANAGER_SOURCES)"
	@echo "FleetManager Objects: $(FLEETMANAGER_OBJECTS)"
	@echo "Layer 1 Objects: $(LAYER1_OBJECTS)"
	@echo "Layer 2 Objects: $(LAYER2_OBJECTS)"
	@echo "Layer 3 Objects: $(LAYER3_OBJECTS)"
	@echo "All Objects: $(ALL_OBJECTS)"

# Debug build
debug: CXXFLAGS += -DDEBUG -O0
debug: all
