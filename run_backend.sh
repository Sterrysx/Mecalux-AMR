#!/bin/bash

echo "======================================================================"
echo "  Starting Mecalux Backend Fleet Manager"
echo "======================================================================"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Clean old telemetry files from orca folder
echo "Cleaning old telemetry files..."
rm -f api/orca/*.json
echo "  ✓ Removed all .json files from api/orca/"
echo ""

cd backend

# Check if built
if [ ! -f "build/fleet_manager" ]; then
    echo "ERROR: Backend not built. Building now..."
    make
    if [ $? -ne 0 ]; then
        echo "Build failed!"
        exit 1
    fi
fi

echo "Running fleet manager..."
echo "Press Ctrl+C to stop"
echo ""

./build/fleet_manager --from-json
