# System Architecture Overview

## Layer Structure

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  03_real_time_simulator/  (NEW - Dynamic Online Mode)         │
│  ┌───────────────────────────────────────────────────────┐    │
│  │                                                         │    │
│  │  main.cc  ──→  SimulationController                   │    │
│  │                      │                                  │    │
│  │                      ├─→ while(true) loop              │    │
│  │                      ├─→ Time management               │    │
│  │                      ├─→ Input thread (commands)       │    │
│  │                      └─→ Calls Planifier methods       │    │
│  │                                                         │    │
│  └─────────────────────────────┬───────────────────────────┘    │
│                                │ uses                           │
└────────────────────────────────┼────────────────────────────────┘
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  02_layer_planner/  (Library - Scheduling Engine)             │
│  ┌───────────────────────────────────────────────────────┐    │
│  │                                                         │    │
│  │  Planifier  (State Manager)                           │    │
│  │    │                                                    │    │
│  │    ├─→ Robot queues (available, busy, charging)       │    │
│  │    ├─→ Task queue (pending)                            │    │
│  │    └─→ Algorithm executor                              │    │
│  │                                                         │    │
│  │  Algorithms (Strategy Pattern)                         │    │
│  │    ├─→ BruteForce (optimal)                            │    │
│  │    ├─→ Greedy (fast heuristic)                         │    │
│  │    └─→ HillClimbing (iterative)                        │    │
│  │                                                         │    │
│  │  Models                                                 │    │
│  │    ├─→ Robot (id, position, battery, task)            │    │
│  │    └─→ Task (id, origin, destination)                  │    │
│  │                                                         │    │
│  └─────────────────────────────┬───────────────────────────┘    │
│                                │ uses                           │
└────────────────────────────────┼────────────────────────────────┘
                                 ▼
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  01_layer_mapping/  (Foundation - Graph Representation)        │
│  ┌───────────────────────────────────────────────────────┐    │
│  │                                                         │    │
│  │  Graph                                                  │    │
│  │    ├─→ Nodes (waypoints, charging, pickup, dropoff)   │    │
│  │    ├─→ Edges (connections, distances, speeds)         │    │
│  │    ├─→ Shortest path algorithms                        │    │
│  │    └─→ File I/O (load/save)                            │    │
│  │                                                         │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Real-Time Simulator Data Flow

```
User Input Thread                Main Simulation Thread
─────────────────               ────────────────────────

  ┌──────────┐
  │  stdin   │
  └────┬─────┘
       │
       ▼
  ┌──────────┐                   ┌──────────────┐
  │  Parse   │                   │ Simulation   │
  │ Command  │                   │    Loop      │
  └────┬─────┘                   └──────┬───────┘
       │                                │
       ├─ help ────────────────→ Print Help
       │                                │
       ├─ status ──────────────→ Print Status
       │                                │
       ├─ pause ───────────────→ Toggle Pause
       │                                │
       ├─ algorithm ───────────→ Set Algorithm
       │                                │
       ├─ add task ────┐                │
       │                │                │
       ▼                │                ▼
  [Task Queue] ◄───────┘      ┌──────────────────┐
  (mutex lock)                │  Update Robots   │
       │                      │  - Move          │
       │                      │  - Battery       │
       │                      │  - Completion    │
       │                      └────────┬─────────┘
       │                               │
       │                               ▼
       │                      ┌──────────────────┐
       │                      │ Scheduler Check  │
       │                      │ (every N sec)    │
       │                      └────────┬─────────┘
       │                               │
       └──────────────────────────────→│
                                       ▼
                              ┌──────────────────┐
                              │  Call Planifier  │
                              │  .executePlan()  │
                              └────────┬─────────┘
                                       │
                                       ▼
                              ┌──────────────────┐
                              │ Algorithm Result │
                              │  - Assignment    │
                              │  - Makespan      │
                              └──────────────────┘
```

## Thread Safety

