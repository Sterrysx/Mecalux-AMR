CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g
SRCDIR = src
INCDIR = include
OBJDIR = obj

# Source files
SOURCES = $(SRCDIR)/Graph.cc main.cpp
OBJECTS = $(SOURCES:.cc=.o)
OBJECTS := $(OBJECTS:.cpp=.o)

# Target executable
TARGET = graph_demo

# Default target
all: $(TARGET)

# Create object directory
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Build target
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) $(SOURCES) -o $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET) $(OBJDIR)/*.o

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run