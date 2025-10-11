# Strategy Pattern Implementation for Layer 02 Planner

## Overview

The Layer 02 Planner has been refactored to use the **Strategy Pattern** for algorithm selection. This design pattern allows for dynamic algorithm selection at runtime and makes it easy to add new planning algorithms without modifying existing code.

## Architecture

### UML Class Diagram

```
┌─────────────────┐          ┌─────────────────────┐
│   Planifier     │◇─────────│    Algorithm        │
├─────────────────┤          │   «interface»       │
│ - currentAlg    │          ├─────────────────────┤
│ - graph         │          │ + execute()         │
│ - robots        │          │ + getName()         │
│ - tasks         │          │ + getDescription()  │
├─────────────────┤          └─────────────────────┘
│ + plan()        │                     △
│ + executePlan() │                     │
│ + setAlgorithm()│          ┌──────────┴──────────┐
└─────────────────┘          │                     │
                   ┌─────────┴─────────┐ ┌─────────┴─────────┐
                   │ BruteForceAlgorithm │ GreedyAlgorithm   │
                   ├─────────────────────┤ ├─────────────────┤
                   │ + execute()         │ │ + execute()     │
                   └─────────────────────┘ └─────────────────┘
                             │                     │
                   ┌─────────┴─────────┐ ┌─────────┴─────────┐
                   │ PriorityAlgorithm │ │ HybridAlgorithm   │
                   ├─────────────────────┤ ├─────────────────┤
                   │ + execute()         │ │ + execute()     │
                   └─────────────────────┘ └─────────────────┘
```

## Directory Structure

```
optimality/02_layer_planner/
├── include/
│   ├── Algorithm.hh              # Abstract base class (Strategy interface)
│   ├── BruteForceAlgorithm.hh    # Concrete strategy 1
│   ├── GreedyAlgorithm.hh        # Concrete strategy 2
│   ├── PriorityAlgorithm.hh      # Concrete strategy 3
│   ├── HybridAlgorithm.hh        # Concrete strategy 4
│   ├── Planifier.hh              # Context class (uses strategies)
│   ├── Robot.hh
│   └── Task.hh
├── src/
│   ├── BruteForceAlgorithm.cc    # Implementation of brute force strategy
│   ├── GreedyAlgorithm.cc        # Implementation of greedy strategy
│   ├── PriorityAlgorithm.cc      # Implementation of priority strategy
│   ├── HybridAlgorithm.cc        # Implementation of hybrid strategy
│   ├── Planifier.cc              # Refactored to use strategies
│   ├── Robot.cc
│   └── Task.cc
├── build/                        # Compiled object files
├── tests/                        # Test case files
└── utils/                        # Generator utilities
```

## Components

### 1. Algorithm (Strategy Interface)

**File:** `include/Algorithm.hh`

Abstract base class that defines the interface for all planning algorithms.

**Key Methods:**
- `execute()` - Pure virtual method that implements the algorithm logic
- `getName()` - Returns the algorithm name
- `getDescription()` - Returns a brief description

### 2. Concrete Algorithms (Concrete Strategies)

Each algorithm inherits from `Algorithm` and provides its own implementation:

#### BruteForceAlgorithm
- **Purpose:** Explores all possible task assignments to find optimal solution
- **Trade-off:** Guarantees optimal solution but computationally expensive
- **File:** `include/BruteForceAlgorithm.hh`, `src/BruteForceAlgorithm.cc`

#### GreedyAlgorithm
- **Purpose:** Assigns tasks to the nearest available robot
- **Trade-off:** Fast but may not find optimal solution
- **File:** `include/GreedyAlgorithm.hh`, `src/GreedyAlgorithm.cc`

#### PriorityAlgorithm
- **Purpose:** Assigns tasks based on priority ordering
- **Trade-off:** Good for time-sensitive tasks
- **File:** `include/PriorityAlgorithm.hh`, `src/PriorityAlgorithm.cc`

