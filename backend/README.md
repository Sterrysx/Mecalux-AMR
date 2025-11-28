# Mecalux AMR Backend

<p align="center">
  <img src="UMLMecalux.svg" alt="Architecture Diagram" width="800"/>
</p>

## Overview

The **Mecalux AMR Backend** is a centralized "brain" for a dynamic Multi-Robot Task Allocation and Navigation System designed for logistics warehouses. It coordinates a fleet of Autonomous Mobile Robots (AMRs) to efficiently execute pickup and dropoff tasks while avoiding collisions in real-time.

### Key Features

- 🤖 **Multi-Robot Fleet Management** - Coordinate 1-100+ robots simultaneously
- 📦 **Dynamic Task Allocation** - Continuously optimize task assignments as new orders arrive
- 🗺️ **Dual-Layer Mapping** - Static NavMesh for planning, Dynamic Bitmap for navigation
- 🚧 **Real-Time Collision Avoidance** - ORCA-based multi-agent collision prevention
- ⚡ **Reactive Scheduling** - Three scheduling scenarios for different task arrival patterns
- 🔄 **Hot Task Injection** - Add new tasks without stopping the system

---

## Architecture

The system follows a **three-layer architecture** that divides responsibilities:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           FLEET MANAGER                                      │
│                     (System Orchestrator)                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐             │
│   │   LAYER 1       │  │   LAYER 2       │  │   LAYER 3       │             │
│   │   Mapping       │  │   Planning      │  │   Physics       │             │
│   │                 │  │                 │  │                 │             │
│   │ • StaticBitMap  │  │ • VRP Solver    │  │ • RobotDriver   │             │
│   │ • InflatedBitMap│  │ • RobotAgent    │  │ • Theta* Paths  │             │
│   │ • DynamicBitMap │  │ • CostMatrix    │  │ • ORCA Avoidance│             │
│   │ • NavMesh       │  │ • TaskLoader    │  │ • FastLoop      │             │
│   │ • POIRegistry   │  │                 │  │                 │             │
│   └─────────────────┘  └─────────────────┘  └─────────────────┘             │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Layer Responsibilities

| Layer | Name | Role | Frequency |
|-------|------|------|-----------|
| **Layer 1** | Mapping | Static infrastructure (NavMesh, POIs) | Offline |
| **Layer 2** | Planning | Task assignment (VRP solver) | 1 Hz |
| **Layer 3** | Physics | Path execution (Theta*, ORCA) | 20 Hz |

---

## Layer 1: Mapping

Layer 1 provides **two views of the world** for different purposes.

### Components

#### 1. StaticBitMap
The original warehouse map as a 2D occupancy grid.
- **Resolution**: 0.1m per pixel (DECIMETERS)
- **Format**: Text file where `.` = walkable, `#` = obstacle
- **Size**: Typically 1600×600 pixels (160m × 60m)

```cpp
// Load map from file
StaticBitMap map = StaticBitMap::CreateFromFile("map_layout.txt", Resolution::DECIMETERS);
```

#### 2. InflatedBitMap
Safety map with robot clearance added around obstacles.
- **Inflation**: Adds robot radius margin around all obstacles
- **Purpose**: Ensures paths have clearance for robot body
- **Used by**: Layer 3 (Theta* pathfinding)

```cpp
// Create safety map with 0.3m robot radius
InflatedBitMap inflated(staticMap, 0.3f, Resolution::DECIMETERS);
```

#### 3. DynamicBitMap
Live-updated map with temporary obstacles.
- **Updated**: 1 Hz by ObstacleLoop
- **Contains**: Dropped boxes, temporary blockages
- **Used by**: Layer 3 for real-time navigation

#### 4. NavMesh
High-level graph for strategic planning.
- **Type**: Uniform grid of tiles (0.5m × 0.5m)
- **Size**: ~25,000 nodes for standard warehouse
- **Purpose**: Fast A* queries for cost matrix
- **Used by**: Layer 2 (VRP solver)

```cpp
// Generate NavMesh from static map
NavMeshGenerator generator;
NavMesh mesh = generator.GenerateUniformGrid(staticMap, Resolution::DECIMETERS);
```

#### 5. POIRegistry
Points of Interest configuration.
- **Types**: CHARGING, PICKUP, DROPOFF
- **Format**: JSON configuration file
- **Mapping**: Each POI mapped to nearest NavMesh node

```json
{
  "pois": [
    {"id": "C1", "type": "CHARGING", "x": 1467, "y": 207},
    {"id": "P1", "type": "PICKUP", "x": 100, "y": 150},
    {"id": "D1", "type": "DROPOFF", "x": 500, "y": 300}
  ]
}
```

---

## Layer 2: Planning

Layer 2 is the **Strategic Brain** - it decides which robot does which task.

