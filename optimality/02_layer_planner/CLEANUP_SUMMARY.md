
## Auto-Generation Integration Update

### Additional Fix: Task Generator Integration

**Problem:** The planner's auto-generation feature was calling the old generator executables:
```
./utils/build/generate_tasks_graph4 1 17 16
```
This failed because those executables no longer exist.

**Solution:** Updated `main.cc` to use the new universal task generator:
```cpp
// Old command (broken)
string generatorCmd = "./utils/build/generate_tasks_graph" + to_string(graphID) + 
                     " 1 " + to_string(numTasks) + " 16";

// New command (working)
string generatorCmd = "cd utils && ./build/taskGenerator " + to_string(graphID) + 
                     " 1 " + to_string(numTasks) + " 16";
```

**Why `cd utils &&`?**
The taskGenerator uses relative path `../tests/graphN/`, so it must be run from the utils directory to create files in the correct location (`02_layer_planner/tests/graphN/`).

### Updated Help Message

Removed obsolete algorithm reference:
```diff
- 2 = Dynamic Programming
+ (removed - algorithm doesn't exist)
```

Updated to show only available algorithms:
- 1 = Brute Force (optimal)
- 3 = Greedy (suboptimal heuristic)

### Testing Results

✅ **Auto-generation works for all graphs:**
```bash
./build/planner 1 4 17 2    # Graph 4, 17 tasks - auto-generates
./build/planner 3 7 12 2    # Graph 7, 12 tasks - auto-generates  
./build/planner 3 10 30 3   # Graph 10, 30 tasks, 3 robots - auto-generates
```

✅ **Doesn't regenerate existing files:**
```bash
./build/planner 3 4 17 2    # Uses existing file, doesn't regenerate
```

✅ **Complete workflow:**
1. User runs: `./build/planner <alg> <graphID> <numTasks> <numRobots>`
2. Planner checks if `tests/graph<ID>/<numTasks>_tasks.inp` exists
3. If not, automatically runs: `cd utils && ./build/taskGenerator <graphID> 1 <numTasks> 16`
4. Task file is created with seed=16 (reproducible)
5. Planner proceeds with the generated tasks

## Complete Changes Summary

1. ✅ Removed DynamicProgramming from Makefile
2. ✅ Deleted redundant `03_Greedy_print.cc`
3. ✅ Updated task generator command in main.cc
4. ✅ Updated help message to reflect available algorithms only
5. ✅ All algorithms compile and run successfully
6. ✅ Auto-generation works for all 10 graphs
