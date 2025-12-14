#!/bin/bash

echo "======================================================================"
echo "  Starting Mecalux Simulator"
echo "======================================================================"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR/simulador"

# Check if built
if [ ! -f "examen" ]; then
    echo "ERROR: Simulator not built. Building now..."
    make
    if [ $? -ne 0 ]; then
        echo "Build failed!"
        exit 1
    fi
fi

echo "Starting simulator..."
echo ""
echo "Instructions:"
echo "  1. Click 'Connect to Backend' button"
echo "  2. Watch robots move in real-time!"
echo ""
echo "Camera controls:"
echo "  - Left mouse + drag: Rotate"
echo "  - Mouse wheel: Zoom"
echo ""

./examen
