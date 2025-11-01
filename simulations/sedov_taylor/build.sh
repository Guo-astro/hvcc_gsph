#!/bin/bash

# Build script for Sedov-Taylor simulation plugin

set -e  # Exit on error

cd "$(dirname "$0")"

echo "=========================================="
echo "Building Sedov-Taylor Simulation Plugin"
echo "=========================================="

# Create build directory
mkdir -p build
cd build

# Configure with CMake (DIM=3 required)
echo "Configuring..."
cmake -DDIM=3 ..

# Build
echo "Building..."
make

echo ""
echo "=========================================="
echo "Build complete!"
echo "=========================================="
echo ""
echo "Plugin location:"
echo "  $(pwd)/sedov_taylor_plugin.dylib"
echo ""
echo "To run:"
echo "  cd ../../build"
echo "  ./sph3d ../simulations/sedov_taylor/build/sedov_taylor_plugin.dylib"
echo "  # or"
echo "  ./run_simulation sedov_taylor"
echo ""
