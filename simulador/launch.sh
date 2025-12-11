#!/bin/bash
# Launch script for the warehouse simulator
# This script forces the use of X11 (XCB) backend to avoid Wayland warnings

export QT_QPA_PLATFORM=xcb
./examen
