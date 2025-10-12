# Makefile for Task Generator
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCDIR = src
BUILDDIR = build

# Binary
TARGET = $(BUILDDIR)/taskGenerator

.PHONY: all clean help test-all

all: $(TARGET)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build universal task generator
$(TARGET): $(SRCDIR)/taskGenerator.cc | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) $< -o $@
	@echo ""
	@echo "✓ Task generator built successfully!"
	@echo "  Usage: ./$(TARGET) <graphID> <num_cases> <packets_per_case> [seed]"
	@echo "  Example: ./$(TARGET) 1 10 15"

# Clean build artifacts
clean:
	rm -rf $(BUILDDIR)
	@echo "Cleaned build directory"

# Clean all generated test files (use with caution!)
clean-tests:
	@echo "Removing all generated test files..."
	@for i in 1 2 3 4 5 6 7 8 9 10; do \
		if [ -d ../tests/graph$$i ]; then \
			rm -rf ../tests/graph$$i/*; \
			echo "  Cleaned tests/graph$$i/"; \
		fi \
	done

# Test generator with all 10 graphs (generates 5 tasks for each)
test-all: $(TARGET)
	@echo ""
	@echo "Testing task generator with all 10 graphs..."
	@echo "============================================="
	@for i in 1 2 3 4 5 6 7 8 9 10; do \
		echo ""; \
		echo "--- Testing Graph $$i ---"; \
		./$(TARGET) $$i 3 5 12345; \
	done
	@echo ""
	@echo "✓ All graphs tested successfully!"

## Help target
help:
	@echo "Universal Task Generator Build System"
	@echo "======================================"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build the task generator"
	@echo "  make clean        - Remove build directory"
	@echo "  make clean-tests  - Remove all generated test files (graph1-10)"
	@echo "  make test-all     - Test generator with all 10 graphs"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Generated binary:"
	@echo "  build/taskGenerator   - Universal task generator for all graphs"
	@echo ""
	@echo "Usage:"
	@echo "  ./build/taskGenerator <graphID> <num_cases> <packets_per_case> [seed]"
	@echo "  ./build/taskGenerator <graphID> -i <num_cases> <start> <increment> [seed]"
	@echo ""
	@echo "Examples:"
	@echo "  ./build/taskGenerator 1 10 15        # Graph1: 10 files with 15 packets each"
	@echo "  ./build/taskGenerator 3 -i 10 10 10  # Graph3: 10, 20, 30, ..., 100 packets"
	@echo ""
	@echo "Supported Graphs: 1-10"
	@echo ""
	@echo "Usage examples (direct binaries):"
	@echo "  From utils/: ./build/generate_tasks_graph1 10 15        # Uses relative paths"
	@echo "  From workspace root (recommended):"
	@echo "    export MECALUX_ROOT=\$$(pwd)"
	@echo "    ./optimality/02_layer_planner/utils/build/generate_tasks_graph1 -i 10 10 19 179"
	@echo ""
	@echo "Note: Set MECALUX_ROOT to the workspace root if you want absolute output paths."
