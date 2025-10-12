# Algorithm Comparison Summary

## Overview
This document summarizes the three task assignment algorithms implemented in the AMR planner system:

1. **Brute Force** (Algorithm 1) - Optimal solution
2. **Greedy** (Algorithm 2) - Fast heuristic  
3. **Hill Climbing** (Algorithm 3) - Improved greedy via local search

---

## Algorithm Descriptions

### 1. Brute Force (01_BruteForce.hh/cc)
- **Strategy**: Exhaustive search of all possible task assignments
- **Optimality**: Always finds the optimal solution (minimum makespan)
- **Complexity**: Exponential - O(k^n) where k=robots, n=tasks
- **Best for**: Small problem instances (≤12 tasks)
- **Battery Management**: Full preventive charging logic

### 2. Greedy (02_Greedy.hh/cc)
- **Strategy**: Assign each task to the robot that can complete it soonest
- **Optimality**: Suboptimal heuristic (no optimality guarantee)
- **Complexity**: Polynomial - O(n*k) where k=robots, n=tasks
- **Best for**: Large problems requiring fast solutions
- **Battery Management**: Full preventive charging logic

### 3. Hill Climbing (03_HillClimbing.hh/cc)
- **Strategy**: 
  1. Start with greedy solution
  2. Try swapping tasks between robots
  3. Accept swaps that improve makespan
  4. Stop when no improvement for 20 iterations
- **Optimality**: Better than greedy, not guaranteed optimal
- **Complexity**: Polynomial with iterations - O(iterations * n^2 * k^2)
- **Best for**: Medium to large problems needing better quality than greedy
- **Battery Management**: Full preventive charging logic

---

## Performance Comparison

### Test Case: Graph 1, 12 Tasks, 2 Robots

| Algorithm     | Makespan (s) | Computation Time | Quality vs Optimal | Speed Ranking |
|---------------|--------------|------------------|-------------------|---------------|
| Brute Force   | 112.24       | 21.36 ms         | 100.0% (optimal)  | Slowest       |
| Greedy        | 119.41       | 0.14 ms          | 94.0%             | Fastest       |
| Hill Climbing | 116.93       | 10.58 ms         | 96.0%             | Medium        |