### The VRP Problem

We solve a variant of the **Vehicle Routing Problem (VRP)**:

| Variant | Description |
|---------|-------------|
| **Capacitated** | Each robot carries 1 packet at a time |
| **Dynamic** | Tasks arrive continuously |
| **Heterogeneous** | Robots start at different charging stations |

### Components

#### 1. CostMatrixProvider
Precomputes travel costs between all POI nodes.

```cpp
// Precompute N×N cost matrix (one-time, offline)
CostMatrixProvider costs(navMesh);
costs.PrecomputeAllCosts(poiNodes);  // O(N² × (E + V·log(V)))

// Query cost (instant, online)
float cost = costs.GetCost(nodeA, nodeB);
```

#### 2. VRP Solvers (Strategy Pattern)

Three metaheuristic algorithms available:

| Algorithm | Best For | Time Complexity |
|-----------|----------|-----------------|
| **Tabu Search** | Large instances (100+ tasks) | O(iterations × R × T) |
| **Simulated Annealing** | Escaping local optima | O(iterations × R × T) |
| **Hill Climbing** | Fast, simple cases | O(iterations × R × T) |

```cpp
// Strategy pattern - swap algorithms easily
std::unique_ptr<IVRPSolver> solver = std::make_unique<TabuSearch>();
VRPResult result = solver->Solve(tasks, robots, costMatrix);
```

#### 3. RobotAgent
Logical representation of each robot for planning.

```cpp
struct RobotAgent {
    int robotId;
    int currentNode;
    RobotStatus status;          // IDLE, MOVING, CARRYING
    std::vector<int> itinerary;  // Ordered goal nodes
    int batteryLevel;
};
```

#### 4. Task
A pickup-dropoff job.

```cpp
struct Task {
    int taskId;
    int sourceNode;       // Pickup location (NavMesh node ID)
    int destinationNode;  // Dropoff location (NavMesh node ID)
    TaskStatus status;    // PENDING, ASSIGNED, IN_PROGRESS, COMPLETED
};
```

### VRP Output

The solver produces **itineraries** - ordered lists of nodes for each robot:

```
Robot 0: [P5 → D5 → P12 → D12 → C1]  (pickup, drop, pickup, drop, charge)
Robot 1: [P3 → D3 → P7 → D7 → P9 → D9]
```

---

## Layer 3: Physics

Layer 3 is the **Tactical Brain** - it executes goals with real-time collision avoidance.

### Components

#### 1. RobotDriver
Per-robot controller that follows the itinerary.

**States:**
- `IDLE` - No goal, stationary
- `COMPUTING_PATH` - Waiting for Theta* path
- `MOVING` - Following path
- `ARRIVED` - Reached goal node
- `COLLISION_WAIT` - Waiting for obstacle to clear

```cpp
// Set next goal from itinerary
driver.SetGoal(nextNodeId, pathfindingService);

// Called every physics tick (50ms)
driver.UpdateLoop(deltaTime, neighbors);
```

#### 2. ThetaStarSolver
Any-angle pathfinding algorithm.

**Advantages over A*:**
- Produces smooth paths (not grid-aligned)
- Shorter path lengths
- More natural robot movement

```cpp
// Find smooth path from A to B
PathResult result = thetaStar.FindPath(start, goal, safetyMap);
// Result: [A → (intermediate waypoints) → B]
```

#### 3. PathfindingService
Manages pathfinding requests with a thread pool.

**Features:**
- Priority queue (blocked robots get priority)
- Thread pool for parallel computation
- Request deduplication

#### 4. ORCASolver
Optimal Reciprocal Collision Avoidance.

**Algorithm:**
1. Get preferred velocity from path
2. Detect nearby obstacles (robots + static)
3. Compute velocity that avoids all collisions
4. Apply smoothing for natural movement

```cpp
// Every 50ms
Vector2 preferred = GetPathDirection() * maxSpeed;
Vector2 safe = orca.CalculateSafeVelocity(myData, neighbors, preferred);
ApplyVelocity(safe);
```

#### 5. FastLoopManager
High-frequency physics loop (20 Hz).

```cpp
void FastLoopManager::Tick(float dt) {
    for (RobotDriver& robot : robots_) {
        // Gather neighbors within 5m radius
        auto neighbors = GetNeighbors(robot, 50.0);  // 5m in pixels
        
        // Update each robot (path following + ORCA)
        robot.UpdateLoop(dt, neighbors);
    }
}
```

---

## The FleetManager (Orchestrator)

The `FleetManager` class owns all data and coordinates the three layers.

### Threading Model

