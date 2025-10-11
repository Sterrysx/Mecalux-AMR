# Task Generators for Layer Planner

This directory contains utilities for generating random test cases for the layer planner.

## Building

```bash
make        # Build all generators
make clean  # Remove build artifacts and generated test files
make help   # Show detailed help
```

## Generators

### generate_tasks_graph1
Generates test cases for graph1 (14-node graph).
- **Pickup nodes**: 2, 3, 4, 5
- **Dropoff nodes**: 11, 12, 13

### generate_tasks_graph2
Generates test cases for graph2 (11-node graph).
- **Pickup nodes**: 2, 3, 4, 5
- **Dropoff nodes**: 6, 7, 8, 9

## Usage

### Fixed Mode
Generate test cases with a fixed number of tasks per case:

```bash
./build/generate_tasks_graph1 <num_cases> <tasks_per_case> [seed]
```

**Example:**
```bash
./build/generate_tasks_graph1 10 15        # 10 cases with 15 tasks each
./build/generate_tasks_graph1 5 20 42      # 5 cases with 20 tasks, seed=42
```

### Incremental Mode
Generate test cases with incrementing number of tasks:

```bash
./build/generate_tasks_graph1 -i <num_cases> <start> <increment> [seed]
```

**Example:**
```bash
./build/generate_tasks_graph1 -i 10 10 10    # 10 cases: 10, 20, 30, ..., 100 tasks
./build/generate_tasks_graph2 -i 5 5 5 999   # 5 cases: 5, 10, 15, 20, 25 tasks, seed=999
```

## Output Paths

The generators support two modes for determining output paths:

1. **With MECALUX_ROOT environment variable** (recommended for scripts):
   - Files are written to absolute path: `$MECALUX_ROOT/optimality/02_layer_planner/tests/graphN/`
   - This works regardless of where you run the generator from

2. **Without MECALUX_ROOT** (fallback):
   - Files are written to relative path: `../../tests/graphN/`
   - Only works correctly when run from the `utils/` directory

### Recommended Usage from Workspace Root

```bash
# From /path/to/Mecalux-AMR
export MECALUX_ROOT=$(pwd)
./optimality/02_layer_planner/utils/build/generate_tasks_graph1 -i 10 10 10
./optimality/02_layer_planner/utils/build/generate_tasks_graph2 -i 10 10 10
```

### Usage from utils/ Directory

```bash
# From /path/to/Mecalux-AMR/optimality/02_layer_planner/utils
./build/generate_tasks_graph1 10 15
./build/generate_tasks_graph2 10 15
```

## Output Format

Generated files follow the format:
```
<num_tasks>
<task_id> <pickup_node> <dropoff_node>
<task_id> <pickup_node> <dropoff_node>
...
```

**Example (graph1_case1.inp):**
```
10
0 2 11
1 3 12
2 4 13
...
```

## Integration with Planner

The generated test files are used by the main planner:

```bash
cd /path/to/Mecalux-AMR/optimality/02_layer_planner
./planner
# Reads from: tests/graph1/graph1_case1.inp
```
