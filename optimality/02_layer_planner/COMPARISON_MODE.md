# Algorithm Comparison Mode - Quick Reference

## Overview
The planner now includes a **Comparison Mode** (Algorithm ID 0) that automatically runs all three algorithms on the same problem for easy performance comparison.

## Usage

### Basic Comparison
```bash
./build/planner 0 1 12 2
```
This runs all 3 algorithms on:
- Graph 1
- 12 tasks
- 2 robots

### Comparison Syntax
```bash
./build/planner 0 [graphID] [numTasks] [numRobots]
```

## Example Output

```
========================================
  ALGORITHM COMPARISON TEST
  Graph with 17 nodes, 12 Tasks, 2 Robots
========================================

--- BRUTE FORCE (Optimal) ---
Algorithm computation time: 23.17 ms
Optimal assignment found with a makespan (max robot completion time) of: 112.24 seconds.

--- GREEDY (Fast Heuristic) ---
Algorithm computation time: 0.05 ms
Makespan (max robot completion time): 119.41 seconds.

--- HILL CLIMBING (Improved Greedy) ---
Algorithm computation time: 7.76 ms
Total improvements: 23
Final makespan (max robot completion time): 116.93 seconds.

========================================
```

## Comparison Examples

### Small Problem (≤12 tasks)
```bash
./build/planner 0 1 10 2
```
**Recommended**: Use Brute Force for optimal solution (reasonable computation time)

### Medium Problem (15-30 tasks)
```bash
./build/planner 0 1 20 2
```
**Recommended**: Use Hill Climbing (good balance of quality and speed)

### Large Problem (50+ tasks)
```bash
# Note: Brute Force will be very slow!
./build/planner 0 1 50 2
```
**Recommended**: Use Greedy (fast) or Hill Climbing (better quality)

## Interpreting Results

### Metrics Shown
- **Computation time**: How long the algorithm took to run
- **Makespan**: Maximum completion time across all robots (lower is better)
- **Improvements** (Hill Climbing only): Number of successful optimizations

### Performance Expectations

| Problem Size | Brute Force | Greedy | Hill Climbing |
|-------------|-------------|--------|---------------|
| ≤10 tasks   | <10 ms      | <0.1 ms| ~5-10 ms      |
| ~12 tasks   | ~25 ms      | <0.1 ms| ~10 ms        |
| ~20 tasks   | ~500 ms     | <0.2 ms| ~50-100 ms    |
| ~30 tasks   | Minutes!    | <0.5 ms| ~200-500 ms   |
| 50+ tasks   | Hours!      | ~1 ms  | ~1-5 seconds  |

### Quality Expectations

| Algorithm | Quality | Typical Optimality |
|-----------|---------|-------------------|
| Brute Force | Optimal | 100% |
| Hill Climbing | Near-optimal | 94-98% |
| Greedy | Heuristic | 85-95% |

## Benefits

✅ **Same Test Conditions**: All algorithms use identical initial states  
✅ **Fair Comparison**: Same graph, tasks, and robots for all  
✅ **Quick Analysis**: See trade-offs between speed and quality  
✅ **Algorithm Validation**: Verify implementations are working correctly  
✅ **Research**: Collect data for algorithm performance studies  

## Use Cases

### Development & Testing
```bash
# Quick sanity check after code changes
./build/planner 0 1 8 2
```

### Algorithm Selection
```bash
# Decide which algorithm to use for your problem size
./build/planner 0 2 15 3  # Test on Graph 2
```

### Performance Benchmarking
```bash
# Generate comparison data for different graphs
for graph in {1..10}; do
    echo "Graph $graph:"
    ./build/planner 0 $graph 12 2 2>&1 | grep "makespan"
done
```

### Research & Documentation
```bash
# Compare algorithms across different problem sizes
for tasks in 8 10 12 15 20; do
    echo "=== $tasks tasks ==="
    ./build/planner 0 1 $tasks 2
done
```

## Implementation Details

### How It Works
1. Saves original task list
2. For each algorithm (1, 2, 3):
   - Recreates identical robot states (position 0,0, 100% battery)
   - Recreates task queue from saved list
   - Runs the algorithm
   - Displays results
3. Shows summary separator

### Initial Conditions
All algorithms start with:
- Robots at position (0.0, 0.0)
- 100% battery level
- No current tasks
- Identical task queue

### Full vs Compact Output
The comparison mode shows:
- ✅ Algorithm headers
- ✅ Computation time
- ✅ Makespan (completion time)
- ✅ Number of improvements (Hill Climbing)
- ✅ Detailed assignment report for each algorithm

## Tips

### For Quick Comparisons
Extract just the summary lines:
```bash
./build/planner 0 1 12 2 2>&1 | grep -E "(computation time|makespan|improvements)"
```

### For Large Datasets
Skip Brute Force (not implemented yet, but you could modify):
```bash
# Currently runs all 3, but you can use individual IDs for large problems
./build/planner 2 1 100 4  # Just Greedy
./build/planner 3 1 100 4  # Just Hill Climbing
```

### For Reproducibility
Specify all parameters explicitly:
```bash
./build/planner 0 1 12 2  # Graph 1, 12 tasks, 2 robots
```

## Related Files

- `main.cc` - Handles algorithmID 0 parameter
- `Planifier.cc` - Implements comparison mode logic
- `ALGORITHMS_SUMMARY.md` - Detailed algorithm documentation
- `test_comparison.sh` - Example test script

---

**Created**: December 2024  
**Feature**: Comparison Mode (Algorithm ID 0)  
**Version**: 1.0
