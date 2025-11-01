# GSPH Architecture Diagrams

Visual guide to understanding GSPHCODE's architecture.

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                         GSPHCODE System                          │
│                                                                  │
│  CLI Entry → Solver → Simulation State → Physics Modules        │
│                  ↓                                               │
│              Integration Loop                                    │
│                  ↓                                               │
│          Output (HDF5/Text)                                      │
└─────────────────────────────────────────────────────────────────┘
```

## Component Hierarchy

```
main.cpp
  └── Solver
       ├── Simulation (state management)
       │    ├── Particles (std::vector<SPHParticle>)
       │    ├── BHTree (spatial indexing)
       │    ├── KernelFunction (W, gradW)
       │    └── Periodic (boundary conditions)
       │
       ├── SPHParameters (configuration)
       │    ├── TimeParams (start, end, output)
       │    ├── SPHParams (type, kernel, neighbors)
       │    ├── PhysicsParams (gamma, gravity)
       │    └── AVParams (artificial viscosity)
       │
       ├── Output (I/O management)
       │
       └── Physics Modules (via ModuleFactory)
            ├── PreInteraction (neighbors, density)
            ├── FluidForce (pressure, viscosity)
            ├── GravityForce (self-gravity)
            ├── Timestep (adaptive dt)
            └── HeatingCooling (energy sources)
```

## Execution Flow

```
┌─────────────┐
│   main()    │
└──────┬──────┘
       │
       ▼
┌────────────────────────────────────────┐
│ Solver::Solver(argc, argv)             │
│                                         │
│ 1. Parse command line                  │
│    - sample_name                        │
│    - config.json (optional)             │
│    - num_threads                        │
│                                         │
│ 2. Setup logging                        │
│ 3. Configure OpenMP                     │
└────────┬───────────────────────────────┘
         │
         ▼
┌────────────────────────────────────────┐
│ Solver::run()                           │
│                                         │
│ ┌────────────────────────────────────┐ │
│ │ initialize()                        │ │
│ │                                     │ │
│ │ • Load sample via SampleRegistry   │ │
│ │ • Create modules via ModuleFactory │ │
│ │ • Initialize all modules           │ │
│ └────────────────────────────────────┘ │
│                                         │
│ ┌────────────────────────────────────┐ │
│ │ while (t < t_end):                 │ │
│ │   ┌──────────────────────────────┐ │ │
│ │   │ integrate()                   │ │ │
│ │   │                               │ │ │
│ │   │ predict():                    │ │ │
│ │   │   • Kick velocities (dt/2)   │ │ │
│ │   │   • Drift positions (dt)     │ │ │
│ │   │                               │ │ │
│ │   │ correct():                    │ │ │
│ │   │   • make_tree()              │ │ │
│ │   │   • pre_interaction()        │ │ │
│ │   │   • fluid_force()            │ │ │
│ │   │   • gravity_force()          │ │ │
│ │   │   • heating_cooling()        │ │ │
│ │   │   • timestep()               │ │ │
│ │   │   • Kick velocities (dt/2)   │ │ │
│ │   └──────────────────────────────┘ │ │
│ │                                     │ │
│ │   output_particle() if t > t_out   │ │
│ │   output_energy() if t > t_ene     │ │
│ └────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

## Registration System

### Sample Registration

```
Compile Time:
┌─────────────────────────────────────────────────────┐
│ src/sample/shock_tube.cpp                           │
│                                                      │
│ namespace {                                          │
│   void load_shock_tube(sim, param) { ... }          │
│   REGISTER_SAMPLE("shock_tube", load_shock_tube);   │
│ }                                                    │
│          │                                           │
│          │ (macro expansion)                         │
│          ▼                                           │
│ static const bool s_registered_load_shock_tube =    │
│     []() {                                           │
│         SampleRegistry::instance()                   │
│             .register_sample("shock_tube",           │
│                              load_shock_tube);       │
│         return true;                                 │
│     }();                                             │
│          │                                           │
│          │ (static initialization before main)      │
│          ▼                                           │
│ ┌────────────────────────────────────┐              │
│ │ SampleRegistry                     │              │
│ │ m_registry["shock_tube"] = fn_ptr  │              │
│ └────────────────────────────────────┘              │
└─────────────────────────────────────────────────────┘

Runtime:
┌─────────────────────────────────────────────────────┐
│ Solver::initialize()                                 │
│   SampleRegistry::create_sample("shock_tube", ...)  │
│          │                                           │
│          ▼                                           │
│   Lookup "shock_tube" in m_registry                 │
│          │                                           │
│          ▼                                           │
│   Call function pointer: load_shock_tube(sim, param)│
│          │                                           │
│          ▼                                           │
│   Particles initialized!                            │
└─────────────────────────────────────────────────────┘
```

