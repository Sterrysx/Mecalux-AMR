# Makefile for Task Generators
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCDIR = src
BUILDDIR = build

# Set MECALUX_ROOT to workspace root (two levels up from utils/)
export MECALUX_ROOT := $(shell cd ../..; pwd)

# Binaries
GENERATORS = generate_tasks_graph1 generate_tasks_graph2

# Full paths to binaries in build directory
BINS = $(addprefix $(BUILDDIR)/, $(GENERATORS))

.PHONY: all clean help

all: $(BUILDDIR) $(BINS)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build individual generators
$(BUILDDIR)/generate_tasks_graph1: $(SRCDIR)/generate_tasks_graph1.cc | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/generate_tasks_graph2: $(SRCDIR)/generate_tasks_graph2.cc | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)
	rm -rf ../tests/graph1/*
	rm -rf ../tests/graph2/*

# Help target
help:
	@echo "Task Generator Build System"
	@echo "============================"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all generators in build/"
	@echo "  make clean        - Remove build/ and generated test files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Environment Variables:"
	@echo "  MECALUX_ROOT      - Set to workspace root for absolute paths"
	@echo "                      (automatically set during build)"
	@echo ""
	@echo "Usage examples:"
	@echo "  From workspace root:"
	@echo "    export MECALUX_ROOT=\$$(pwd)"
	@echo "    ./optimality/02_layer_planner/utils/build/generate_tasks_graph1 10 15"
	@echo "    ./optimality/02_layer_planner/utils/build/generate_tasks_graph1 -i 10 10 10"
	@echo ""
	@echo "  From utils/ directory:"
	@echo "    ./build/generate_tasks_graph1 10 15        # Uses relative paths"
	@echo "    ./build/generate_tasks_graph2 -i 10 5 5    # 10 cases: 5, 10, ..., 50 tasks"
