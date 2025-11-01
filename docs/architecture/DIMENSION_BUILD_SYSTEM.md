# Dimension Build System

## Overview

GSPHCODE now supports building executables for all three dimensions (1D, 2D, and 3D) using CMake's `BUILD_DIM` option. This document explains the implementation and usage.

## Background

The original codebase had a hardcoded `#define DIM 1` in `include/utilities/defines.hpp`, which meant:
- Only one dimension could be compiled at a time
- Switching dimensions required full recompilation
- Testing multiple dimensions was cumbersome
- CI/CD would need complex build matrices

## Solution

### CMake Configuration

The `BUILD_DIM` CMake option allows specifying the spatial dimension at build time:

```bash
cmake -B build -DBUILD_DIM=2  # Build for 2D
cmake --build build
```

Valid values: `1`, `2`, or `3` (default: `2`)

### Implementation Details

1. **CMakeLists.txt Changes**:
   - Added `BUILD_DIM` as a CMake STRING variable
   - Removed hardcoded `#define DIM 1` from defines.hpp
   - Added compile definition: `add_compile_definitions(DIM=${BUILD_DIM})`
   - Changed to STATIC library to avoid dimension conflicts
   - Used dimension-specific executable names: `sph1d`, `sph2d`, `sph3d`
   - Added `-force_load` (macOS) to ensure sample registry initialization

2. **Header Changes** (`include/utilities/defines.hpp`):
   - Removed: `#define DIM 1`
   - Added: `#ifndef DIM` guard with error message
   - DIM is now defined via compiler flag: `-DDIM=${BUILD_DIM}`

3. **Build Script** (`scripts/build_all_dimensions.sh`):
   - Builds all three dimensions automatically
   - Uses separate temporary build directories to avoid conflicts
   - Copies executables to final build directory
   - Build time: ~23-32 seconds for all three dimensions

## Usage

### Building All Dimensions

```bash
./scripts/build_all_dimensions.sh build
```

This creates:
- `build/sph1d` - 1D simulations
- `build/sph2d` - 2D simulations  
- `build/sph3d` - 3D simulations

### Building Single Dimension

```bash
cmake -B build -DBUILD_DIM=1
cmake --build build
# Creates build/sph1d
```

### Running Simulations

```bash
./build/sph1d shock_tube      # 1D shock tube
./build/sph2d khi              # 2D Kelvin-Helmholtz instability
./build/sph3d evrard           # 3D Evrard collapse
```

### Smart Run Script

Automatically detects dimension from config file:

```bash
./scripts/smart_run.sh sample/khi/config.toml
# Detects 2D from config, runs ./build/sph2d khi
```

## Technical Notes

### Why Static Linking?

Originally attempted SHARED library, but discovered:
- All executables would load the same `libsph_lib.dylib`
- When building sph1d (DIM=1), then sph2d (DIM=2), then sph3d (DIM=3), the library was overwritten each time
- Final result: all executables linked to 3D library
- Solution: Use STATIC library with `-force_load` to embed code directly

### Sample Registry Initialization

C++ global constructors in static libraries don't run unless referenced. Solution:
- **macOS**: `-Wl,-force_load,$<TARGET_FILE:sph_lib>`
- **Linux**: `-Wl,--whole-archive sph_lib -Wl,--no-whole-archive`

This ensures `REGISTER_SAMPLE()` macros execute and populate the sample registry.

### Main.cpp Separation

`src/core/main.cpp` must NOT be in the library:
- Each executable has its own `main()` function
- Library only contains reusable simulation code
- Updated `src/core/CMakeLists.txt` to exclude main.cpp

## Build Directory Structure

```
build_1d_temp/          # Temporary build for 1D (cleaned up)
build_2d_temp/          # Temporary build for 2D (cleaned up)
build_3d_temp/          # Temporary build for 3D (cleaned up)
build/
  ├── sph1d             # Final 1D executable (2.2 MB)
  ├── sph2d             # Final 2D executable (2.2 MB)
  └── sph3d             # Final 3D executable (2.2 MB)
```

## Validation

All three dimensions tested and working:

```
✓ 1D shock_tube:  Output directory: results/SSPH/shock_tube/1D
✓ 2D khi:         Output directory: results/SSPH/khi/2D  
✓ 2D hydrostatic: Output directory: results/SSPH/hydrostatic/2D
✓ 3D evrard:      Output directory: results/SSPH/evrard/3D
```

## Future Enhancements

Potential improvements (Phase 5+):

1. **Runtime Templates** (Complex, 20-30 hours):
   - `VectorND<D>` and `ParticleND<D>` templates
   - Runtime dimension selection via `std::variant`
   - Single executable for all dimensions
   - See `DIMENSION_HANDLING.md` Option 2

2. **CMake Presets**:
   - Add `CMakePresets.json` with 1D/2D/3D configurations
   - Users can select: `cmake --preset=2d`

3. **Package Configuration**:
   - Install rules for all three executables
   - Symlink `sph -> sph2d` as default

## References

- `DIMENSION_HANDLING.md` - Comprehensive analysis of dimension problem
- `CMakeLists.txt` - Root build configuration
- `scripts/build_all_dimensions.sh` - Automated build script
- `scripts/smart_run.sh` - Dimension auto-detection runner