### Module Registration

```
Compile Time:
┌─────────────────────────────────────────────────────┐
│ src/gsph/g_fluid_force.cpp                          │
│                                                      │
│ class GFluidForce : public FluidForce { ... }       │
│ REGISTER_MODULE("gsph", "fluid_force", GFluidForce);│
│          │                                           │
│          │ (macro expansion)                         │
│          ▼                                           │
│ static struct Register_GFluidForce {                │
│   Register_GFluidForce() {                          │
│     ModuleFactory::register_module(                 │
│       "gsph", "fluid_force",                        │
│       []() { return new GFluidForce(); }            │
│     );                                               │
│   }                                                  │
│ } s_register_GFluidForce;                           │
│          │                                           │
│          │ (static initialization before main)      │
│          ▼                                           │
│ ┌────────────────────────────────────────┐          │
│ │ ModuleFactory                          │          │
│ │ registry_[("gsph","fluid_force")] =    │          │
│ │     creator_function                   │          │
│ └────────────────────────────────────────┘          │
└─────────────────────────────────────────────────────┘

Runtime:
┌─────────────────────────────────────────────────────┐
│ Solver::initialize()                                 │
│   sph_type = param->sph.type;  // "gsph"            │
│   m_fforce = ModuleFactory::create_module<          │
│                FluidForce>(sph_type, "fluid_force");│
│          │                                           │
│          ▼                                           │
│   Lookup ("gsph", "fluid_force") in registry_       │
│          │                                           │
│          ▼                                           │
│   Call creator function                             │
│          │                                           │
│          ▼                                           │
│   return new GFluidForce();                         │
└─────────────────────────────────────────────────────┘
```

## Module Interaction

```
┌────────────────────────────────────────────────────────┐
│              Integration Timestep                       │
└────────┬───────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────┐
│ PreInteraction      │  Dependencies: None
│                     │  Provides: neighbor lists, density
│ • Build tree        │
│ • Find neighbors    │
│ • Compute density   │
└────────┬────────────┘
         │
         ├─────────────────┐
         │                 │
         ▼                 ▼
┌─────────────────┐  ┌──────────────────┐
│ FluidForce      │  │ GravityForce     │
│                 │  │                  │
│ Needs:          │  │ Needs:           │
│ • neighbors     │  │ • tree           │
│ • density       │  │                  │
│                 │  │ Computes:        │
│ Computes:       │  │ • acc (gravity)  │
│ • acc (pressure)│  └──────────────────┘
│ • dedt          │
└────────┬────────┘
         │
         ▼
┌─────────────────────┐
│ HeatingCooling      │  Dependencies: FluidForce
│                     │  Modifies: dedt
│ • Energy sources    │
│ • Energy sinks      │
└────────┬────────────┘
         │
         ▼
┌─────────────────────┐
│ Timestep            │  Dependencies: All forces computed
│                     │  Provides: dt
│ • Compute CFL       │
│ • Set next dt       │
└─────────────────────┘
```

## Data Flow

