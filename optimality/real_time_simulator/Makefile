CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -pthread
SRCDIR = src
INCDIR = include
BUILDDIR = build

# Path to 01_layer_mapping directory
GRAPH_DIR = ../01_layer_mapping
GRAPH_INC = $(GRAPH_DIR)/include
GRAPH_SRC = $(GRAPH_DIR)/src

# Path to 02_layer_planner directory (we use it as a library)
PLANNER_DIR = ../02_layer_planner
PLANNER_INC = $(PLANNER_DIR)/include
PLANNER_SRC = $(PLANNER_DIR)/src

# Source files from current project
LOCAL_SOURCES = $(SRCDIR)/SimulationController.cc main.cc

# Source files from planner (reused as library)
PLANNER_SOURCES = $(PLANNER_SRC)/Planifier.cc \
                  $(PLANNER_SRC)/Robot.cc \
                  $(PLANNER_SRC)/Task.cc \
                  $(PLANNER_SRC)/algorithms/01_BruteForce.cc \
                  $(PLANNER_SRC)/algorithms/02_Greedy.cc \
                  $(PLANNER_SRC)/algorithms/03_HillClimbing.cc \
                  $(PLANNER_SRC)/algorithms/utils/SchedulerUtils.cc \
                  $(PLANNER_SRC)/algorithms/utils/TSPSolver.cc \
                  $(PLANNER_SRC)/algorithms/utils/AssignmentPrinter.cc

# Source files from graph layer
GRAPH_SOURCES = $(GRAPH_SRC)/Graph.cc

# Object files
OBJECTS = $(BUILDDIR)/SimulationController.o \
          $(BUILDDIR)/main.o \
          $(BUILDDIR)/Planifier.o \
          $(BUILDDIR)/Robot.o \
          $(BUILDDIR)/Task.o \
          $(BUILDDIR)/01_BruteForce.o \
          $(BUILDDIR)/02_Greedy.o \
          $(BUILDDIR)/03_HillClimbing.o \
          $(BUILDDIR)/SchedulerUtils.o \
          $(BUILDDIR)/TSPSolver.o \
          $(BUILDDIR)/AssignmentPrinter.o \
          $(BUILDDIR)/Graph.o

# Target executable
TARGET = $(BUILDDIR)/simulator

# Default target
all: $(TARGET)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build target
$(TARGET): $(BUILDDIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Compile SimulationController.cc
$(BUILDDIR)/SimulationController.o: $(SRCDIR)/SimulationController.cc $(INCDIR)/SimulationController.hh $(PLANNER_INC)/Planifier.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(SRCDIR)/SimulationController.cc -o $(BUILDDIR)/SimulationController.o

# Compile main.cc
$(BUILDDIR)/main.o: main.cc $(INCDIR)/SimulationController.hh $(PLANNER_INC)/Planifier.hh $(GRAPH_INC)/Graph.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(PLANNER_INC) -I$(GRAPH_INC) -c main.cc -o $(BUILDDIR)/main.o

# Compile Planifier components (from 02_layer_planner)
$(BUILDDIR)/Planifier.o: $(PLANNER_SRC)/Planifier.cc $(PLANNER_INC)/Planifier.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/Planifier.cc -o $(BUILDDIR)/Planifier.o

$(BUILDDIR)/Robot.o: $(PLANNER_SRC)/Robot.cc $(PLANNER_INC)/Robot.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -c $(PLANNER_SRC)/Robot.cc -o $(BUILDDIR)/Robot.o

$(BUILDDIR)/Task.o: $(PLANNER_SRC)/Task.cc $(PLANNER_INC)/Task.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -c $(PLANNER_SRC)/Task.cc -o $(BUILDDIR)/Task.o

# Compile Algorithm implementations (from 02_layer_planner)
$(BUILDDIR)/01_BruteForce.o: $(PLANNER_SRC)/algorithms/01_BruteForce.cc $(PLANNER_INC)/algorithms/01_BruteForce.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/algorithms/01_BruteForce.cc -o $(BUILDDIR)/01_BruteForce.o

$(BUILDDIR)/02_Greedy.o: $(PLANNER_SRC)/algorithms/02_Greedy.cc $(PLANNER_INC)/algorithms/02_Greedy.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/algorithms/02_Greedy.cc -o $(BUILDDIR)/02_Greedy.o

$(BUILDDIR)/03_HillClimbing.o: $(PLANNER_SRC)/algorithms/03_HillClimbing.cc $(PLANNER_INC)/algorithms/03_HillClimbing.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/algorithms/03_HillClimbing.cc -o $(BUILDDIR)/03_HillClimbing.o

# Compile utility implementations (from 02_layer_planner)
$(BUILDDIR)/SchedulerUtils.o: $(PLANNER_SRC)/algorithms/utils/SchedulerUtils.cc $(PLANNER_INC)/algorithms/utils/SchedulerUtils.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/algorithms/utils/SchedulerUtils.cc -o $(BUILDDIR)/SchedulerUtils.o

$(BUILDDIR)/TSPSolver.o: $(PLANNER_SRC)/algorithms/utils/TSPSolver.cc $(PLANNER_INC)/algorithms/utils/TSPSolver.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/algorithms/utils/TSPSolver.cc -o $(BUILDDIR)/TSPSolver.o

$(BUILDDIR)/AssignmentPrinter.o: $(PLANNER_SRC)/algorithms/utils/AssignmentPrinter.cc $(PLANNER_INC)/algorithms/utils/AssignmentPrinter.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(PLANNER_INC) -I$(GRAPH_INC) -c $(PLANNER_SRC)/algorithms/utils/AssignmentPrinter.cc -o $(BUILDDIR)/AssignmentPrinter.o

# Compile Graph.cc (from 01_layer_mapping)
$(BUILDDIR)/Graph.o: $(GRAPH_SRC)/Graph.cc $(GRAPH_INC)/Graph.hh | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(GRAPH_INC) -c $(GRAPH_SRC)/Graph.cc -o $(BUILDDIR)/Graph.o

# Clean build files
clean:
	rm -rf $(BUILDDIR)

# Run the program with default parameters
run: $(TARGET)
	$(TARGET) 1 5

# Run with a test case
run-test: $(TARGET)
	$(TARGET) 1 5 1

# Help target
help:
	@echo "=== Real-Time Simulator Build System ==="
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build the simulator"
	@echo "  make run          - Build and run with default parameters (graph 1, 5 robots)"
	@echo "  make run-test     - Build and run with test case (graph 1, 5 robots, case 1)"
	@echo "  make clean        - Remove compiled files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Manual execution:"
	@echo "  $(TARGET) <graphId> [numRobots] [testCase]"
	@echo "  Example: $(TARGET) 1 10       # Graph 1, 10 robots"
	@echo "  Example: $(TARGET) 3 5 2      # Graph 3, 5 robots, test case 2"
	@echo ""
	@echo "Architecture:"
	@echo "  This simulator reuses 02_layer_planner as a library."
	@echo "  It does NOT modify the planner's main.cc or any existing functionality."
	@echo "  SimulationController manages the real-time loop and user interaction."
	@echo ""

.PHONY: all clean run run-test help
