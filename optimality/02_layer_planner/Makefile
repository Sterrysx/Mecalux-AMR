CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
INCDIR = include
OBJDIR = obj
OUTDIR = output

# Path to 01_layer_mapping directory
GRAPH_DIR = ../01_layer_mapping
GRAPH_INC = $(GRAPH_DIR)/include
GRAPH_SRC = $(GRAPH_DIR)/src

# Source files
SOURCES = $(SRCDIR)/Planifier.cc $(SRCDIR)/Robot.cc $(GRAPH_SRC)/Graph.cc main.cc
OBJECTS = $(OBJDIR)/Planifier.o $(OBJDIR)/Robot.o $(OBJDIR)/Graph.o $(OBJDIR)/main.o

# Target executable
TARGET = planner

# Default target
all: $(TARGET)

# Create output directory
$(OUTDIR):
	mkdir -p $(OUTDIR)

# Create object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Build target
$(TARGET): $(OBJDIR) $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Compile Planifier.cc
$(OBJDIR)/Planifier.o: $(SRCDIR)/Planifier.cc $(INCDIR)/Planifier.hh $(GRAPH_INC)/Graph.hh $(INCDIR)/Robot.hh | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c $(SRCDIR)/Planifier.cc -o $(OBJDIR)/Planifier.o

# Compile Robot.cc
$(OBJDIR)/Robot.o: $(SRCDIR)/Robot.cc $(INCDIR)/Robot.hh | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $(SRCDIR)/Robot.cc -o $(OBJDIR)/Robot.o

# Compile Graph.cc from 01_layer_mapping
$(OBJDIR)/Graph.o: $(GRAPH_SRC)/Graph.cc $(GRAPH_INC)/Graph.hh | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(GRAPH_INC) -c $(GRAPH_SRC)/Graph.cc -o $(OBJDIR)/Graph.o

# Compile main.cc
$(OBJDIR)/main.o: main.cc $(INCDIR)/Planifier.hh $(INCDIR)/Robot.hh $(GRAPH_INC)/Graph.hh | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -I$(GRAPH_INC) -c main.cc -o $(OBJDIR)/main.o

# Clean build files
clean:
	rm -rf $(TARGET) $(OBJDIR)

# Clean everything including generated outputs
clean-all: clean
	rm -rf $(OUTDIR)

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
	@echo "  make clean-all    - Remove compiled files and outputs"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Packet Generators:"
	@echo "  cd packet_generators && make       - Build packet generators"
	@echo "  cd packet_generators && ./generate_packets_graph1 10 15  - Generate test cases"
	@echo ""
	@echo "Documentation:"
	@echo "  README.md                - Main documentation"
	@echo "  PACKET_FORMAT.md         - Packet file format specification"
	@echo "  QUICKSTART_PACKETS.md    - Quick start guide for packet generation"
	@echo "  packet_generators/README.md - Generator documentation"

.PHONY: all clean clean-all run test help
