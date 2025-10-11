# Makefile for Task Generators
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCDIR = src
BUILDDIR = build

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

## (No wrapper scripts are generated anymore - build only creates binaries)

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)
	rm -rf ../tests/graph1/*
	rm -rf ../tests/graph2/*

## Help target
help:
	@echo "Task Generator Build System"
	@echo "============================"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all generator binaries in build/"
	@echo "  make clean        - Remove build/ and generated test files"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Generated files:"
	@echo "  build/generate_tasks_graph1       - Direct binary"
	@echo "  build/generate_tasks_graph2       - Direct binary"
	@echo ""
	@echo "Usage examples (direct binaries):"
	@echo "  From utils/: ./build/generate_tasks_graph1 10 15        # Uses relative paths"
	@echo "  From workspace root (recommended):"
	@echo "    export MECALUX_ROOT=\$$(pwd)"
	@echo "    ./optimality/02_layer_planner/utils/build/generate_tasks_graph1 -i 10 10 19 179"
	@echo ""
	@echo "Note: Set MECALUX_ROOT to the workspace root if you want absolute output paths."
