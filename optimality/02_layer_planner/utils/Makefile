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
	@echo "Usage examples:"
	@echo "  ./build/generate_tasks_graph1 10 15        # 10 cases, 15 tasks each"
	@echo "  ./build/generate_tasks_graph1 -i 10 10 10  # 10 cases: 10, 20, ..., 100 tasks"
