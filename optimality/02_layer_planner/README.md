# Layer 2: Planner

## Overview
This layer implements the planning algorithms for the multi-robot warehouse system. It uses the Graph data structure from Layer 1 (01_layer_mapping) and manages Robot instances.

## Building and Running

### Compile
```bash
make
```

### Usage
```bash
./planner [graph_number] [algorithm]
```

#### Arguments
- **graph_number**: Number from 1-10 (default: 1)
  - Loads from `../01_layer_mapping/distributions/graphN.inp`
- **algorithm**: Algorithm selection (default: 1)
  - `0` = Interactive mode (prompts for algorithm selection)
  - `1-4` = Direct algorithm selection

#### Examples
```bash
./planner              # Use graph1.inp with algorithm 1
./planner 5            # Use graph5.inp with algorithm 1
./planner 3 2          # Use graph3.inp with algorithm 2
./planner 1 0          # Use graph1.inp with interactive mode
```

### Interactive Mode
When using algorithm 0 or running without arguments, you'll be prompted:
```
=== Algorithm Selection ===
Available algorithms:
  1. Algorithm 1
  2. Algorithm 2
  3. Algorithm 3
  4. Algorithm 4
Choose an algorithm (1-4):
```

```cpp
P.plan(1);  // Algorithm 1
P.plan(2);  // Algorithm 2
P.plan(3);  // Algorithm 3
P.plan(4);  // Algorithm 4
```

### Clean Build
```bash
make clean       # Remove compiled files
make clean-all   # Remove everything including outputs
```

## Makefile Targets

| Target | Description |
|--------|-------------|
| `make` | Build the planner program |
| `make run` | Build and run the program |
| `make test` | Build and run (alias for run) |
| `make clean` | Remove compiled files |
| `make clean-all` | Remove all build artifacts |
| `make help` | Show available targets |

## Dependencies

- **Layer 1 (01_layer_mapping)**: Uses Graph.hh and Graph.cc
- **C++17 compiler**: Required for modern C++ features
- **Standard Library**: iostream, string, queue, vector, etc.

## Directory Structure

```
02_layer_planner/
├── Makefile                  # Build configuration
├── main.cc                   # Main entry point
├── README.md                 # This file
├── include/
│   ├── Planifier.hh          # Planner class header
│   └── Robot.hh              # Robot class header
├── src/
│   ├── Planifier.cc          # Planner implementation
│   └── Robot.cc              # Robot implementation
├── obj/                      # Compiled object files (generated)
├── packets/                  # Manual/original test packets
│   └── packet1.inp           # Example packet file
├── packet_generators/        # Packet generation tools
│   ├── generate_packets_graph1.cc
│   ├── Makefile
│   └── README.md             # Generator documentation
└── generated_packets/        # Auto-generated test cases
    └── graph1/
        ├── graph1_case1.inp
        ├── graph1_case2.inp
        └── ...
```

## Test Packet Generation

The system includes packet generators for creating random test cases:

### Building Generators
```bash
cd packet_generators
make
```

### Generating Test Cases
```bash
# Generate 10 test cases with 15 packets each for graph1
./generate_packets_graph1 10 15

# Generate with specific seed for reproducibility
./generate_packets_graph1 10 15 42
```

See `packet_generators/README.md` for detailed documentation.

## Algorithm Placeholders

Currently, all four algorithms are placeholders that print a message. They are ready to be implemented with actual planning logic:

1. **Algorithm 1**: Placeholder for Algorithm 1
2. **Algorithm 2**: Placeholder for Algorithm 2
3. **Algorithm 3**: Placeholder for Algorithm 3
4. **Algorithm 4**: Placeholder for Algorithm 4

## Example Usage

```cpp
#include "include/Planifier.hh"
#include "include/Robot.hh"
#include "../01_layer_mapping/include/Graph.hh"

int main() {
    // Create warehouse graph
    Graph warehouse;
    warehouse.addNode(0, Graph::NodeType::Charging, 0, 0);
    warehouse.addNode(1, Graph::NodeType::Pickup, 10, 0);
    warehouse.addNode(2, Graph::NodeType::Dropoff, 20, 0);
    warehouse.addEdge(0, 1);
    warehouse.addEdge(1, 2);
    
    // Create planner with 3 robots
    Planifier planner(warehouse, 3);
    
    // Run planning (interactive mode)
    planner.plan();
    
    // Or run specific algorithm
    planner.plan(2);  // Algorithm 2
    
    return 0;
}
```

## Current Test

The current `main.cc` creates a simple test graph with:
- 1 Charging station at (0, 0)
- 1 Pickup point at (10, 0)
- 1 Dropoff point at (20, 0)
- 1 Waypoint at (30, 0)
- Edges connecting them

It then creates a planner with 2 robots and runs in interactive mode.

## Next Steps

- Implement actual planning algorithms (1-4)
- Add robot initialization and management
- Implement task assignment logic
- Add path planning integration
- Connect with Layer 3 (Multi-Agent Pathfinding)