```
┌─────────────────────────────────────────────────────────────────┐
│                     FLEET MANAGER THREADS                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌───────────────┐  │
│  │   MAIN THREAD    │  │  FLEET THREAD    │  │ OBSTACLE LOOP │  │
│  │      (1 Hz)      │  │    (20 Hz)       │  │    (1 Hz)     │  │
│  ├──────────────────┤  ├──────────────────┤  ├───────────────┤  │
│  │ • VRP solving    │  │ • Position update│  │ • Dynamic     │  │
│  │ • Task assignment│  │ • ORCA collision │  │   obstacles   │  │
│  │ • Goal dispatch  │  │ • Path following │  │ • Map refresh │  │
│  │ • Re-planning    │  │ • L2↔L3 sync     │  │               │  │
│  └──────────────────┘  └──────────────────┘  └───────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### L2 ↔ L3 Bridge

The FleetManager synchronizes state between Layer 2 (planning) and Layer 3 (physics):

```cpp
// When Layer 3 reports goal reached
void OnL3GoalReached(int robotId, int nodeId) {
    // Update Layer 2 agent state
    fleetRegistry_[robotId].PopItinerary();
    
    // Check if there's a next goal
    if (agent.HasNextGoal()) {
        int nextGoal = agent.GetNextGoal();
        drivers_[robotId]->SetGoal(nextGoal);  // Send to L3
    }
}
```

---

## Dynamic Scheduling Scenarios

The system supports three scenarios for task arrival patterns:

### Scenario A: Boot-Up (Full VRP Solve)

**When:** System starts with initial task queue

```
08:00 AM - System boots with 100 tasks
         ↓
    [VRP SOLVER] - Heavy computation (~100-200ms)
         ↓
    Assign itineraries to all robots
         ↓
    All robots request Theta* paths (staggered load)
```

### Scenario B: Streaming (Cheap Insertion)

**When:** ≤ 5 new tasks arrive (configurable threshold)

```
New Task Arrives
       ↓
  [CHEAP INSERTION] - O(R) where R = robots
       ↓
  Find robot with lowest insertion cost
       ↓
  Append to that robot's itinerary
       ↓
  No interruption to running robots
```

**Insertion Cost:**
```
cost = distance(robot_last_node → pickup) + distance(pickup → dropoff)
```

### Scenario C: Batch (Background Re-plan)

**When:** > 5 new tasks arrive at once

```
Large Batch Arrives (e.g., 10 tasks)
              ↓
    [BACKGROUND REPLAN] - std::async
              ↓
    VRP solver runs in separate thread
              ↓
    Robots continue current tasks
              ↓
    When complete: atomic swap of itineraries
```

**Smart Wait Logic:**
- If robot finishes during replan and `next_task_duration < replan_time`: **WAIT**
- If robot finishes during replan and `next_task_duration > replan_time`: **PROCEED**

---

## Building & Running

### Prerequisites

- C++17 compatible compiler (GCC 9+, Clang 10+)
- POSIX threads (pthread)
- Make

### Build

```bash
cd backend
make -j4
```

### Run Modes

```bash
# Interactive mode (press Enter to stop)
./build/fleet_manager

# Batch mode (max speed, auto-terminate when done)
./build/fleet_manager --batch

# Demo mode (demonstrates all scheduling scenarios)
./build/fleet_manager --demo

# Custom options
./build/fleet_manager --tasks ../api/custom_tasks.json --robots 8 --duration 60
```

### Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--tasks FILE` | Path to tasks JSON file | `../api/set_of_tasks.json` |
| `--robots N` | Number of robots (0 = auto) | 0 |
| `--duration S` | Run for S seconds | Until Enter |
| `--batch` | Batch mode (no sleep, auto-stop) | Off |
| `--demo` | Demo scheduling scenarios | Off |
| `--help` | Show help message | - |

---

## Configuration

### System Configuration (`system_config.json`)

```json
{
    "orca_tick_ms": 50,
    "warehouse_tick_ms": 100,
    "robot_radius_meters": 0.3,
    "robot_physical_width_meters": 0.6,
    "poi_config_path": "layer1/assets/poi_config.json"
}
```

### Task Format (`set_of_tasks.json`)

```json
{
    "format": "poi",
    "tasks": [
        {
            "id": 1,
            "pickup": "P5",
            "dropoff": "D12"
        },
        {
            "id": 2,
            "pickup": "P3",
            "dropoff": "D7"
        }
    ]
}
```

---

## Project Structure

