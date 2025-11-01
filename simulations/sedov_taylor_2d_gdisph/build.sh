#!/bin/bash

# Build script for Sedov-Taylor 2D GDISPH simulation plugin

set -e  # Exit on error

cd "$(dirname "$0")"

echo "==================================================="
echo "Building Sedov-Taylor 2D GDISPH Simulation Plugin"
echo "==================================================="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring..."
cmake ..

# Build
echo "Building..."
make

echo ""
echo "==================================================="
echo "Build complete!"
echo "==================================================="
echo ""
echo "Plugin location:"
echo "  $(pwd)/sedov_taylor_2d_gdisph_plugin.dylib"
echo ""
echo "To run:"
echo "  cd ../../build"
echo "  ./sph2d ../simulations/sedov_taylor_2d_gdisph/sedov_taylor_2d_gdisph_plugin.dylib"
echo ""
echo "Or with config file:"
echo "  ./sph2d ../simulations/sedov_taylor_2d_gdisph/config.json"
echo ""
