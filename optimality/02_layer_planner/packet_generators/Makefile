# Makefile for Packet Generators
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Binaries
GENERATORS = generate_packets_graph1 generate_packets_graph2

.PHONY: all clean help

all: $(GENERATORS)

# Build individual generators
generate_packets_graph1: generate_packets_graph1.cc
	$(CXX) $(CXXFLAGS) $< -o $@

generate_packets_graph2: generate_packets_graph2.cc
	$(CXX) $(CXXFLAGS) $< -o $@

# Clean build artifacts
clean:
	rm -f $(GENERATORS)
	rm -rf ../generated_packets/graph1/*
	rm -rf ../generated_packets/graph2/*

# Help target
help:
	@echo "Packet Generator Build System"
	@echo "=============================="
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build all generators"
	@echo "  make clean        - Remove binaries and generated packets"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  ./generate_packets_graph1 10 15        # 10 cases, 15 packets each"
	@echo "  ./generate_packets_graph1 5 20 42      # 5 cases, 20 packets, seed=42"
