# Packet Generation System - Quick Start Guide

## What's New?

A complete packet generation system for creating random test cases based on graph distributions.

## File Organization

```
02_layer_planner/
├── packet_generators/              # NEW: Generator tools
│   ├── generate_packets_graph1     # Compiled generator binary
│   ├── generate_packets_graph1.cc  # Generator source code
│   ├── batch_generate.sh           # Batch generation script
│   ├── Makefile                    # Build system
│   └── README.md                   # Detailed documentation
├── generated_packets/              # NEW: Auto-generated test cases
│   └── graph1/
│       ├── graph1_case1.inp
│       ├── graph1_case2.inp
│       └── ... (up to case10.inp)
├── packets/                        # Original manual test cases
│   └── packet1.inp
└── PACKET_FORMAT.md                # NEW: Format documentation
```

## Quick Start

### 1. Build the Generator
```bash
cd packet_generators
make
```

### 2. Generate Test Cases

**Simple generation:**
```bash
./generate_packets_graph1 10 15
# Generates 10 cases with 15 packets each
```

**With specific seed (reproducible):**
```bash
./generate_packets_graph1 10 15 42
# Always generates the same random cases
```

**Batch generation:**
```bash
./batch_generate.sh
# Generates 30 test cases with varying sizes
```

### 3. View Generated Files
```bash
cd ../generated_packets/graph1
ls -lh
cat graph1_case1.inp
```

## Example Output

### Command:
```bash
./generate_packets_graph1 3 5
```

### Terminal Output:
```
=== Packet Generator for Graph1 ===
Generating 3 test cases
Packets per case: 5
Random seed: 1760131988
Output directory: ../generated_packets/graph1/

Generated: ../generated_packets/graph1/graph1_case1.inp
Generated: ../generated_packets/graph1/graph1_case2.inp
Generated: ../generated_packets/graph1/graph1_case3.inp

Successfully generated 3 test cases!

Pickup nodes used: 2 3 4 5 
Dropoff nodes used: 11 12 13
```

### Generated File (graph1_case1.inp):
```
5
0 5 11
1 2 12
2 4 13
3 5 12
4 4 11
```

## Graph 1 Node Information

- **Total Nodes:** 17
- **Pickup Nodes (P):** 2, 3, 4, 5
- **Dropoff Nodes (D):** 11, 12, 13

All generated packets randomly select from these valid pickup/dropoff nodes.

## Common Use Cases

### Testing with Small Cases
```bash
./generate_packets_graph1 10 5
# Good for quick algorithm testing
```

### Testing with Realistic Cases
```bash
./generate_packets_graph1 10 15
# Similar to packet1.inp size
```

### Testing with Large Cases
```bash
./generate_packets_graph1 10 50
# Stress testing
```

### Reproducible Benchmarks
```bash
./generate_packets_graph1 5 20 12345
# Use same seed for consistent benchmarking
```

## Next Steps

1. ✅ Generator for Graph 1 is complete
2. ⏳ Generators for Graph 2-10 (to be implemented)
3. ⏳ Integration with planner to read packet files
4. ⏳ Batch testing script for all graphs

## Need Help?

- **Generator documentation:** `packet_generators/README.md`
- **Packet format specification:** `PACKET_FORMAT.md`
- **Planner documentation:** `README.md`
- **Usage help:** `./generate_packets_graph1` (no arguments)

## Tips

- Use consistent seeds for reproducible experiments
- Generate multiple cases to test algorithm robustness
- Vary packet counts to test scalability
- Check generated files are valid before running planner