```
backend/
├── main.cc                     # Entry point
├── Makefile                    # Build configuration
├── system_config.json          # Runtime configuration
│
├── include/
│   └── FleetManager.hh         # System orchestrator
│
├── src/
│   └── FleetManager.cc         # Orchestrator implementation
│
├── common/                     # Shared utilities
│   ├── include/
│   │   ├── Coordinates.hh      # 2D integer coordinates
│   │   └── Resolution.hh       # Map resolution enum
│   └── src/
│       ├── Coordinates.cc
│       └── Resolution.cc
│
├── layer1/                     # Mapping layer
│   ├── include/
│   │   ├── AbstractGrid.hh     # Grid interface
│   │   ├── StaticBitMap.hh     # Original map
│   │   ├── InflatedBitMap.hh   # Safety-padded map
│   │   ├── DynamicBitMap.hh    # Live-updated map
│   │   ├── NavMesh.hh          # Planning graph
│   │   ├── NavMeshGenerator.hh # Graph generation
│   │   └── POIRegistry.hh      # Points of interest
│   ├── src/
│   │   └── *.cc
│   └── assets/
│       ├── map_layout.txt      # Warehouse bitmap
│       └── poi_config.json     # POI definitions
│
├── layer2/                     # Planning layer
│   ├── include/
│   │   ├── IVRPSolver.hh       # Solver interface
│   │   ├── TabuSearch.hh       # Tabu search solver
│   │   ├── SimulatedAnnealing.hh
│   │   ├── HillClimbing.hh
│   │   ├── RobotAgent.hh       # Logical robot
│   │   ├── Task.hh             # Task definition
│   │   ├── TaskLoader.hh       # JSON parser
│   │   └── CostMatrixProvider.hh # Path costs
│   └── src/
│       └── *.cc
│
├── layer3/                     # Physics layer
│   ├── include/
│   │   ├── Core/
│   │   │   ├── RobotDriver.hh      # Robot controller
│   │   │   └── FastLoopManager.hh  # Physics loop
│   │   ├── Pathfinding/
│   │   │   ├── ThetaStarSolver.hh  # Any-angle paths
│   │   │   └── PathfindingService.hh
│   │   ├── Physics/
│   │   │   ├── ORCASolver.hh       # Collision avoidance
│   │   │   └── ObstacleData.hh
│   │   └── Vector2.hh              # 2D vector math
│   └── src/
│       └── *.cc
│
└── api/
    ├── APIService.hh           # File-based API
    └── APIService.cc
```

---

## API Integration

The backend provides a file-based API for visualization frontends:

### Output Files (`api/output/`)

| File | Content | Update Rate |
|------|---------|-------------|
| `robots.json` | Robot positions, velocities, states | 20 Hz |
| `tasks.json` | Task statuses | 1 Hz |
| `map.json` | Dynamic obstacle positions | 1 Hz |

### Example Robot State

```json
{
    "robots": [
        {
            "id": 0,
            "x": 1467,
            "y": 207,
            "vx": 15.2,
            "vy": -3.1,
            "state": "MOVING",
            "goal": 21852,
            "itinerary": [21852, 7872, 18665]
        }
    ],
    "timestamp": 1699800000.123
}
```

---

## Performance

### Benchmarks (100 tasks, 6 robots)

| Metric | Value |
|--------|-------|
| VRP Solve Time | ~100-200 ms |
| Theta* Path (average) | ~50 ms |
| Physics Tick | < 5 ms |
| Total Throughput | ~200 tasks/minute |

### Complexity Analysis

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Cost Matrix (offline) | O(N² × (E + V·log(V))) | One-time precomputation |
| VRP Solve | O(iterations × R × T) | Metaheuristic, configurable |
| Theta* Path | O(V² × log(V)) | Where V = grid nodes |
| ORCA (per robot) | O(N) | N = neighbors in radius |
| Fleet Loop Tick | O(R × N) | R robots, N avg neighbors |

---

## Extending the System

### Adding a New VRP Algorithm

1. Implement the `IVRPSolver` interface:

```cpp
class GeneticVRP : public IVRPSolver {
public:
    VRPResult Solve(
        const std::vector<Task>& tasks,
        std::vector<RobotAgent>& robots,
        const CostMatrixProvider& costs
    ) override;
};
```

2. Register in FleetManager initialization.

### Adding New POI Types

1. Update `POIRegistry.hh`:
```cpp
enum class POIType { CHARGING, PICKUP, DROPOFF, MAINTENANCE };
```

2. Add handling in TaskLoader and VRP solver.

---

## Troubleshooting

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| "No path found" | Start/end in obstacle | Check POI coordinates |
| Robots stuck | ORCA deadlock | Increase `slowdownDistance` |
| Slow VRP | Too many tasks | Increase `BATCH_THRESHOLD` |
| High CPU | 20Hz too fast | Reduce `orca_tick_ms` |

### Debug Output

Set environment variable for verbose logging:
```bash
export MECALUX_DEBUG=1
./build/fleet_manager
```

---

## License

Copyright © 2024 Mecalux. Internal use only.

---

## Authors

- Backend Architecture & Implementation
- AMR Algorithms Team

---

*For questions or issues, please contact the development team.*