**Key Insights**:
- Hill Climbing achieved **96% optimality** (vs Greedy's 94%)
- Hill Climbing is **76x slower** than Greedy but **2x faster** than Brute Force
- Greedy is **152x faster** than Brute Force but **6.4% worse** in quality

### Test Case: Graph 1, 20 Tasks, 2 Robots

| Algorithm     | Makespan (s) | Computation Time | Improvements Made |
|---------------|--------------|------------------|-------------------|
| Hill Climbing | 245.61       | 69.60 ms         | 72 improvements   |

**Key Insights**:
- Starting makespan: 481.80s (greedy solution)
- Final makespan: 245.61s (after hill climbing)
- **49% improvement** over greedy through local search
- 72 successful swaps found during optimization

---

## Battery Management

All three algorithms implement the same battery constraints:
- **Battery Life**: 300 seconds at full charge
- **Recharge Rate**: 2% per second
- **Low Battery Threshold**: 20%
- **Empty Load Multiplier (α)**: 2.0 (double consumption when empty)
- **Preventive Charging**: Robots charge proactively if next task would drop below 20%

---

## Usage Examples

### Standard Mode (Single Algorithm)

```bash
# Brute Force (optimal but slow)
./build/planner 1 1 10 2

# Greedy (fast heuristic)
./build/planner 2 1 50 3

# Hill Climbing (balanced quality/speed)
./build/planner 3 1 30 2
```

### Comparison Mode (All Algorithms)

Run all three algorithms on the same problem for easy comparison:

```bash
# Compare algorithms on Graph 1, 12 tasks, 2 robots
./build/planner 0 1 12 2
```

**Example Output:**
```
========================================
  ALGORITHM COMPARISON TEST
  Graph with 17 nodes, 12 Tasks, 2 Robots
========================================

--- BRUTE FORCE (Optimal) ---
Algorithm computation time: 29.81 ms
Optimal assignment found with a makespan (max robot completion time) of: 112.24 seconds.

--- GREEDY (Fast Heuristic) ---
Algorithm computation time: 0.10 ms
Makespan (max robot completion time): 119.41 seconds.

--- HILL CLIMBING (Improved Greedy) ---
Algorithm computation time: 6.91 ms
Total improvements: 23
Final makespan (max robot completion time): 116.93 seconds.
========================================
```

**Benefits of Comparison Mode (ID 0)**:
- ✅ Same initial conditions for all algorithms
- ✅ Side-by-side performance metrics
- ✅ Easy quality vs speed trade-off analysis
- ✅ Automated testing for algorithm validation

---

## Algorithm Selection Guide

### Choose **Brute Force** when:
- ✅ Problem size is small (≤12 tasks)
- ✅ Optimal solution is required
- ✅ Computation time is not critical
- ❌ NOT for: Large problems (exponential explosion)

### Choose **Greedy** when:
- ✅ Fast solution needed
- ✅ Large problem sizes (50+ tasks)
- ✅ Approximate solution acceptable
- ❌ NOT for: When quality is critical

### Choose **Hill Climbing** when:
- ✅ Medium to large problems (15-50 tasks)
- ✅ Better quality than greedy desired
- ✅ Can afford more computation than greedy
- ✅ Optimal solution not strictly required
- ❌ NOT for: Real-time systems (unpredictable iterations)

---

## Implementation Details

### File Structure
```
include/algorithms/
  ├── 01_BruteForce.hh
  ├── 02_Greedy.hh
  └── 03_HillClimbing.hh

src/algorithms/
  ├── 01_BruteForce.cc      (380 lines)
  ├── 02_Greedy.cc          (296 lines)
  └── 03_HillClimbing.cc    (446 lines)
```

### Common Helper Methods
All algorithms share similar helper structures:
- `calculateDistance()` - Euclidean distance
- `getChargingNodeId()` - Find charging station
- `calculateTaskBatteryConsumption()` - Battery usage prediction
- `shouldCharge()` - Preventive charging decision
- `performCharging()` - Charging simulation
- `printBeautifiedAssignment()` - Formatted output

### Hill Climbing Specifics
- **Max Iterations**: 100
- **Stopping Criterion**: 20 iterations without improvement
- **Neighborhood Search**: Task swaps + task moves
- **Evaluation**: Full makespan recalculation per candidate

---

## Build System

### Makefile Updates
All three algorithms properly integrated:
```makefile
SOURCES = ... \
  $(SRCDIR)/algorithms/01_BruteForce.cc \
  $(SRCDIR)/algorithms/02_Greedy.cc \
  $(SRCDIR)/algorithms/03_HillClimbing.cc \
  ...
```

### Compilation
```bash
# Clean and rebuild
make clean
make

# Test all algorithms
./build/planner 1 1 12 2  # Brute Force
./build/planner 2 1 12 2  # Greedy
./build/planner 3 1 12 2  # Hill Climbing
```

---

## Future Improvements

### Potential Enhancements
1. **Simulated Annealing**: Accept worse solutions probabilistically
2. **Genetic Algorithm**: Population-based metaheuristic
3. **Tabu Search**: Memory-based local search
4. **Parallel Evaluation**: Multi-threaded candidate evaluation
5. **Adaptive Parameters**: Dynamic iteration limits based on problem size

### Performance Tuning
- Memoization of distance calculations
- Incremental makespan updates (avoid full recalc)
- Better initial solutions (construction heuristics)
- Multi-start hill climbing (random restarts)

---

## References

### Related Files
- `main.cc` - Entry point with algorithm selection
- `Planifier.cc` - Algorithm orchestration
- `Robot.hh/cc` - Robot state management
- `Task.hh/cc` - Task definitions
- `Graph.hh/cc` - Warehouse layout (from 01_layer_mapping)

### Test Files
```
tests/graph1/   # 12, 13, 14, 15, 20, 30 tasks
tests/graph2/   # Various task counts
...
tests/graph10/  # Various task counts
```

### Tools
- `utils/taskGenerator.cc` - Universal task file generator
- `utils/build/taskGenerator` - Compiled generator

---

**Last Updated**: December 2024  
**Authors**: AMR Planner Development Team  
**Version**: 3.0 (Three algorithm implementation)
