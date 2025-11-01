#!/bin/bash

# Build script for shock tube simulation plugin

set -e  # Exit on error

cd "$(dirname "$0")"

echo "======================================"
echo "Building Shock Tube Simulation Plugin"
echo "======================================"

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring..."
cmake -DDIM=1 ..

# Build
echo "Building..."
make

echo ""
echo "======================================"
echo "Build complete!"
echo "======================================"
echo ""
echo "Plugin location:"
echo "  $(pwd)/shock_tube_plugin.dylib"
echo ""
echo "To run:"
echo "  cd ../../build"
echo "  ./sph1d ../simulations/shock_tube/build/shock_tube_plugin.dylib"
echo ""
