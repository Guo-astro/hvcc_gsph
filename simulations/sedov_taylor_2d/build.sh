#!/bin/bash
# Build script for sedov_taylor_2d plugin

set -e  # Exit on error

# Determine dimension (default: 2)
DIM=${1:-2}

# Validate dimension
if [ "$DIM" != "2" ]; then
    echo "Error: This simulation requires DIM=2"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

# Configure and build
echo "Building sedov_taylor_2d plugin for ${DIM}D..."
cmake -DBUILD_DIM=$DIM ..
cmake --build .

# Check if build succeeded
if [ -f "libsedov_taylor_2d_plugin.dylib" ] || [ -f "libsedov_taylor_2d_plugin.so" ]; then
    echo "✓ Build successful!"
    
    # Show plugin info
    if [ -f "libsedov_taylor_2d_plugin.dylib" ]; then
        ls -lh libsedov_taylor_2d_plugin.dylib
    else
        ls -lh libsedov_taylor_2d_plugin.so
    fi
else
    echo "✗ Build failed!"
    exit 1
fi
