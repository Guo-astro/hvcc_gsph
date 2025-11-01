# GSPH Project Overview

## Purpose
GSPHCODE is a Smoothed Particle Hydrodynamics (SPH) simulation framework for compressible fluid dynamics. It's designed for astrophysical and computational fluid dynamics research, supporting multiple SPH variants and physical phenomena.

## Tech Stack
- **Language**: C++14
- **Build System**: CMake (>= 3.23)
- **Dependencies**: 
  - Boost (filesystem, iostreams)
  - OpenMP (parallelization)
- **Platform**: macOS (arm64), Linux support via Makefile
- **Compiler**: LLVM/Clang (macOS), GCC 7.4.0+ (Linux)

## Key Features
1. **Multiple SPH Algorithms**:
   - SSPH (Standard SPH) - density-energy formulation
   - DISPH (Density Independent SPH) - pressure-energy formulation
   - GSPH (Godunov SPH) - Riemann solver-based

2. **Physical Modules**:
   - Fluid forces with artificial viscosity
   - Self-gravity (tree-based)
   - Heating/cooling
   - Shock detection
   - Periodic boundaries

3. **Kernel Functions**:
   - Cubic spline
   - Wendland C4

4. **Simulation Types**:
   - Sample simulations (test cases, benchmarks)
   - Production simulations (research-grade runs)

## Project Structure
```
sphcode/
├── include/           # Header files
│   ├── simulation.hpp # Core simulation state
│   ├── solver.hpp     # Main solver orchestration
│   ├── module.hpp     # Module interface
│   ├── module_factory.hpp # Module registration system
│   ├── sample_registry.hpp # Sample registration system
│   ├── particle.hpp   # Particle data structure
│   ├── parameters.hpp # Configuration parameters
│   ├── kernel/        # Kernel functions
│   ├── gsph/          # Godunov SPH specific
│   ├── disph/         # DISPH specific
│   └── utilities/     # Helper utilities
├── src/               # Implementation files
│   ├── main.cpp       # Entry point
│   ├── solver.cpp     # Solver implementation
│   ├── simulation.cpp # Simulation state management
│   ├── sample/        # Sample simulation implementations
│   ├── production_sims/ # Production simulations
│   ├── gsph/          # GSPH implementations
│   ├── disph/         # DISPH implementations
│   └── relaxation/    # Relaxation algorithms
├── sample/            # Sample configuration files (JSON)
├── production_sims/   # Production simulation configs
├── build/             # CMake build directory
├── build_manual/      # Makefile build directory
└── test/              # Unit tests
```

## Architecture Overview

### Core Components
1. **Simulation**: Manages particle state, time, kernel, tree, periodic boundaries
2. **Solver**: Orchestrates the simulation loop, integrates modules
3. **Module**: Interface for pluggable physics calculations
4. **SampleRegistry**: Factory pattern for registering/creating simulations
5. **ModuleFactory**: Factory pattern for creating SPH-variant-specific modules

### Registration Pattern
Both samples and modules use compile-time registration via macros:
- `REGISTER_SAMPLE(name, function)` - Registers simulation setups
- `REGISTER_MODULE(sph_type, module_type, class)` - Registers algorithm implementations
