#!/bin/bash
# Test script to demonstrate algorithm comparison mode

cd "$(dirname "$0")"

echo "=========================================="
echo "  Testing Algorithm Comparison Mode"
echo "=========================================="
echo ""
echo "Running: ./build/planner 0 1 12 2"
echo "(Compare all algorithms on Graph 1, 12 tasks, 2 robots)"
echo ""

./build/planner 0 1 12 2 2>&1 | grep -A1 -E "^---|computation time|makespan|improvements"

echo ""
echo "=========================================="
echo "  Test Complete"
echo "=========================================="