```
Main Thread                     Input Thread
───────────                    ─────────────

while (running) {              while (running) {
                                 read command
  update robots                    │
                                   │
  if (time to schedule) {          │
    ┌─────────────┐                │
    │  Lock Mutex │←───────────────┘
    ├─────────────┤
    │ Get Tasks   │
    │ Run Planner │
    │ Get Result  │
    ├─────────────┤
    │Unlock Mutex │
    └─────────────┘
  }
  
  sleep(timeStep)                sleep(100ms)
}                              }
```

## Key Design Decisions

### 1. Non-Invasive Integration
- Real-time simulator is a **new application**
- Planifier is used as a **library**
- No modifications to existing 02_layer_planner code
- Original main.cc remains untouched

### 2. Separation of Concerns
- **Layer 1**: Graph structure and pathfinding
- **Layer 2**: Task scheduling and optimization
- **Layer 3**: Real-time execution and interactivity

### 3. Strategy Pattern
- Algorithms are interchangeable
- Can switch during runtime
- Each algorithm implements same interface

### 4. Thread-Safe Design
- Mutex protects shared state
- Lock only during critical sections
- Non-blocking input handling

## File Dependencies

```
real_time_simulator/
├── main.cc
│   └─→ requires: SimulationController.hh, Graph.hh, Task.hh
│
├── src/SimulationController.cc
│   └─→ requires: SimulationController.hh
│                 Planifier.hh
│                 algorithms/*.hh
│                 Robot.hh, Task.hh, Graph.hh
│
└── include/SimulationController.hh
    └─→ requires: Planifier.hh, Graph.hh, Task.hh, Robot.hh
```

## Compilation Chain

```
Source Files                Object Files              Executable
────────────               ────────────              ──────────

main.cc                 ─→ main.o                 ┐
SimulationController.cc ─→ SimulationController.o │
                                                   │
Planifier.cc            ─→ Planifier.o            │
Robot.cc                ─→ Robot.o                │
Task.cc                 ─→ Task.o                 ├─→ simulator
01_BruteForce.cc        ─→ 01_BruteForce.o        │
02_Greedy.cc            ─→ 02_Greedy.o            │
03_HillClimbing.cc      ─→ 03_HillClimbing.o      │
SchedulerUtils.cc       ─→ SchedulerUtils.o       │
TSPSolver.cc            ─→ TSPSolver.o            │
AssignmentPrinter.cc    ─→ AssignmentPrinter.o    │
                                                   │
Graph.cc                ─→ Graph.o                ┘
```

## Usage Flow

```
1. User launches:
   ./build/simulator <graphId> [numRobots] [testCase]
   
2. main.cc:
   - Parses arguments
   - Loads Graph from file
   - Loads initial Tasks (if testCase provided)
   - Creates SimulationController
   
3. SimulationController.start():
   - Spawns input thread
   - Enters main loop
   
4. Main Loop (continuous):
   while (running) {
     - Update simulation time
     - Update robots (position, battery, tasks)
     - Check if scheduler interval elapsed
     - If yes: run scheduler cycle
     - Sleep for timeStep
   }
   
5. Input Thread (parallel):
   while (running) {
     - Wait for user input
     - Parse command
     - Execute action (add task, change algo, etc.)
     - Continue
   }
   
6. Scheduler Cycle (periodic):
   - Lock task queue
   - Call Planifier.executePlan()
   - Algorithm runs and returns result
   - Assignment printed
   - Unlock
   
7. User quits:
   - Sets running = false
   - Main loop exits
   - Input thread joins
   - Cleanup and exit
```

## Extension Points

To add new features to the simulator:

### 1. Add New Commands
- Update `processCommand()` method
- Add case to command parser
- Implement command handler

### 2. Add Robot Behaviors
- Implement in `updateRobots()`
- Access robot queues via Planifier
- Update positions, battery, states

### 3. Add Visualization
- Create new class: `SimulationVisualizer`
- Hook into main loop
- Render state at each step

### 4. Add Metrics/Logging
- Create new class: `SimulationLogger`
- Record events and statistics
- Export to file or database

### 5. Add Network Interface
- Create new class: `SimulationServer`
- Expose REST API or WebSocket
- Allow remote monitoring/control

---

**Note**: All extensions should be added to the real_time_simulator layer, keeping the existing layers (01 and 02) unchanged.