```
SPHParticle Structure:
┌────────────────────────────────────────┐
│ Position:   pos[DIM]                   │ ← Updated by drift
│ Velocity:   vel[DIM]                   │ ← Updated by kick
│ Mass:       mass                       │ ← Constant
│ Density:    dens                       │ ← PreInteraction
│ Pressure:   pres                       │ ← PreInteraction (EOS)
│ Energy:     ene                        │ ← Updated by kick
│ Smoothing:  sml                        │ ← PreInteraction
│                                        │
│ Derivatives:                           │
│ Acceleration: acc[DIM]                 │ ← FluidForce + GravityForce
│ Energy rate:  dedt                     │ ← FluidForce + HeatingCooling
│                                        │
│ Neighbors:                             │
│ neighbor_indices: vector<int>          │ ← PreInteraction (tree search)
└────────────────────────────────────────┘

Flow per timestep:
┌────────────┐   ┌──────────────┐   ┌─────────────┐
│ pos, vel   │ → │ PreInteration│ → │ dens, pres  │
│ (previous) │   │ (tree search)│   │ sml, neighb │
└────────────┘   └──────────────┘   └──────┬──────┘
                                            │
                      ┌─────────────────────┤
                      │                     │
                      ▼                     ▼
              ┌──────────────┐      ┌─────────────┐
              │ FluidForce   │      │ GravityForce│
              │              │      │             │
              │ dens, pres → │      │ pos, mass → │
              │ → acc, dedt  │      │ → acc       │
              └──────┬───────┘      └──────┬──────┘
                     │                     │
                     └──────────┬──────────┘
                                │
                                ▼
                        ┌───────────────┐
                        │ acc, dedt     │
                        │ (summed)      │
                        └───────┬───────┘
                                │
                                ▼
                        ┌───────────────┐
                        │ Timestep      │
                        │ acc → dt      │
                        └───────┬───────┘
                                │
                                ▼
                        ┌───────────────┐
                        │ Kick (update) │
                        │ vel += acc*dt │
                        │ ene += dedt*dt│
                        └───────────────┘
```

## SPH Variant Specialization

```
                    ┌──────────────┐
                    │   Module     │
                    │  Interface   │
                    └───┬──────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│    SSPH      │ │    DISPH     │ │    GSPH      │
│  (Standard)  │ │ (Density Ind)│ │  (Godunov)   │
└──────┬───────┘ └──────┬───────┘ └──────┬───────┘
       │                │                │
       │                │                │
PreInteraction:         │                │
src/pre_interaction.cpp │                │
       │                │                │
       │         DPreInteraction         │
       │         (d_pre_interaction.cpp) │
       │                │                │
       │                │         GPreInteraction
       │                │         (g_pre_interaction.cpp)
       │                │                │
FluidForce:            │                │
src/fluid_force.cpp    │                │
       │         DFluidForce             │
       │         (d_fluid_force.cpp)     │
       │                │                │
       │                │         GFluidForce
       │                │         (g_fluid_force.cpp)
       │                │                │
       ▼                ▼                ▼
   Registered      Registered       Registered
   as "ssph"       as "disph"       as "gsph"
```

## File Organization Map

```
sphcode/
│
├── include/                      # Public interfaces
│   ├── simulation.hpp            # Core: Simulation class
│   ├── solver.hpp                # Core: Solver class
│   ├── particle.hpp              # Data: SPHParticle struct
│   ├── parameters.hpp            # Data: SPHParameters struct
│   ├── module.hpp                # Interface: Module base
│   ├── module_factory.hpp        # Pattern: Factory + Registry
│   ├── sample_registry.hpp       # Pattern: Registry
│   │
│   ├── kernel/                   # Algorithms: Kernel functions
│   │   ├── cubic_spline.hpp
│   │   └── wendland.hpp
│   │
│   ├── gsph/                     # Specialization: GSPH
│   │   ├── g_pre_interaction.hpp
│   │   └── g_fluid_force.hpp
│   │
│   ├── disph/                    # Specialization: DISPH
│   │   ├── d_pre_interaction.hpp
│   │   └── d_fluid_force.hpp
│   │
│   └── utilities/                # Helpers
│
├── src/                          # Implementations
│   ├── main.cpp                  # Entry point
│   ├── solver.cpp                # Solver logic
│   ├── simulation.cpp            # State management
│   ├── *_force.cpp               # Base implementations (SSPH)
│   ├── sample_registry.cpp       # Registry implementation
│   │
│   ├── sample/                   # Test cases
│   │   ├── CMakeLists.txt        # Auto-glob *.cpp
│   │   ├── shock_tube.cpp
│   │   ├── evrard.cpp
│   │   └── khi.cpp
│   │
│   ├── production_sims/          # Research simulations
│   │   ├── CMakeLists.txt
│   │   └── razor_thin_hvcc.cpp
│   │
│   ├── gsph/                     # GSPH implementations
│   │   ├── CMakeLists.txt
│   │   ├── g_pre_interaction.cpp
│   │   └── g_fluid_force.cpp
│   │
│   └── disph/                    # DISPH implementations
│       ├── CMakeLists.txt
│       ├── d_pre_interaction.cpp
│       └── d_fluid_force.cpp
│
├── sample/                       # Configuration files
│   ├── shock_tube/
│   │   └── shock_tube.json
│   └── evrard/
│       └── evrard.json
│
└── production_sims/              # Production configs
    └── razor_thin_hvcc/
        └── razor_thin_hvcc.json
```

