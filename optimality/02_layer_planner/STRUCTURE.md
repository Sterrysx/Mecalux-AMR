# Layer 02: Planner - Refactored Structure

## Directory Organization

```
02_layer_planner/
├── src/                    # Source files (.cc)
│   ├── Planifier.cc       # Main planning logic
│   ├── Robot.cc           # Robot class implementation
│   └── Task.cc            # Task class implementation
├── include/                # Header files (.hh)
│   ├── Planifier.hh       # Planifier interface
│   ├── Robot.hh           # Robot interface
│   └── Task.hh            # Task interface
├── build/                  # Object files (.o) - generated
│   ├── Graph.o
│   ├── Planifier.o
│   ├── Robot.o
│   ├── Task.o
│   └── main.o
├── tests/                  # Test case files
│   ├── graph1/            # Test cases for graph 1
│   │   ├── graph1_case1.inp
│   │   ├── graph1_case2.inp
│   │   └── ...
│   └── graph2/            # Test cases for graph 2
│       ├── graph2_case1.inp
│       └── ...
├── utils/                  # Task generation utilities
│   ├── src/               # Generator source files
│   │   ├── generate_tasks_graph1.cc
│   │   └── generate_tasks_graph2.cc
│   ├── build/             # Compiled generators
│   │   ├── generate_tasks_graph1
│   │   └── generate_tasks_graph2
│   └── Makefile           # Utility build system
├── main.cc                 # Main entry point
├── planner                 # Compiled executable
└── Makefile                # Main build system
```

## Building

```bash
# Build the planner
make

# Clean build artifacts
make clean

# Clean everything including old directories
make clean-all
```

## Running

```bash
# Run with default settings (graph1, algorithm 1)
./planner

# Run with specific graph and algorithm
./planner 1 1    # graph1, brute force
./planner 2 2    # graph2, algorithm 2
```

## Generating Test Cases

```bash
# Navigate to utils directory
cd utils

# Build generators
make

# Generate fixed-size test cases
./build/generate_tasks_graph1 10 15          # 10 cases, 15 tasks each

# Generate incremental test cases
./build/generate_tasks_graph1 -i 10 10 10    # 10 cases: 10, 20, 30, ..., 100 tasks

# With reproducible seed
./build/generate_tasks_graph1 -i 5 5 5 999   # Seed=999 for reproducibility
```

## Classes

### Planifier
- **Purpose**: Manages robot fleet and executes planning algorithms
- **Key Members**:
  - `Graph G` - Warehouse graph
  - `queue<Robot> availableRobots` - Available robots
  - `queue<Robot> busyRobots` - Busy robots
  - `queue<Robot> chargingRobots` - Charging robots
  - `queue<Task> pendingTasks` - Tasks to be executed
  - `const int totalRobots` - Total number of robots
- **Key Methods**:
  - `plan(int algorithm)` - Execute planning algorithm
  - `bruteforce_algorithm()` - Brute force implementation (placeholder)

### Robot
- **Purpose**: Represents an individual AMR
- **Key Members**:
  - `int robotId` - Unique identifier
  - `pair<double, double> position` - Current position
  - `double batteryLevel` - Battery percentage
  - `int currentTask` - Current task ID (-1 if none)
  - `bool isAvailable` - Availability status

### Task
- **Purpose**: Represents a delivery task
- **Key Members**:
  - `int taskId` - Unique identifier
  - `int originNode` - Pickup node
  - `int destinationNode` - Dropoff node

## Algorithm Implementation Status

- ✅ **Algorithm 1 (Brute Force)**: Placeholder ready for implementation
- ⏳ **Algorithm 2**: Placeholder
- ⏳ **Algorithm 3**: Placeholder
- ⏳ **Algorithm 4**: Placeholder

## Next Steps

1. Implement `bruteforce_algorithm()` in `src/Planifier.cc`
2. Test with various graph/task combinations
3. Implement remaining algorithms (2-4)
4. Add performance metrics and logging
