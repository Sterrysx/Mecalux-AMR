# Layer 2: Planner

## Overview
This layer implements the planning algorithms for the multi-robot warehouse system. It uses the Graph data structure from Layer 1 (01_layer_mapping) and manages Robot instances.

## Components

### Robot Class (`Robot.hh` / `Robot.cc`)
Represents an individual robot with the following properties:
- **ID**: Unique identifier
- **Position**: Current coordinates (x, y)
- **Battery Level**: Current battery percentage
- **Current Task**: Task being executed
- **Max Speed**: Maximum velocity
- **Load Capacity**: Maximum weight capacity
- **Availability**: Whether the robot is available for tasks

### Planifier Class (`Planifier.hh` / `Planifier.cc`)
Manages the fleet of robots and executes planning algorithms:
- **Graph**: Warehouse map (from Layer 1)
- **Robot Queues**: Available, busy, and charging robots
- **Planning Algorithms**: Multiple strategies (1-4)

## Building and Running

### Compile
```bash
make
```

### Run (Interactive Mode)
```bash
make run
# or
./planner
```

You will be prompted to choose an algorithm (1-4):
```
=== Algorithm Selection ===
Available algorithms:
  1. Algorithm 1
  2. Algorithm 2
  3. Algorithm 3
  4. Algorithm 4
Choose an algorithm (1-4):
```

### Run with Specific Algorithm
Modify `main.cc` to call `P.plan(n)` where n is 1-4:
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
├── Makefile              # Build configuration
├── main.cc               # Main entry point
├── README.md             # This file
├── include/
│   ├── Planifier.hh      # Planner class header
│   └── Robot.hh          # Robot class header
├── src/
│   ├── Planifier.cc      # Planner implementation
│   └── Robot.cc          # Robot implementation
└── obj/                  # Compiled object files (generated)
```

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
