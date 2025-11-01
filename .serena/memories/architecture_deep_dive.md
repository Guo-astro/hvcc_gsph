# Architecture Deep Dive

## Execution Flow

### Main Entry Point
```
main() 
  → exception_handler()
    → Solver::Solver(argc, argv)  # Parse arguments
      → Solver::run()
        → initialize()              # Set up simulation
        → while (t < t_end):
            integrate()             # Time stepping
            output()                # Write results
```

### Solver Initialization
1. Parse command-line arguments:
   - `argv[1]`: Sample name
   - `argv[2]`: JSON file OR thread count
   - `argv[3]`: Thread count (if argv[2] was JSON)

2. Create simulation components:
   - `m_param`: SPHParameters (from JSON if provided)
   - `m_sim`: Simulation state
   - `m_output`: Output handler
   - Modules: timestep, pre-interaction, fluid force, gravity, heating/cooling

3. Call sample creator via `SampleRegistry::create_sample()`

4. Initialize modules via `ModuleFactory::create_module()`

### Integration Loop
```
integrate():
  → predict()      # Predictor step (kick-drift-kick)
    - Update velocities
    - Update positions
  → correct()      # Corrector step
    - Rebuild tree
    - Find neighbors
    - Calculate forces
    - Update velocities/energies
```

## Registration Systems

### Sample Registration
**Purpose**: Register simulation initial conditions

**Mechanism**: Static initialization at program startup
```cpp
REGISTER_SAMPLE("name", function)
  → Creates static bool in anonymous namespace
  → Calls SampleRegistry::register_sample() during static init
  → Maps "name" → function pointer
```

**Usage**: `SampleRegistry::create_sample(name, sim, param)`
- Looks up function by name
- Calls function to initialize particles

### Module Registration
**Purpose**: Register SPH-variant-specific implementations

**Mechanism**: Template-based factory pattern
```cpp
REGISTER_MODULE(sph_type, module_type, ClassName)
  → Creates static struct with constructor
  → Calls ModuleFactory::register_module()
  → Maps (sph_type, module_type) → creator function
```

**Module Types**:
- `"pre_interaction"`: Neighbor search, density calculation
- `"fluid_force"`: Pressure forces, artificial viscosity
- `"gravity_force"`: Self-gravity computation
- `"timestep"`: Adaptive time step calculation
- `"heating_cooling"`: Thermal energy sources/sinks

**SPH Types**:
- `"ssph"`: Standard SPH
- `"disph"`: Density Independent SPH
- `"gsph"`: Godunov SPH

## Module System Architecture

### Module Interface
All modules implement:
```cpp
void initialize(param, sim);  # One-time setup
void calculation(param, sim); # Per-step computation
```

### Module Specialization
Each SPH variant has specialized modules in subdirectories:
- `src/gsph/` - Godunov SPH implementations
- `src/disph/` - DISPH implementations
- Base implementations in `src/` (used by SSPH)

Example specializations:
- `GPreInteraction` (gsph) vs `DPreInteraction` (disph)
- `GFluidForce` vs `DFluidForce`

### Module Selection
In `Solver::initialize()`:
```cpp
m_pre = ModuleFactory::create_module<PreInteraction>(sph_type, "pre_interaction");
m_fforce = ModuleFactory::create_module<FluidForce>(sph_type, "fluid_force");
```
Based on `param->sph.type`, appropriate implementation is instantiated.

## Data Flow

### Particle Data
`SPHParticle` struct contains:
- Position, velocity, mass
- Density, pressure, energy
- Smoothing length
- Acceleration, time derivative of energy
- Neighbor lists

Stored in: `Simulation::m_particles` (std::vector)

### Parameters
`SPHParameters` struct hierarchy:
- `time`: start, end, output intervals
- `sph`: type (ssph/disph/gsph), kernel, neighbor count
- `physics`: gamma (adiabatic index), gravity
- `av`: artificial viscosity parameters
- `periodic`: boundary conditions

Loaded from JSON, can override via `parseJsonOverrides()`

### Tree Structure
`BHTree` for:
- Fast neighbor search (O(N log N))
- Gravity force calculation with Barnes-Hut

Rebuilt each timestep when particles move.

## Extension Points

### Adding New Samples
1. Create `src/sample/my_sim.cpp`
2. Implement `void load_my_sim(sim, param)`
3. Add `REGISTER_SAMPLE("my_sim", load_my_sim)`
4. Create `sample/my_sim/my_sim.json` config
5. Recompile - auto-registered

### Adding New Production Sims
Same as samples but in `src/production_sims/`

### Adding New Algorithms
1. Derive from appropriate module base class
2. Implement `initialize()` and `calculation()`
3. Add `REGISTER_MODULE(sph_type, module_type, MyClass)`
4. Recompile - auto-registered

No central list to update - registration is automatic!
