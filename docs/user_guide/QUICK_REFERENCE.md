# GSPH Quick Extension Guide

Quick reference for adding new components to GSPHCODE.

## Quick Links
- [Add Sample Simulation](#add-sample-simulation-5-minutes)
- [Add Production Simulation](#add-production-simulation-5-minutes)
- [Add New SPH Algorithm](#add-new-sph-algorithm-30-minutes)
- [Add Physics Module](#add-physics-module-1-hour)

---

## Add Sample Simulation (5 minutes)

### 1. Create file: `src/sample/my_sim.cpp`

```cpp
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "sample_registry.hpp"

namespace {
using namespace sph;

void load_my_sim(std::shared_ptr<Simulation> sim,
                 std::shared_ptr<SPHParameters> param) {
    #if DIM != 2
        THROW_ERROR("my_sim requires DIM == 2");
    #endif
    
    // Create particles
    std::vector<SPHParticle> particles;
    const int N = 100;
    const real dx = 1.0 / N;
    
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            SPHParticle p;
            p.pos[0] = i * dx;
            p.pos[1] = j * dx;
            p.vel[0] = 0.0;
            p.vel[1] = 0.0;
            p.dens = 1.0;
            p.pres = 1.0;
            p.mass = dx * dx;
            p.ene = p.pres / ((param->physics.gamma - 1.0) * p.dens);
            p.id = particles.size();
            particles.push_back(p);
        }
    }
    
    sim->set_particles(particles);
    sim->set_particle_num(particles.size());
}

REGISTER_SAMPLE("my_sim", load_my_sim);

} // namespace
```

### 2. Create config: `sample/my_sim/my_sim.json`

```json
{
  "outputDirectory": "results/my_sim",
  "endTime": 1.0,
  "outputTime": 0.01,
  "SPHType": "gsph",
  "gamma": 1.4,
  "kernel": "wendland",
  "neighborNumber": 50,
  "cflSound": 0.3
}
```

### 3. Build and run

```bash
cd build && make
./sph my_sim ../sample/my_sim/my_sim.json 8
```

**Done!** No registration lists to update.

---

## Add Production Simulation (5 minutes)

Same as sample, but:
- File location: `src/production_sims/my_prod.cpp`
- Config location: `production_sims/my_prod/my_prod.json`

---

## Add New SPH Algorithm (30 minutes)

### When?
- Implementing new variant (e.g., "gsph_muscl", "anisotropic_sph")
- Alternative fluid force calculation
- Different time integration scheme

### 1. Create module header: `include/gsph/my_fluid_force.hpp`

```cpp
#pragma once
#include "fluid_force.hpp"

namespace sph {

class MyFluidForce : public FluidForce {
public:
    void initialize(std::shared_ptr<SPHParameters> param,
                   std::shared_ptr<Simulation> sim) override;
    void calculation(std::shared_ptr<SPHParameters> param,
                    std::shared_ptr<Simulation> sim) override;
private:
    // Algorithm-specific members
    real m_my_parameter;
};

} // namespace sph
```

### 2. Implement: `src/gsph/my_fluid_force.cpp`

```cpp
#include "gsph/my_fluid_force.hpp"
#include "module_factory.hpp"
#include "particle.hpp"

namespace sph {

void MyFluidForce::initialize(std::shared_ptr<SPHParameters> param,
                              std::shared_ptr<Simulation> sim) {
    m_my_parameter = 1.5;  // Load from param if needed
}

void MyFluidForce::calculation(std::shared_ptr<SPHParameters> param,
                               std::shared_ptr<Simulation> sim) {
    auto& particles = sim->get_particles();
    const auto& kernel = sim->get_kernel();
    
    #pragma omp parallel for
    for (size_t i = 0; i < particles.size(); ++i) {
        auto& p_i = particles[i];
        p_i.acc.fill(0.0);
        p_i.dedt = 0.0;
        
        for (auto j : p_i.neighbor_indices) {
            auto& p_j = particles[j];
            // Your algorithm here
            // p_i.acc += force;
            // p_i.dedt += energy_change;
        }
    }
}

// Register for a new SPH type
REGISTER_MODULE("my_sph_type", "fluid_force", MyFluidForce);

} // namespace sph
```

### 3. Use in config

```json
{
  "SPHType": "my_sph_type"
}
```

### Module Types Available
- `"pre_interaction"` → Inherit from `PreInteraction`
- `"fluid_force"` → Inherit from `FluidForce`
- `"gravity_force"` → Inherit from `GravityForce`
- `"timestep"` → Inherit from `Timestep`
- `"heating_cooling"` → Inherit from `HeatingCooling`

---

## Add Physics Module (1 hour)

### When?
- Adding new physics (MHD, radiation, chemistry)
- Creating new module type beyond existing five

### 1. Define base interface: `include/my_module.hpp`

```cpp
#pragma once
#include "module.hpp"

namespace sph {

class MyModule : public Module {
public:
    virtual ~MyModule() = default;
    
    void initialize(std::shared_ptr<SPHParameters> param,
                   std::shared_ptr<Simulation> sim) override = 0;
    void calculation(std::shared_ptr<SPHParameters> param,
                    std::shared_ptr<Simulation> sim) override = 0;
    
    // Module-specific interface
    virtual void my_special_function() = 0;
};

} // namespace sph
```

### 2. Implement for each SPH type needed

`include/gsph/g_my_module.hpp`:
```cpp
#pragma once
#include "my_module.hpp"

namespace sph {

class GMyModule : public MyModule {
public:
    void initialize(...) override;
    void calculation(...) override;
    void my_special_function() override;
};

} // namespace sph
```

`src/gsph/g_my_module.cpp`:
```cpp
#include "gsph/g_my_module.hpp"
#include "module_factory.hpp"

namespace sph {

void GMyModule::initialize(...) { /* ... */ }
void GMyModule::calculation(...) { /* ... */ }
void GMyModule::my_special_function() { /* ... */ }

REGISTER_MODULE("gsph", "my_module_type", GMyModule);

} // namespace sph
```

### 3. Integrate into Solver: `include/solver.hpp`

Add member:
```cpp
class Solver {
private:
    std::shared_ptr<MyModule> m_my_module;
};
```

### 4. Initialize in Solver: `src/solver.cpp`

```cpp
void Solver::initialize() {
    // ... existing code ...
    
    m_my_module = ModuleFactory::create_module<MyModule>(
        m_param->sph.type,
        "my_module_type"
    );
    m_my_module->initialize(m_param, m_sim);
}

void Solver::integrate() {
    // ... existing code ...
    m_my_module->calculation(m_param, m_sim);
}
```

---

## Common Patterns

### Accessing Particle Data
```cpp
auto& particles = sim->get_particles();
for (auto& p : particles) {
    p.acc += acceleration;
    p.dedt += energy_rate;
}
```

### Neighbor Iteration
```cpp
for (size_t i = 0; i < particles.size(); ++i) {
    auto& p_i = particles[i];
    for (auto j : p_i.neighbor_indices) {
        auto& p_j = particles[j];
        Vector r_ij = p_i.pos - p_j.pos;
        real r = r_ij.norm();
        // Pairwise calculation
    }
}
```

### Using Kernel Functions
```cpp
const auto& kernel = sim->get_kernel();
real h = 0.5 * (p_i.sml + p_j.sml);
real W = kernel->W(r, h);           // Kernel value
auto grad_W = kernel->gradW(r_ij, h);  // Gradient vector
```

### Parallel Loops
```cpp
#pragma omp parallel for
for (size_t i = 0; i < particles.size(); ++i) {
    // Thread-safe operations only
    // No shared state modification without atomics/locks
}
```

### Error Handling
```cpp
if (invalid_condition) {
    THROW_ERROR("Description of what went wrong");
}
```

### Conditional Compilation
```cpp
#if DIM == 1
    // 1D-specific code
#elif DIM == 2
    // 2D-specific code
#elif DIM == 3
    // 3D-specific code
#endif
```

---

## Parameter Access

### Physics Parameters
```cpp
real gamma = param->physics.gamma;           // Adiabatic index
bool use_gravity = param->physics.useGravity;
real G = param->physics.G;                   // Gravitational constant
```

### SPH Parameters
```cpp
std::string sph_type = param->sph.type;      // "ssph", "disph", "gsph"
std::string kernel = param->sph.kernel;      // "cubic_spline", "wendland"
int neighbors = param->sph.neighborNumber;
```

### Time Parameters
```cpp
real t_end = param->time.end;
real dt_output = param->time.output;
```

### Adding New Parameters

1. Update `include/parameters.hpp`:
```cpp
struct PhysicsParams {
    real gamma;
    bool useGravity;
    real my_new_param;  // Add here
};
```

2. Load in `src/solver.cpp` (or use JSON):
```cpp
param->physics.my_new_param = json["myNewParam"];
```

---

## Build System Notes

### Files Auto-Included
All `.cpp` files in these directories are automatically compiled:
- `src/sample/*.cpp`
- `src/production_sims/*.cpp`
- `src/gsph/*.cpp`
- `src/disph/*.cpp`
- `src/relaxation/*.cpp`

No need to update CMakeLists.txt!

### Adding New Directory
If creating new category (e.g., `src/mhd/`):

1. Create `src/mhd/CMakeLists.txt`:
```cmake
file(GLOB MHD_SOURCES *.cpp)
target_sources(sph_lib PRIVATE ${MHD_SOURCES})
```

2. Add to `src/CMakeLists.txt`:
```cmake
add_subdirectory(mhd)
```

---

## Checklist Before Commit

- [ ] Code compiles: `cd build && make`
- [ ] Set correct `DIM` in `include/defines.hpp`
- [ ] Sample registered with `REGISTER_SAMPLE`
- [ ] Module registered with `REGISTER_MODULE`
- [ ] JSON config created
- [ ] Tested execution
- [ ] No debug print statements left in code
- [ ] Parallel loops use `#pragma omp parallel for`

---

## Examples Repository

Check these existing implementations for reference:

**Simple Sample:**
- `src/sample/shock_tube.cpp` - Basic 1D test
- `src/sample/sedov_taylor.cpp` - Point explosion

**Complex Sample:**
- `src/sample/khi.cpp` - Kelvin-Helmholtz instability
- `src/sample/evrard.cpp` - Self-gravitating collapse

**Module Implementation:**
- `src/gsph/g_fluid_force.cpp` - Godunov fluid forces
- `src/disph/d_pre_interaction.cpp` - DISPH density calc

**Production Simulation:**
- `src/production_sims/razor_thin_hvcc.cpp` - Research-grade setup

---

## Getting Unstuck

**"Sample not found"**
→ Check `REGISTER_SAMPLE` macro is called
→ Verify file is in `src/sample/` or `src/production_sims/`
→ Rebuild: `cd build && make`

**"Module not registered"**
→ Check `REGISTER_MODULE` macro
→ Verify SPH type string matches config JSON
→ Module type must be known: pre_interaction, fluid_force, etc.

**"DIM mismatch"**
→ Edit `include/defines.hpp`
→ Full rebuild: `rm -rf build && mkdir build && cd build && cmake .. && make`

**"Segmentation fault"**
→ Check array bounds
→ Verify neighbor lists are built (call `sim->make_tree()`)
→ Run with debugger: `gdb --args ./sph ...`

**"Performance is slow"**
→ Check OpenMP is enabled: `#ifdef _OPENMP` in logs
→ Use tree: `sim->make_tree()` before neighbor search
→ Profile with: `perf record ./sph ...` (Linux)

---

For detailed explanations, see `DEVELOPER_GUIDE.md`.
