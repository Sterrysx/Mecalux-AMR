# Real-Time Simulator - Usage Examples

## Quick Test (Automated Demo)

The simulator has been tested and works successfully! Here's what was demonstrated:

### Test Run Output:
```
Loading graph 1...
Graph loaded: 17 nodes
Loading 10 tasks...
Loaded 10 tasks

========================================
  REAL-TIME WAREHOUSE SIMULATION
========================================
Graph: 17 nodes
Robots: 3
Initial Tasks: 10
Time Step: 0.1s
Scheduler Interval: 5s

Commands tested:
✓ status          - Showed simulation state
✓ add 100 2 8     - Added task dynamically during runtime
✓ add 101 5 12    - Added another task
✓ algorithm 3     - Switched to Hill Climbing algorithm
✓ quit            - Exited simulation

The scheduler automatically ran and assigned all 10 tasks to 3 robots,
calculating optimal paths and showing battery consumption!
```

## Manual Usage Examples

### Example 1: Start with Empty Queue
```bash
./build/simulator 1 5

# Then interactively add tasks:
> add 1 2 11
> add 2 4 13
> add 3 5 12
> status
> quit
```

### Example 2: Load Pre-configured Tasks
```bash
# Load graph 1 with 5 robots and 10 tasks
./build/simulator 1 5 10

# Watch the simulator work, then:
> status
> algorithm 2    # Switch to Greedy (if not already)
> pause          # Pause to examine
> status
> pause          # Resume
> quit
```

### Example 3: Larger Simulation
```bash
# Load graph 3 with 10 robots and 15 tasks
./build/simulator 3 10 15

# Adjust scheduler frequency:
> interval 2     # Run scheduler every 2 seconds
> status
> quit
```

### Example 4: Algorithm Comparison
```bash
# Start with moderate workload
./build/simulator 1 5 12

# Watch default (Greedy) algorithm work...
> status

# After first scheduler cycle, switch:
> algorithm 3    # Try Hill Climbing
> status

# Compare performance!
> quit
```

## Available Commands

| Command | Description |
|---------|-------------|
| `help` or `h` | Show all commands |
| `status` or `s` | Display current simulation state |
| `pause` or `p` | Pause/resume simulation |
| `add <id> <from> <to>` | Add new task (e.g., `add 100 5 12`) |
| `algorithm <1-3>` | Change algorithm (1=BruteForce, 2=Greedy, 3=HillClimbing) |
| `interval <sec>` | Set scheduler interval (e.g., `interval 10`) |
| `quit` or `q` | Exit simulation |

## What the Simulator Shows

When the scheduler runs, you'll see:
- **Task assignments** for each robot
- **Travel times** to pickup locations
- **Execution times** for each task
- **Battery consumption** tracking
- **Makespan** (total time to complete all tasks)
- **Algorithm metrics** (computation time, optimality info)

## Tips for Best Results

1. **Start small**: Try `./build/simulator 1 3 10` first
2. **Watch the scheduler**: It runs every 5 seconds by default
3. **Use status frequently**: Check robot distribution and task progress
4. **Try different algorithms**: Compare Greedy (fast) vs Hill Climbing (better)
5. **Add tasks dynamically**: Test the online behavior!

## Known Limitations

- Robot movement simulation is placeholder (robots don't actually move yet)
- Battery updates during movement not fully implemented
- Task completion detection is basic

These are intentional - the focus is on the **scheduling algorithms** and
**real-time interaction** architecture. Full robot physics can be added
in the `updateRobots()` method as needed.

## Success Indicators

You'll know it's working when you see:
✓ Graph loads successfully
✓ Tasks are loaded (if specified)
✓ Simulation starts and shows time progression
✓ Scheduler runs automatically at intervals
✓ Commands are processed (status, add, algorithm, etc.)
✓ Algorithm execution shows detailed assignments

Enjoy exploring the real-time warehouse simulation! 🤖📦
