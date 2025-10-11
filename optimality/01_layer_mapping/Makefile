CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
INCDIR = include
BUILDDIR = build
OUTDIR = utils/output

# Source files
SOURCES = $(SRCDIR)/Graph.cc main.cc
OBJECTS = $(BUILDDIR)/Graph.o $(BUILDDIR)/main.o

# Target executable
TARGET = $(BUILDDIR)/graph_demo

# Default target
all: $(TARGET)

# Create output directory
$(OUTDIR):
	mkdir -p $(OUTDIR)

# Create build directory
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# Build object files
$(BUILDDIR)/Graph.o: $(SRCDIR)/Graph.cc | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

$(BUILDDIR)/main.o: main.cc | $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

# Build target executable
$(TARGET): $(OBJECTS) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET)

# Clean build files
clean:
	rm -rf $(BUILDDIR)

# Clean everything including generated visualizations
clean-all: clean
	rm -f $(OUTDIR)/*.svg

# Run the program
run: $(TARGET)
	./$(TARGET)

# Build and run
all-run: $(TARGET)
	./$(TARGET)

# View visualizations in browser
view:
	@echo "Opening interactive viewer..."
	@command -v xdg-open >/dev/null 2>&1 && xdg-open utils/viewer.html || \
	command -v open >/dev/null 2>&1 && open utils/viewer.html || \
	command -v explorer.exe >/dev/null 2>&1 && explorer.exe utils/viewer.html || \
	echo "Please open utils/viewer.html manually in your browser"

# Help target
help:
	@echo "Available targets:"
	@echo "  make          - Build the program"
	@echo "  make run      - Build and run the program"
	@echo "  make all-run  - Build and run (alias)"
	@echo "  make view     - Open the visualization viewer"
	@echo "  make clean    - Remove compiled files"
	@echo "  make clean-all- Remove compiled files and visualizations"
	@echo "  make help     - Show this help message"

.PHONY: all clean clean-all run all-run view help