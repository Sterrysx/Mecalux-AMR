CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
INCDIR = include
OBJDIR = obj
OUTDIR = output

# Source files
SOURCES = $(SRCDIR)/Graph.cc main.cpp
OBJECTS = $(SOURCES:.cc=.o)
OBJECTS := $(OBJECTS:.cpp=.o)

# Target executable
TARGET = graph_demo

# Default target
all: $(TARGET)

# Create output directory
$(OUTDIR):
	mkdir -p $(OUTDIR)

# Create object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Build target
$(TARGET): $(SOURCES) | $(OUTDIR)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) $(SOURCES) -o $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET) $(OBJDIR)/*.o

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
	@command -v xdg-open >/dev/null 2>&1 && xdg-open viewer.html || \
	command -v open >/dev/null 2>&1 && open viewer.html || \
	command -v explorer.exe >/dev/null 2>&1 && explorer.exe viewer.html || \
	echo "Please open viewer.html manually in your browser"

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