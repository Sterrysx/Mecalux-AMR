# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g \
           -I../optimality/02_layer_planner/include \
           -I../optimality/01_layer_mapping/include \
           -isystem ../external/googletest/googletest/include -pthread

# Linker flags: Link our planner objects and the gtest libraries
LDFLAGS = -L../external/googletest/build/lib -lgtest -lgtest_main -pthread

# The final test executable name (output to build directory)
TARGET = build/run_tests

# Test source files
TEST_SOURCES = main.cc \
               unit_tests/brute_force_test.cc \
               unit_tests/greedy_test.cc \
               unit_tests/hill_climbing_test.cc \
               integration_tests/optimal_comparison_test.cc \
               integration_tests/heuristic_comparison_test.cc

# Application source files (needed for linking) - exclude main.cc and Algorithm.cc
APP_SOURCES = $(filter-out ../optimality/02_layer_planner/main.cc ../optimality/02_layer_planner/src/Algorithm.cc, \
              $(wildcard ../optimality/02_layer_planner/src/*.cc) \
              $(wildcard ../optimality/02_layer_planner/src/algorithms/*.cc) \
              $(wildcard ../optimality/02_layer_planner/src/algorithms/utils/*.cc) \
              $(wildcard ../optimality/01_layer_mapping/src/*.cc))

# Generate object file names (output to build directory)
BUILD_DIR = build
APP_OBJECTS = $(patsubst %.cc, $(BUILD_DIR)/%.o, $(notdir $(APP_SOURCES)))
TEST_OBJECTS = $(patsubst %.cc, $(BUILD_DIR)/%.o, $(notdir $(TEST_SOURCES)))

# Create build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

all: $(TARGET)

# Link the final executable
$(TARGET): $(APP_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(APP_OBJECTS) $(TEST_OBJECTS) $(LDFLAGS)

# Compile application source files
$(BUILD_DIR)/%.o: ../optimality/02_layer_planner/src/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../optimality/02_layer_planner/src/algorithms/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../optimality/02_layer_planner/src/algorithms/utils/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: ../optimality/01_layer_mapping/src/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile test source files
$(BUILD_DIR)/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: unit_tests/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: integration_tests/%.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f unit_tests/*.o integration_tests/*.o  # Remove old .o files if they exist

.PHONY: all clean
