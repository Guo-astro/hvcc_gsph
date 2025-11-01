# Architecture Refactoring Recommendations

## Current Strengths ✅
1. **Plugin Architecture**: REGISTER_SAMPLE and REGISTER_MODULE macros enable easy extension
2. **Separation of Concerns**: Clear separation between simulation state, solver logic, and physics modules
3. **Factory Pattern**: ModuleFactory and SampleRegistry provide loose coupling
4. **CMake Automation**: Glob patterns automatically include new .cpp files

## Current Pain Points ❌

### 1. **Manual JSON Config Creation**
Each sample requires manually creating JSON files with all parameters.

### 2. **No Simulation Discovery**
No way to list available samples without reading source code or Makefile.

### 3. **Dimension Recompilation**
Changing DIM requires full recompilation - can't run 1D, 2D, 3D in same binary.

### 4. **Scattered Documentation**
Sample parameters spread across JSON files, README, and source comments.

### 5. **No Parameter Validation**
Easy to misconfigure simulations with invalid parameter combinations.

### 6. **Manual Makefile Targets**
Each new sample requires updating Makefile with run target.

### 7. **Limited Extensibility Guidance**
No clear template or guide for adding new modules/samples.

## Proposed Improvements 🚀

### Improvement 1: Self-Documenting Samples
**Goal**: Each sample documents its own parameters and purpose

**Implementation**:
```cpp
struct SampleMetadata {
    std::string name;
    std::string description;
    int required_dim;
    std::vector<std::string> tags;  // "benchmark", "production", "test"
    std::map<std::string, std::string> param_docs;
};

#define REGISTER_SAMPLE_EX(NAME, FUNC, META) \\
    REGISTER_SAMPLE(NAME, FUNC); \\
    static const SampleMetadata s_meta_##FUNC = META;
```

**Benefits**:
- `./sph --list-samples` shows all available simulations
- `./sph --info shock_tube` shows parameter documentation
- Auto-generate README sections from metadata

### Improvement 2: Parameter Builders
**Goal**: Type-safe, validated parameter construction

**Implementation**:
```cpp
class ParameterBuilder {
public:
    ParameterBuilder& set_time(real start, real end, real output);
    ParameterBuilder& set_sph_type(SPHType type);
    ParameterBuilder& set_kernel(KernelType kernel);
    ParameterBuilder& enable_gravity(real G, real theta);
    std::shared_ptr<SPHParameters> build();  // Validates on build
};

// In sample code:
void load_shock_tube(sim, param) {
    *param = ParameterBuilder()
        .set_time(0, 0.2, 0.002)
        .set_sph_type(SPHType::GSPH)
        .set_physics(1.4)  // gamma
        .build();
    // ... particle setup
}
```

**Benefits**:
- Compile-time parameter name checking
- Runtime validation before simulation starts
- Default values clearly defined
- Easier to discover available options

### Improvement 3: Template-Based Samples
**Goal**: Reduce boilerplate for common simulation patterns

**Implementation**:
```cpp
// Base templates
class ShockTubeSample : public SampleTemplate {
protected:
    virtual void set_left_state(real& dens, real& pres, real& vel) = 0;
    virtual void set_right_state(real& dens, real& pres, real& vel) = 0;
public:
    void initialize(sim, param) final override;  // Common logic
};

// Specific instances
class SodShockTube : public ShockTubeSample {
    void set_left_state(...) override { dens=1.0; pres=1.0; vel=0; }
    void set_right_state(...) override { dens=0.25; pres=0.1795; vel=0; }
};
REGISTER_SAMPLE("sod", SodShockTube);
```

**Benefits**:
- Less code duplication
- Consistent implementation patterns
- Easy to create variants of existing tests

### Improvement 4: Module Capabilities System
**Goal**: Modules declare what they provide/require

