# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g \
           -I../optimality/02_layer_planner/include \
           -I../optimality/01_layer_mapping/include \
           -isystem ../external/googletest/googletest/include -pthread

# Linker flags: Link our planner objects and the gtest libraries
LDFLAGS = -L../external/googletest/build/lib -lgtest -lgtest_main -pthread

# The final test executable name
TARGET = run_tests

# Test source files
TEST_SOURCES = main_test.cc scheduler_tests/scheduler_test.cc

# Application source files (needed for linking) - exclude main.cc and Algorithm.cc
APP_SOURCES = $(filter-out ../optimality/02_layer_planner/main.cc ../optimality/02_layer_planner/src/Algorithm.cc, \
              $(wildcard ../optimality/02_layer_planner/src/*.cc) \
              $(wildcard ../optimality/02_layer_planner/src/algorithms/*.cc) \
              $(wildcard ../optimality/02_layer_planner/src/algorithms/utils/*.cc) \
              $(wildcard ../optimality/01_layer_mapping/src/*.cc))

# Generate object file names
APP_OBJECTS = $(patsubst %.cc, %.o, $(APP_SOURCES))
TEST_OBJECTS = $(patsubst %.cc, %.o, $(TEST_SOURCES))

all: $(TARGET)

# Link the final executable
$(TARGET): $(APP_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(APP_OBJECTS) $(TEST_OBJECTS) $(LDFLAGS)

# Generic rule to compile any .cc file into a .o file
%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(APP_OBJECTS) $(TEST_OBJECTS) $(TARGET)

.PHONY: all clean