## Extension Points Map

```
Want to add...              Create file...                    Use pattern...
───────────────────────────────────────────────────────────────────────────────
New test simulation      src/sample/my_test.cpp            REGISTER_SAMPLE
New production sim       src/production_sims/my_prod.cpp   REGISTER_SAMPLE
New SPH variant          src/my_sph/*.cpp                  REGISTER_MODULE (all)
New physics (all SPH)    src/my_physics.cpp                REGISTER_MODULE
New physics (one SPH)    src/gsph/g_my_physics.cpp         REGISTER_MODULE
New kernel function      include/kernel/my_kernel.hpp      Inherit KernelFunction
New parameter category   include/parameters.hpp (add)      Struct member
New output format        src/output.cpp (modify)           Extend Output class
```

## Threading Model

```
┌─────────────────────────────────────────────────────┐
│ main thread (master)                                 │
│                                                      │
│ ┌────────────────────────────────────────────────┐  │
│ │ Solver::integrate()                            │  │
│ │                                                │  │
│ │   [Serial] predict() - update positions       │  │
│ │                                                │  │
│ │   [Serial] make_tree()                        │  │
│ │                                                │  │
│ │   [Parallel] PreInteraction::calculation()    │  │
│ │   ┌──────────────────────────────────────┐    │  │
│ │   │ #pragma omp parallel for             │    │  │
│ │   │ for (i : particles):                 │    │  │
│ │   │   find_neighbors(i)                  │    │  │
│ │   │   compute_density(i)                 │    │  │
│ │   └──────────────────────────────────────┘    │  │
│ │                                                │  │
│ │   [Parallel] FluidForce::calculation()        │  │
│ │   ┌──────────────────────────────────────┐    │  │
│ │   │ #pragma omp parallel for             │    │  │
│ │   │ for (i : particles):                 │    │  │
│ │   │   for (j : neighbors[i]):            │    │  │
│ │   │     compute_pair_force(i, j)         │    │  │
│ │   └──────────────────────────────────────┘    │  │
│ │                                                │  │
│ │   [Parallel] GravityForce (if enabled)        │  │
│ │   [Parallel] HeatingCooling (if enabled)      │  │
│ │                                                │  │
│ │   [Serial] Timestep::calculation() - reduce   │  │
│ │                                                │  │
│ │   [Serial] correct() - update velocities      │  │
│ └────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘

Thread Safety Notes:
• Each particle writes to its own data (thread-safe)
• Neighbor lists are read-only after construction
• Tree is read-only after make_tree()
• No atomics needed for force accumulation (disjoint writes)
```

## Memory Layout

```
Simulation::m_particles: std::vector<SPHParticle>
┌────────────────────────────────────────────────────┐
│ [0] SPHParticle                                    │
│     pos[3], vel[3], acc[3], mass, dens, ...        │
├────────────────────────────────────────────────────┤
│ [1] SPHParticle                                    │
│     pos[3], vel[3], acc[3], mass, dens, ...        │
├────────────────────────────────────────────────────┤
│ ...                                                 │
├────────────────────────────────────────────────────┤
│ [N-1] SPHParticle                                  │
└────────────────────────────────────────────────────┘
         ↑
         │ Stored contiguously (cache-friendly)
         │ AoS (Array of Structures) layout
         
BHTree: Hierarchical spatial index
┌────────────────────────────────────┐
│ Root Node (level 0)                │
│ ├─ Child 0 (level 1)               │
│ │  ├─ Child 0 (level 2)            │
│ │  │  └─ Particles: [0, 5, 12]     │
│ │  └─ Child 1 (level 2)            │
│ │     └─ Particles: [3, 7]         │
│ ├─ Child 1 (level 1)               │
│ │  └─ ...                           │
│ └─ ...                              │
└────────────────────────────────────┘
```

---

For implementation details, see `DEVELOPER_GUIDE.md`.
For quick recipes, see `QUICK_REFERENCE.md`.