**Implementation**:
```cpp
class ModuleCapabilities {
    std::set<std::string> provides;  // "density", "pressure_forces", "gravity"
    std::set<std::string> requires;  // "neighbor_list", "tree"
};

class Module {
public:
    virtual ModuleCapabilities capabilities() const = 0;
    // ... rest of interface
};

// In ModuleFactory
void validate_module_graph(SPHParameters param) {
    // Check all required capabilities are provided
    // Detect circular dependencies
    // Warn about unused modules
}
```

**Benefits**:
- Catch configuration errors at initialization
- Document module interdependencies
- Enable dynamic module loading/unloading

### Improvement 5: Simulation Wizard
**Goal**: Interactive setup for new simulations

**Implementation**:
```bash
./sph --wizard
> Simulation name: my_rayleigh_taylor
> Dimension (1/2/3): 2
> SPH method (ssph/disph/gsph): gsph
> Include gravity? (y/n): y
> Include heating/cooling? (y/n): n
> Output directory: ./results/rayleigh_taylor

Generated:
  - src/sample/my_rayleigh_taylor.cpp (template)
  - sample/my_rayleigh_taylor/my_rayleigh_taylor.json
  - Makefile target: run_my_rayleigh_taylor
```

**Benefits**:
- Lower barrier to entry for new simulations
- Consistent structure across simulations
- Auto-generated boilerplate

### Improvement 6: Directory-Based Organization
**Goal**: Better organize samples by category

**Current**:
```
src/sample/
  shock_tube.cpp
  evrard.cpp
  khi.cpp
  ... (21 files)
```

**Proposed**:
```
src/sample/
  hydrodynamics/
    shock_tube.cpp
    sedov_taylor.cpp
    shock_tube_variants/
      strong_shock.cpp
      heating_cooling.cpp
  instabilities/
    khi.cpp
    rayleigh_taylor.cpp
  gravity/
    evrard.cpp
    lane_emden.cpp
  disks/
    thin_disk_3d.cpp
    thin_slice_poly_2_5d.cpp
```

**Benefits**:
- Easier to find related tests
- Clear organization by physics
- Subdirectory CMakeLists for modularity

### Improvement 7: Configuration Inheritance
**Goal**: Reduce JSON duplication via base configs

**Implementation**:
```json
// base_configs/gsph_2d.json
{
  "SPHType": "gsph",
  "cflSound": 0.3,
  "use2ndOrderGSPH": true,
  "kernel": "wendland"
}

// sample/my_sim/my_sim.json
{
  "extends": "../../base_configs/gsph_2d.json",
  "endTime": 1.0,
  "gamma": 1.4
  // Other params inherited
}
```

**Benefits**:
- DRY principle for configurations
- Easy to change defaults globally
- Clear indication of non-standard settings

### Improvement 8: Runtime Dimension Selection
**Goal**: Support multiple dimensions in one binary

**Challenge**: Template-heavy code with DIM as compile-time constant

**Partial Solution**:
```cpp
// Keep DIM for performance-critical inner loops
// Add runtime dispatch at Solver level
class SolverFactory {
    static std::unique_ptr<SolverBase> create(int dim, ...);
};

// Separate binaries still recommended for production
// But useful for testing/development
```

**Benefits**:
- Faster iteration during development
- Easier to package/distribute
- Trade-off: Small performance cost from virtual dispatch

## Implementation Priority 🎯

**Phase 1** (Quick Wins):
1. Self-documenting samples with `--list-samples`, `--info`
2. Directory-based organization
3. Configuration inheritance

**Phase 2** (Medium Effort):
4. Parameter builders for type safety
5. Module capabilities validation
6. Template-based sample patterns

**Phase 3** (Long Term):
7. Simulation wizard
8. Runtime dimension selection (optional)

## Migration Strategy 📋

1. **Incremental**: Old REGISTER_SAMPLE still works
2. **New is Optional**: REGISTER_SAMPLE_EX is opt-in enhancement
3. **Gradual Conversion**: Convert samples one-by-one to new patterns
4. **Backward Compatible**: Existing JSON configs continue working
5. **Documentation First**: Update guides before changing APIs