#### HybridAlgorithm
- **Purpose:** Combines multiple strategies for balanced performance
- **Trade-off:** Balances speed and solution quality
- **File:** `include/HybridAlgorithm.hh`, `src/HybridAlgorithm.cc`

### 3. Planifier (Context)

**File:** `include/Planifier.hh`, `src/Planifier.cc`

The context class that uses algorithms. Has been refactored to:
- Hold a reference to the current algorithm strategy
- Provide `setAlgorithm()` to change strategies at runtime
- Call the algorithm's `execute()` method through `executePlan()`

**Key Changes:**
- **Before:** Used `switch` statement in `plan()` method
- **After:** Uses polymorphism with `unique_ptr<Algorithm>`

## Usage

### Basic Usage

```cpp
// Create planner
Planifier planner(graph, numRobots, tasks);

// Option 1: Use plan() with algorithm selection
planner.plan(1);  // Uses BruteForce

// Option 2: Set algorithm manually then execute
planner.setAlgorithm(std::make_unique<GreedyAlgorithm>());
planner.executePlan();
```

### Command-Line Usage

```bash
# Interactive mode (prompts for algorithm selection)
./planner

# Direct algorithm selection
./planner <graph_number> <algorithm>

# Examples:
./planner 1 1    # Graph 1, BruteForce
./planner 2 2    # Graph 2, Greedy
./planner 3 3    # Graph 3, Priority
./planner 5 4    # Graph 5, Hybrid
```

### Algorithm Selection Menu

When running in interactive mode (algorithm = 0):
```
=== Algorithm Selection ===
Available algorithms:
  1. Brute Force - Explores all possible assignments
  2. Greedy - Nearest available robot heuristic
  3. Priority - Task priority-based assignment
  4. Hybrid - Combines multiple strategies
Choose an algorithm (1-4):
```

## Benefits of Strategy Pattern

1. **Open/Closed Principle:** Easy to add new algorithms without modifying existing code
2. **Single Responsibility:** Each algorithm class has one job
3. **Runtime Flexibility:** Can change algorithms dynamically
4. **Testability:** Each algorithm can be tested independently
5. **Maintainability:** Algorithm logic is isolated and easy to understand

## Adding New Algorithms

To add a new planning algorithm:

1. **Create header file** `include/NewAlgorithm.hh`:
```cpp
#ifndef NEW_ALGORITHM_HH
#define NEW_ALGORITHM_HH

#include "Algorithm.hh"

class NewAlgorithm : public Algorithm {
public:
    void execute(...) override;
    std::string getName() const override;
    std::string getDescription() const override;
};

#endif
```

2. **Create implementation** `src/NewAlgorithm.cc`:
```cpp
#include "../include/NewAlgorithm.hh"

void NewAlgorithm::execute(...) {
    // Your algorithm logic here
}

std::string NewAlgorithm::getName() const {
    return "New Algorithm";
}

std::string NewAlgorithm::getDescription() const {
    return "Description of what this algorithm does";
}
```

3. **Update Planifier.cc** to include the new algorithm:
```cpp
#include "../include/NewAlgorithm.hh"

// In plan() method, add new case:
case 5:
    setAlgorithm(make_unique<NewAlgorithm>());
    break;
```

4. **Update Makefile** to compile the new files

5. **Rebuild:**
```bash
make clean && make
```

## Build Instructions

```bash
# Clean build
make clean

# Build
make

# Run
./planner [graph_number] [algorithm]
```

## Future Enhancements

- [ ] Add algorithm benchmarking/comparison
- [ ] Implement actual algorithm logic (currently placeholders)
- [ ] Add algorithm-specific parameters
- [ ] Create algorithm factory for easier instantiation
- [ ] Add metrics collection (execution time, solution quality)
- [ ] Implement algorithm composition/chaining

## References

- **Design Pattern:** Strategy Pattern (Gang of Four)
- **C++ Best Practices:** Smart pointers (`unique_ptr`) for ownership
- **SOLID Principles:** Open/Closed, Single Responsibility
