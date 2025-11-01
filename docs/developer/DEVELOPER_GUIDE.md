# GSPH Developer Guide

A comprehensive guide to understanding, building, and extending the GSPHCODE smoothed particle hydrodynamics simulation framework.

## Table of Contents
1. [Quick Start](#quick-start)
2. [Architecture Overview](#architecture-overview)
3. [Adding New Simulations](#adding-new-simulations)
4. [Adding New Algorithms](#adding-new-algorithms)
5. [Configuration Guide](#configuration-guide)
6. [Build System](#build-system)
7. [Debugging and Testing](#debugging-and-testing)
8. [Best Practices](#best-practices)

---

## Quick Start

### Prerequisites (macOS)
```bash
# Install dependencies via Homebrew
brew install cmake boost llvm libomp

# Configure environment (add to ~/.zshrc)
export OpenMP_ROOT=$(brew --prefix)/opt/libomp
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```

### Build and Run
```bash
# Clone and build
cd /path/to/sphcode
rm -rf build && mkdir build
cmake -B build \
  -DOpenMP_C_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
  -DOpenMP_CXX_FLAGS="-Xpreprocessor -fopenmp /opt/homebrew/opt/libomp/lib/libomp.dylib -I/opt/homebrew/opt/libomp/include" \
  -DOpenMP_C_LIB_NAMES=libomp \
  -DOpenMP_CXX_LIB_NAMES=libomp \
  -DOpenMP_libomp_LIBRARY="/opt/homebrew/opt/libomp/lib/libomp.dylib"
cd build && make

# Run a sample simulation
./sph shock_tube ../sample/shock_tube/shock_tube.json 8
```

### Using Nix (Alternative)
```bash
nix develop          # Enter dev environment
mkdir -p build && cd build
cmake .. && make
./sph shock_tube ../sample/shock_tube/shock_tube.json 8
```

---

## Architecture Overview

### Core Components

```
┌─────────────────────────────────────────────────────┐
│                    main.cpp                         │
│              (Entry Point + CLI Parser)             │
└────────────────────┬────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────┐
│                    Solver                           │
│  • Orchestrates simulation lifecycle                │
│  • Manages time stepping and integration            │
│  • Coordinates modules                              │
└─────┬───────────────────────┬───────────────────────┘
      │                       │
      ▼                       ▼
┌──────────────┐    ┌───────────────────────┐
│  Simulation  │    │  Module System        │
│  • Particles │    │  • PreInteraction     │
│  • Time      │    │  • FluidForce         │
│  • Tree      │    │  • GravityForce       │
│  • Kernel    │    │  • Timestep           │
└──────────────┘    │  • HeatingCooling     │
                    └───────────────────────┘
                              │
                              ▼
                    ┌───────────────────┐
                    │  ModuleFactory    │
                    │  (SPH-type based  │
                    │   instantiation)  │
                    └───────────────────┘
```

### Execution Flow

1. **Initialization** (`Solver::Solver`)
   - Parse command line: `<sample_name> [config.json] [threads]`
   - Load parameters from JSON
   - Setup OpenMP threading
   - Open log files

2. **Sample Loading** (`Solver::initialize`)
   - Lookup sample by name in `SampleRegistry`
   - Call sample initialization function
   - Populate particle array with initial conditions

3. **Module Setup**
   - Query SPH type from parameters (ssph/disph/gsph)
   - Use `ModuleFactory` to create variant-specific modules
   - Call `initialize()` on each module

4. **Integration Loop** (`Solver::run`)
   ```
   while t < t_end:
       integrate():
           predict():  # Kick-Drift-Kick scheme
               - Update velocities (half-step)
               - Update positions (full-step)
           correct():
               - Rebuild tree structure
               - Find neighbors
               - Calculate densities (PreInteraction)
               - Calculate forces (FluidForce, GravityForce)
               - Calculate dt (Timestep module)
               - Update velocities (half-step)
               - Update energies
       output():
           - Write particle snapshots
           - Write energy history
   ```

### Registration System

Both samples and modules use **automatic registration** via static initialization:

#### Sample Registration
```cpp
// In src/sample/my_simulation.cpp
namespace {
    void load_my_simulation(std::shared_ptr<sph::Simulation> sim,
                           std::shared_ptr<sph::SPHParameters> param) {
        // Setup particles, set parameters
        std::vector<SPHParticle> particles;
        // ... initialize particles ...
        sim->set_particles(particles);
        sim->set_particle_num(particles.size());
    }
    
    REGISTER_SAMPLE("my_simulation", load_my_simulation);
    //             ^^^^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^^
    //             CLI name          Initialization function
}
```

**How it works:**
- Macro creates a static boolean variable
- Static initialization happens before `main()`
- Constructor calls `SampleRegistry::register_sample(name, function)`
- Maps string name → function pointer
- No central list to maintain!

#### Module Registration
```cpp
// In src/gsph/g_fluid_force.cpp
class GFluidForce : public FluidForce {
    void initialize(...) override { /* ... */ }
    void calculation(...) override { /* ... */ }
};

REGISTER_MODULE("gsph", "fluid_force", GFluidForce);
//              ^^^^^^  ^^^^^^^^^^^^^  ^^^^^^^^^^^
//              SPH type Module type   Implementation class
```

**Module selection:**
At runtime, `Solver` calls:
```cpp
m_fforce = ModuleFactory::create_module<FluidForce>(
    param->sph.type,  // "gsph"
    "fluid_force"
);
```
This instantiates `GFluidForce` for GSPH, `DFluidForce` for DISPH, etc.

---

## Adding New Simulations

### Step 1: Choose Location

**Sample simulations** (test cases, benchmarks):
- Location: `src/sample/`
- Config: `sample/<name>/`

**Production simulations** (research runs):
- Location: `src/production_sims/`
- Config: `production_sims/<name>/`

### Step 2: Create Source File

```bash
touch src/sample/my_rayleigh_taylor.cpp
```

### Step 3: Implement Initialization

```cpp
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "sample_registry.hpp"
#include <vector>

namespace {
    using namespace sph;

    void load_my_rayleigh_taylor(std::shared_ptr<Simulation> sim,
                                 std::shared_ptr<SPHParameters> param) {
        #if DIM != 2
            THROW_ERROR("Rayleigh-Taylor requires DIM == 2");
        #endif

        // Domain setup
        const int N = 100;  // particles per dimension
        const real domain_size = 1.0;
        const real dx = domain_size / N;
        
        // Physics parameters
        const real gamma = param->physics.gamma;
        const real rho_heavy = 2.0;
        const real rho_light = 1.0;
        const real pressure = 2.5;
        const real g = -0.1;  // gravity
        
        std::vector<SPHParticle> particles;
        
        // Create particles on a grid
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                SPHParticle p;
                
                p.pos[0] = i * dx + dx/2;
                p.pos[1] = j * dx + dx/2;
                p.vel[0] = 0.0;
                p.vel[1] = 0.0;
                
                // Heavy fluid on top, light fluid on bottom
                if (p.pos[1] > 0.5) {
                    p.dens = rho_heavy;
                    p.mass = rho_heavy * dx * dx;
                } else {
                    p.dens = rho_light;
                    p.mass = rho_light * dx * dx;
                }
                
                // Add small perturbation at interface
                if (std::abs(p.pos[1] - 0.5) < 2*dx) {
                    p.vel[1] = 0.01 * std::sin(2 * M_PI * p.pos[0]);
                }
                
                p.pres = pressure;
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                p.id = particles.size();
                
                particles.push_back(p);
            }
        }
        
        sim->set_particles(particles);
        sim->set_particle_num(particles.size());
    }
    
    REGISTER_SAMPLE("rayleigh_taylor", load_my_rayleigh_taylor);

} // namespace
```

### Step 4: Create Configuration File

```bash
mkdir -p sample/my_rayleigh_taylor
touch sample/my_rayleigh_taylor/rayleigh_taylor.json
```

```json
{
  "outputDirectory": "results/rayleigh_taylor",
  "startTime": 0.0,
  "endTime": 3.0,
  "outputTime": 0.05,
  "energyTime": 0.01,
  
  "SPHType": "gsph",
  "cflSound": 0.3,
  "cflForce": 0.125,
  
  "gamma": 1.4,
  "kernel": "wendland",
  "neighborNumber": 50,
  
  "useGravity": true,
  "G": 1.0,
  
  "avAlpha": 1.0,
  "useBalsaraSwitch": true,
  "useTimeDependentAV": false,
  
  "useArtificialConductivity": true,
  "alphaAC": 0.5,
  
  "use2ndOrderGSPH": true
}
```

### Step 5: Update Makefile (Optional)

```makefile
# Add to Makefile
run_rayleigh_taylor: build
	@echo "Running rayleigh_taylor sample..."
	./build_manual/sph rayleigh_taylor ./sample/my_rayleigh_taylor/rayleigh_taylor.json 8
```

### Step 6: Build and Run

```bash
cd build && make
./sph rayleigh_taylor ../sample/my_rayleigh_taylor/rayleigh_taylor.json 8
```

**That's it!** No central registry to update - the `REGISTER_SAMPLE` macro handles everything automatically.

---

## Adding New Algorithms

### When to Create a New Module

Create a module when you need to:
- Implement a new SPH variant (beyond SSPH/DISPH/GSPH)
- Add a new physics calculation (e.g., magnetic fields, radiation)
- Replace an existing module with an alternative algorithm

### Module Types

| Module Type | Purpose | Base Class |
|-------------|---------|------------|
| `pre_interaction` | Neighbor finding, density calculation | `PreInteraction` |
| `fluid_force` | Pressure forces, artificial viscosity | `FluidForce` |
| `gravity_force` | Self-gravity computation | `GravityForce` |
| `timestep` | Adaptive time step calculation | `Timestep` |
| `heating_cooling` | Energy sources/sinks | `HeatingCooling` |

### Example: Adding a New Fluid Force Module

#### Step 1: Create Header

```bash
touch include/gsph/g_fluid_force_muscl.hpp
```

```cpp
#pragma once

#include "fluid_force.hpp"

namespace sph {

class GFluidForceMUSCL : public FluidForce {
public:
    void initialize(std::shared_ptr<SPHParameters> param,
                   std::shared_ptr<Simulation> sim) override;
    
    void calculation(std::shared_ptr<SPHParameters> param,
                    std::shared_ptr<Simulation> sim) override;

private:
    // MUSCL-specific members
    void reconstruct_gradients(/* ... */);
    real m_limiter_param;
};

} // namespace sph
```

#### Step 2: Implement Methods

```bash
touch src/gsph/g_fluid_force_muscl.cpp
```

```cpp
#include "gsph/g_fluid_force_muscl.hpp"
#include "module_factory.hpp"
#include "particle.hpp"
#include "simulation.hpp"

namespace sph {

void GFluidForceMUSCL::initialize(std::shared_ptr<SPHParameters> param,
                                  std::shared_ptr<Simulation> sim) {
    // Setup MUSCL limiter
    m_limiter_param = 1.5;  // or from param
    
    // Base class initialization if needed
    FluidForce::initialize(param, sim);
}

void GFluidForceMUSCL::calculation(std::shared_ptr<SPHParameters> param,
                                   std::shared_ptr<Simulation> sim) {
    auto& particles = sim->get_particles();
    const auto& kernel = sim->get_kernel();
    
    // First pass: compute gradients
    reconstruct_gradients(/* ... */);
    
    // Second pass: compute forces with reconstructed values
    #pragma omp parallel for
    for (size_t i = 0; i < particles.size(); ++i) {
        auto& p_i = particles[i];
        
        // Reset accelerations
        p_i.acc.fill(0.0);
        p_i.dedt = 0.0;
        
        // Loop over neighbors
        for (auto j : p_i.neighbor_indices) {
            // MUSCL reconstruction
            // Riemann solver
            // Accumulate forces
        }
    }
}

void GFluidForceMUSCL::reconstruct_gradients(/* ... */) {
    // Gradient reconstruction logic
}

// Register the module
REGISTER_MODULE("gsph_muscl", "fluid_force", GFluidForceMUSCL);
//              ^^^^^^^^^^^                   ^^^^^^^^^^^^^^^^^
//              New SPH variant name          Implementation class

} // namespace sph
```

#### Step 3: Update CMakeLists (if needed)

Usually not needed - glob patterns pick up new .cpp files automatically:

```cmake
# src/gsph/CMakeLists.txt already has:
file(GLOB GSPH_SOURCES *.cpp)
target_sources(sph_lib PRIVATE ${GSPH_SOURCES})
```

#### Step 4: Use in Configuration

```json
{
  "SPHType": "gsph_muscl",
  ...
}
```

### Module Interface Details

Every module must implement:

```cpp
class MyModule : public ModuleBase {
public:
    // Called once at simulation start
    // - Load parameters
    // - Allocate buffers
    // - Precompute constants
    void initialize(std::shared_ptr<SPHParameters> param,
                   std::shared_ptr<Simulation> sim) override;
    
    // Called every timestep
    // - Perform physics calculation
    // - Update particle state
    void calculation(std::shared_ptr<SPHParameters> param,
                    std::shared_ptr<Simulation> sim) override;
};
```

**Access particle data:**
```cpp
auto& particles = sim->get_particles();
for (auto& p : particles) {
    p.acc += force;  // Modify particle properties
}
```

**Access kernel functions:**
```cpp
const auto& kernel = sim->get_kernel();
real W = kernel->W(r, h);        // Kernel value
auto grad_W = kernel->gradW(r, h);  // Kernel gradient
```

---

## Configuration Guide

### Parameter Categories

#### Time Parameters
```json
{
  "startTime": 0.0,        // Simulation start time
  "endTime": 1.0,          // Simulation end time
  "outputTime": 0.01,      // Particle output interval
  "energyTime": 0.001      // Energy log interval
}
```

#### SPH Method
```json
{
  "SPHType": "gsph",              // "ssph", "disph", "gsph"
  "kernel": "wendland",           // "cubic_spline", "wendland"
  "neighborNumber": 50,           // Target neighbors per particle
  "iterativeSmoothingLength": true,  // Newton iteration for h
  "use2ndOrderGSPH": true        // MUSCL reconstruction (GSPH only)
}
```

#### Physics
```json
{
  "gamma": 1.4,                   // Adiabatic index (ideal gas)
  "useGravity": true,
  "G": 1.0,                       // Gravitational constant
  "theta": 0.5                    // Barnes-Hut opening angle
}
```

#### Artificial Viscosity
```json
{
  "avAlpha": 1.0,                 // AV coefficient
  "useBalsaraSwitch": true,       // Reduce AV in shear
  "useTimeDependentAV": false,    // Time-evolving alpha
  "alphaMax": 2.0,                // Max alpha (if time-dependent)
  "alphaMin": 0.1,                // Min alpha (if time-dependent)
  "epsilonAV": 0.2                // Decay timescale
}
```

#### Artificial Thermal Conductivity
```json
{
  "useArtificialConductivity": false,
  "alphaAC": 1.0                  // AC coefficient
}
```

#### Time Stepping
```json
{
  "cflSound": 0.3,                // Courant factor (sound speed)
  "cflForce": 0.125               // Courant factor (acceleration)
}
```

#### Boundaries
```json
{
  "periodic": true,
  "rangeMin": [-0.5, -0.5, -0.5],
  "rangeMax": [0.5, 0.5, 0.5]
}
```

#### Tree Algorithm
```json
{
  "maxTreeLevel": 20,             // Max tree depth
  "leafParticleNumber": 1         // Particles per leaf node
}
```

### Configuration Inheritance (Proposed)

To reduce duplication, you could extend configs:
```json
{
  "extends": "../../base_configs/gsph_default.json",
  "endTime": 2.0,
  "gamma": 5.0/3.0
}
```

---

## Build System

### Directory Structure
```
sphcode/
├── CMakeLists.txt           # Root config
├── include/
│   └── CMakeLists.txt       # Header exports
├── src/
│   ├── CMakeLists.txt       # Main sources
│   ├── sample/
│   │   └── CMakeLists.txt   # Glob *.cpp
│   ├── production_sims/
│   │   └── CMakeLists.txt   # Glob *.cpp
│   ├── gsph/
│   │   └── CMakeLists.txt   # GSPH modules
│   ├── disph/
│   │   └── CMakeLists.txt   # DISPH modules
│   └── relaxation/
│       └── CMakeLists.txt   # Relaxation modules
└── test/
    └── CMakeLists.txt       # Test suite
```

### Build Targets

```bash
# Main executable
make sph                     # Builds ./sph

# Shared library (for linking tests)
make sph_lib                 # Builds libsph_lib.so/.dylib

# Tests
make run_kernel_test         # Builds and runs kernel tests
```

### Adding New Subdirectories

If you add a new module category (e.g., `src/mhd/`):

1. Create `src/mhd/CMakeLists.txt`:
```cmake
file(GLOB MHD_SOURCES *.cpp)
target_sources(sph_lib PRIVATE ${MHD_SOURCES})
```

2. Add to `src/CMakeLists.txt`:
```cmake
add_subdirectory(mhd)
```

3. Files auto-included on rebuild

### Compilation Flags

Current flags (see root `CMakeLists.txt`):
```cmake
-Wall                    # All warnings
-Wno-sign-compare        # Suppress sign comparison warnings
-Wno-maybe-uninitialized # Suppress maybe-uninitialized warnings
-funroll-loops           # Aggressive loop unrolling
-ffast-math              # Fast floating-point math (may reduce precision)
```

**Dimension Setting:**
Edit `include/defines.hpp`:
```cpp
#define DIM 2  // 1, 2, or 3
```
Then rebuild entirely:
```bash
rm -rf build && mkdir build && cd build && cmake .. && make
```

---

## Debugging and Testing

### Enabling Debug Builds

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cd build && make
```

Debug flags:
- No optimizations (`-O0`)
- Debug symbols (`-g`)
- Assertions enabled

### Using GDB

```bash
gdb --args ./sph shock_tube ../sample/shock_tube/shock_tube.json 1
(gdb) break Solver::integrate
(gdb) run
(gdb) print particles[0]
```

### Common Issues

#### 1. Dimension Mismatch
```
Error: DIM != 2 for shock_tube_2d
```
**Fix:** Set `DIM` in `include/defines.hpp` and recompile.

#### 2. Sample Not Found
```
Sample 'my_sim' not registered
```
**Fix:** Ensure `REGISTER_SAMPLE` macro is called and file is compiled into `sph_lib`.

#### 3. OpenMP Not Found
```
Could NOT find OpenMP_CXX
```
**Fix:** Install libomp and pass correct flags to CMake (see Quick Start).

#### 4. Boost Not Found
```
Could NOT find Boost
```
**Fix:** `brew install boost` or install via system package manager.

### Logging

Logs are written to `<outputDirectory>/log.txt`:
```cpp
WRITE_LOG << "Important message";           // Console + file
WRITE_LOG_ONLY << "Verbose message";        // File only
```

Check logs for:
- Time step sizes
- Particle counts
- Energy drift (conservation check)

### Unit Tests

Currently limited to kernel tests:
```bash
./sph kernel_test
```

Checks:
- Kernel normalization
- Gradient accuracy
- Smoothing length iteration

**Adding more tests:**
1. Create `test/my_test/`
2. Implement test sample
3. Add target to `test/CMakeLists.txt`

---

## Best Practices

### Performance

1. **Parallelize particle loops:**
   ```cpp
   #pragma omp parallel for
   for (size_t i = 0; i < particles.size(); ++i) {
       // Thread-safe operations only
   }
   ```

2. **Avoid allocations in hot paths:**
   ```cpp
   // Bad
   for (auto& p : particles) {
       std::vector<int> neighbors = find_neighbors(p);  // Allocates every iteration
   }
   
   // Good
   std::vector<int> neighbors;
   neighbors.reserve(100);
   for (auto& p : particles) {
       find_neighbors(p, neighbors);  // Reuse buffer
       neighbors.clear();
   }
   ```

3. **Use tree for O(N log N) neighbor search:**
   ```cpp
   sim->make_tree();  // Rebuild after particle movement
   ```

### Numerical Stability

1. **Check CFL conditions:**
   - `cflSound < 0.5` for sound wave stability
   - `cflForce < 0.25` for force stability

2. **Monitor energy drift:**
   Check `energy.txt` output - should be < 1% for most problems

3. **Use Balsara switch:**
   Reduces artificial viscosity in shear, improves instability tests

4. **Choose appropriate kernel:**
   - Cubic spline: Classic, good for low neighbor counts
   - Wendland C4: Better for high neighbor counts, prevents pairing

### Code Organization

1. **Keep samples focused:**
   - One physics problem per sample
   - Avoid complex initialization - consider splitting

2. **Document non-obvious parameters:**
   ```cpp
   const real theta = 0.5;  // Barnes-Hut opening angle for tree
   ```

3. **Use namespaces:**
   ```cpp
   namespace {  // Anonymous namespace for file-local helpers
       real helper_function() { /* ... */ }
   }
   ```

4. **Separate concerns:**
   - Physics in modules
   - Initial conditions in samples
   - Configuration in JSON

### Version Control

**Do commit:**
- Source files (`.cpp`, `.hpp`)
- Configuration files (`.json`)
- Build files (`CMakeLists.txt`, `Makefile`)
- Documentation (`.md`)

**Don't commit:**
- Build artifacts (`build/`, `*.o`, `sph` binary)
- Results (`results/`)
- Logs

---

## Next Steps

1. **Explore samples:** Run different test cases to understand physics
2. **Modify parameters:** Experiment with JSON configs
3. **Add your simulation:** Follow "Adding New Simulations" guide
4. **Implement algorithm:** Follow "Adding New Algorithms" guide
5. **Contribute:** Submit pull requests with improvements

## Getting Help

- **Code questions:** Check `.serena/` memories for architecture details
- **Physics questions:** See references in main `README.md`
- **Build issues:** Review "Common Issues" in Debugging section

Happy simulating! 🚀
