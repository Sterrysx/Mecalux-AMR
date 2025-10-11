CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
INCDIR = include
BUILDDIR = build

# Path to 01_layer_mapping directory
GRAPH_DIR = ../01_layer_mapping
GRAPH_INC = $(GRAPH_DIR)/include
GRAPH_SRC = $(GRAPH_DIR)/src

# Source files
SOURCES = $(SRCDIR)/Planifier.cc $(SRCDIR)/Robot.cc $(SRCDIR)/Task.cc \
          $(SRCDIR)/BruteForceAlgorithm.cc $(SRCDIR)/GreedyAlgorithm.cc \
          $(SRCDIR)/PriorityAlgorithm.cc $(SRCDIR)/HybridAlgorithm.cc \
          $(GRAPH_SRC)/Graph.cc main.cc
OBJECTS = $(BUILDDIR)/Planifier.o $(BUILDDIR)/Robot.o $(BUILDDIR)/Task.o \
          $(BUILDDIR)/BruteForceAlgorithm.o $(BUILDDIR)/GreedyAlgorithm.o \
          $(BUILDDIR)/PriorityAlgorithm.o $(BUILDDIR)/HybridAlgorithm.o \
          $(BUILDDIR)/Graph.o $(BUILDDIR)/main.o

# Target executable
TARGET = planner

# Default target
all: $(TARGET)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build target
$(TARGET): $(BUILDDIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Compile Planifier.cc
$(BUILDDIR)/Planifier.o: $(SRCDIR)/Planifier.cc $(INCDIR)/Planifier.hh $(INCDIR)/Algorithm.hh $(INCDIR)/Task.hh $(GRAPH_INC)/Graph.hh $(INCDIR)/Robot.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c $(SRCDIR)/Planifier.cc -o $(BUILDDIR)/Planifier.o

# Compile Robot.cc
$(BUILDDIR)/Robot.o: $(SRCDIR)/Robot.cc $(INCDIR)/Robot.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $(SRCDIR)/Robot.cc -o $(BUILDDIR)/Robot.o

# Compile Task.cc
$(BUILDDIR)/Task.o: $(SRCDIR)/Task.cc $(INCDIR)/Task.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $(SRCDIR)/Task.cc -o $(BUILDDIR)/Task.o

# Compile Algorithm implementations
$(BUILDDIR)/BruteForceAlgorithm.o: $(SRCDIR)/BruteForceAlgorithm.cc $(INCDIR)/BruteForceAlgorithm.hh $(INCDIR)/Algorithm.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c $(SRCDIR)/BruteForceAlgorithm.cc -o $(BUILDDIR)/BruteForceAlgorithm.o

$(BUILDDIR)/GreedyAlgorithm.o: $(SRCDIR)/GreedyAlgorithm.cc $(INCDIR)/GreedyAlgorithm.hh $(INCDIR)/Algorithm.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c $(SRCDIR)/GreedyAlgorithm.cc -o $(BUILDDIR)/GreedyAlgorithm.o

$(BUILDDIR)/PriorityAlgorithm.o: $(SRCDIR)/PriorityAlgorithm.cc $(INCDIR)/PriorityAlgorithm.hh $(INCDIR)/Algorithm.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c $(SRCDIR)/PriorityAlgorithm.cc -o $(BUILDDIR)/PriorityAlgorithm.o

$(BUILDDIR)/HybridAlgorithm.o: $(SRCDIR)/HybridAlgorithm.cc $(INCDIR)/HybridAlgorithm.hh $(INCDIR)/Algorithm.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c $(SRCDIR)/HybridAlgorithm.cc -o $(BUILDDIR)/HybridAlgorithm.o

# Compile Graph.cc from 01_layer_mapping
$(BUILDDIR)/Graph.o: $(GRAPH_SRC)/Graph.cc $(GRAPH_INC)/Graph.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(GRAPH_INC) -c $(GRAPH_SRC)/Graph.cc -o $(BUILDDIR)/Graph.o

# Compile main.cc
$(BUILDDIR)/main.o: main.cc $(INCDIR)/Planifier.hh $(INCDIR)/Task.hh $(INCDIR)/Robot.hh $(GRAPH_INC)/Graph.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c main.cc -o $(BUILDDIR)/main.o

# Clean build files
clean:
	rm -rf $(TARGET) $(BUILDDIR)

# Clean everything including old directories
clean-all: clean
	rm -rf obj output

# Run the program
run: $(TARGET)
	./$(TARGET)

# Build and run
test: $(TARGET)
	@echo "=== Running Planner Test ==="
	./$(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  make              - Build the planner program"
	@echo "  make run          - Build and run the program"
	@echo "  make test         - Build and run (alias)"
	@echo "  make clean        - Remove compiled files"
	@echo "  make clean-all    - Remove compiled files and old directories"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Task Generators:"
	@echo "  cd utils && make                              - Build task generators"
	@echo "  cd utils && ./build/generate_tasks_graph1 10 15  - Generate test cases"
	@echo ""
	@echo "Directory Structure:"
	@echo "  src/              - Source files (.cc)"
	@echo "  include/          - Header files (.hh)"
	@echo "  build/            - Object files (.o)"
	@echo "  tests/            - Test cases (graph1/, graph2/)"
	@echo "  utils/            - Task generators (src/, build/)"

.PHONY: all clean clean-all run test help
