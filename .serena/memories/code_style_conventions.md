# Code Style and Conventions

## Naming Conventions

### C++ Identifiers
- **Classes**: PascalCase (e.g., `Simulation`, `SPHParticle`, `BHTree`)
- **Functions/Methods**: camelCase (e.g., `calculate_pressure`, `make_tree`, `get_particles`)
- **Member variables**: Prefix with `m_` + camelCase (e.g., `m_particles`, `m_particle_num`, `m_timestep`)
- **Namespaces**: lowercase (e.g., `sph`)
- **Macros**: UPPER_SNAKE_CASE (e.g., `REGISTER_SAMPLE`, `REGISTER_MODULE`, `DIM`)

### Files
- **Headers**: `.hpp` extension
- **Implementation**: `.cpp` extension
- **Configuration**: `.json` for parameter files
- **Headers/sources**: lowercase with underscores (e.g., `fluid_force.hpp`, `sample_registry.cpp`)

## Code Organization

### Header Structure
```cpp
#pragma once

namespace sph {
    // Forward declarations
    class Simulation;
    
    // Class definition
    class MyClass {
    public:
        // Public interface
        
    private:
        // Member variables with m_ prefix
    };
}
```

### Registration Pattern
```cpp
// Sample registration (in .cpp file)
namespace {
    void my_simulation(std::shared_ptr<sph::Simulation> sim,
                      std::shared_ptr<sph::SPHParameters> param) {
        // Initialization code
    }
    REGISTER_SAMPLE("my_sim_name", my_simulation);
}

// Module registration (in .cpp file)
REGISTER_MODULE("gsph", "fluid_force", GFluidForce);
```

## Compilation Directives

### Dimension Setting
The simulation dimension is set via `DIM` macro in `include/defines.hpp`:
```cpp
#define DIM 1  // or 2, 3
```
This affects vector types and must be set before compilation.

### Conditional Compilation
```cpp
#if DIM != 1
    THROW_ERROR("DIM != 1 for shock_tube");
#endif

#ifdef _OPENMP
    // OpenMP code
#endif
```

## Module Interface Pattern

All physics modules inherit from `Module` base class:
```cpp
class MyModule {
public:
    virtual void initialize(std::shared_ptr<SPHParameters> param,
                          std::shared_ptr<Simulation> sim) = 0;
    virtual void calculation(std::shared_ptr<SPHParameters> param,
                           std::shared_ptr<Simulation> sim) = 0;
};
```

## Memory Management
- Use `std::shared_ptr` for shared ownership (particles, simulation state)
- Use `std::unique_ptr` for exclusive ownership
- Avoid raw pointers except for performance-critical inner loops

## Parallelization
- OpenMP pragmas for parallel loops
- Thread count controlled via command line
- Use `#pragma omp parallel for` for particle loops

## Error Handling
- Use `THROW_ERROR()` macro for runtime errors
- Use `assert()` for debug-time invariants
- Use `exception_handler()` wrapper in main

## Logging
- `WRITE_LOG` for important messages
- `WRITE_LOG_ONLY` for verbose output (no console echo)
- Logger writes to file in output directory

## Comments
- Minimal inline comments (code should be self-documenting)
- Japanese comments exist in original code
- Focus on "why" not "what" in comments
