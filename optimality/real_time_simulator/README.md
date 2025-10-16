# Real-Time Warehouse Simulation

## Overview

This is the **online mode** of the warehouse robot system. It provides a dynamic, real-time simulation where tasks can be added interactively while the simulation is running.

## Architecture

### Design Philosophy

This layer follows a **non-invasive architecture**:
- **Does NOT modify** the existing `02_layer_planner` code
- **Reuses** the Planifier as a library for state management and scheduling
- **Adds** a new simulation layer on top of the existing components

### Components

```
real_time_simulator/
├── include/
│   └── SimulationController.hh    # Main controller class
├── src/
│   └── SimulationController.cc    # Implementation
├── main.cc                        # Entry point
└── Makefile                       # Build configuration
```

### Key Classes

#### SimulationController
The heart of the real-time simulation. Responsibilities:
- **Main simulation loop**: Runs continuously in `while(true)`
- **Time management**: Tracks simulation time and delta time
- **Robot updates**: Updates robot positions, battery levels, and states
- **Scheduler orchestration**: Calls Planifier's scheduling methods at intervals
- **User input handling**: Processes commands in a separate thread
- **Thread-safe task management**: Allows adding tasks during runtime

#### Planifier (from Layer 2)
Used as a **library** for:
- Managing robot queues (available, busy, charging)
- Managing task queues (pending)
- Executing scheduling algorithms (BruteForce, Greedy, HillClimbing)
- Computing optimal/heuristic task assignments

## Usage

### Building

```bash
cd optimality/real_time_simulator
make
```

### Running

```bash
# Basic usage: graph 1, 5 robots, no initial tasks
./build/simulator 1

# With custom number of robots
./build/simulator 1 10

# With initial tasks from test case
./build/simulator 1 5 1

# General format
./build/simulator <graphId> [numRobots] [testCase]
```

### Interactive Commands

Once the simulation is running, you can use these commands:

| Command | Description | Example |
|---------|-------------|---------|
| `help`, `h` | Show available commands | `help` |
| `status`, `s` | Display current simulation status | `status` |
| `pause`, `p` | Pause/resume simulation | `pause` |
| `quit`, `q`, `exit` | Stop simulation and exit | `quit` |
| `add <id> <from> <to>` | Add a new task | `add 100 5 12` |
| `algorithm <1-3>` | Change scheduling algorithm | `algorithm 2` |
| `interval <seconds>` | Set scheduler run interval | `interval 10` |

### Algorithm Choices

1. **BruteForce**: Exhaustive search for optimal solution (slow for many tasks)
2. **Greedy**: Fast heuristic algorithm (default)
3. **HillClimbing**: Iterative improvement heuristic

## Configuration

### Simulation Parameters

You can modify these in `SimulationController.cc`:

- `timeStep`: Time increment per update cycle (default: 0.1s)
- `schedulerInterval`: How often to run scheduler (default: 5.0s)

### Example Session

```
$ ./build/simulator 1 5 1
Loading graph 1...
Graph loaded: 50 nodes
Loading tasks from test case 1...
Loaded 15 tasks

========================================
  REAL-TIME WAREHOUSE SIMULATION
========================================
Graph: 50 nodes
Robots: 5
Initial Tasks: 15
Time Step: 0.1s
Scheduler Interval: 5.0s

Type 'help' for available commands.
========================================

[T=5.0s] ========== SCHEDULER CYCLE 1 ==========
=== Algorithm: Greedy ===
...
[T=5.0s] Scheduler cycle complete. Makespan: 120.5s
========================================

> status
--- Simulation Status ---
Time: 10.25s
Robots - Available: 2 | Busy: 3 | Charging: 0
Tasks - Pending: 10 | Completed: 5
Scheduler Cycles: 2
Status: RUNNING
-------------------------

> add 100 5 12
[T=10.3s] New task added: 100 (5 -> 12)

> algorithm 3
Switched to Hill Climbing algorithm

> quit
Stopping simulation...
```

## Implementation Notes

### Thread Safety

- **Input thread**: Handles user commands asynchronously
- **Main thread**: Runs simulation loop
- **Mutexes**: Protect shared state (task queue, status printing)

### Planifier Integration

The Planifier is used as a **state manager and scheduling engine**:
- Maintains robot and task queues
- Executes scheduling algorithms
- Returns assignment results

The SimulationController:
- Calls Planifier methods periodically
- Does NOT modify Planifier's internal logic
- Adds a temporal dimension (simulation time)

### Future Enhancements

#### TODO: Full Robot State Updates

The current `updateRobots()` method is a placeholder. A complete implementation would:

1. **Robot Movement**
   - Move robots along their assigned paths
   - Update positions based on velocity and time
   - Handle path following and waypoint reaching

2. **Battery Management**
   - Decrease battery during movement
   - Charge robots when battery is low
   - Move robots between queues based on battery state

3. **Task Completion**
   - Detect when robots reach destination nodes
   - Mark tasks as completed
   - Free robots and return them to available queue

4. **Queue Management**
   - Move robots: busy → available (task complete)
   - Move robots: available → charging (low battery)
   - Move robots: charging → available (fully charged)

#### Suggested Planifier Enhancements

To fully support real-time simulation, consider adding to Planifier:

```cpp
// Add methods for dynamic state management
void addPendingTask(const Task& task);
void markTaskCompleted(int taskId);
void updateRobotPosition(int robotId, pair<double, double> position);
void updateRobotBattery(int robotId, double batteryLevel);
Robot& getRobotById(int robotId);
```

## Relationship to Other Layers

```
01_layer_mapping/     → Graph representation and algorithms
       ↓ (uses)
02_layer_planner/     → Task scheduling and robot assignment
       ↓ (library)
real_time_simulator/  → Dynamic simulation with time progression
```

This simulator demonstrates how Layer 2 (planner) can be used as a library component in a larger system without modification.

## Makefile Targets

```bash
make              # Build the simulator
make run          # Build and run with defaults
make run-test     # Build and run with test case
make clean        # Remove build artifacts
make help         # Show detailed help
```

## Design Decisions

### Why Not Modify Planifier?

- **Separation of concerns**: Planifier focuses on scheduling, not simulation
- **Reusability**: Planifier can be used in other contexts (batch processing, testing)
- **Maintainability**: Changes to simulation don't affect core scheduling logic
- **Testing**: Each layer can be tested independently

### Why a New Directory?

- **Clear boundaries**: Real-time simulation is a distinct operational mode
- **Independent evolution**: Can add features without touching planner
- **Multiple applications**: Could have batch mode, real-time mode, visualization mode, etc.

## Contributing

When adding features to the real-time simulator:

1. ✅ **DO**: Add new methods to SimulationController
2. ✅ **DO**: Create helper classes in this directory
3. ✅ **DO**: Add configuration options for simulation behavior
4. ❌ **DON'T**: Modify 02_layer_planner's existing code
5. ❌ **DON'T**: Change Planifier's main.cc
6. ✅ **CONSIDER**: Proposing extensions to Planifier if needed for all applications

---

**Author**: Mecalux AMR Team  
**Date**: 2025  
**Layer**: 03 - Real-Time Simulation  
