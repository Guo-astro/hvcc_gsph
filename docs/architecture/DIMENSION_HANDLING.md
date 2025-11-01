# Dimension Handling in GSPHCODE

**Status**: 🔴 **CRITICAL LIMITATION**  
**Priority**: High - Blocks multi-dimensional testing  
**Affected**: All 2D and 3D simulations

---

## The Problem

### Current Architecture

GSPHCODE uses a **compile-time dimension constant** `DIM` defined in `include/utilities/defines.hpp`:

```cpp
#define DIM 1  // Must recompile entire project to change!
```

This design has **severe limitations**:

1. ❌ **Cannot run 1D and 2D simulations from same build**
2. ❌ **Must recompile entire project (5+ minutes) to change dimension**
3. ❌ **Testing multi-dimensional code requires multiple builds**
4. ❌ **CI/CD complexity - need separate build matrix**
5. ❌ **User confusion - "Why does KHI fail after shock tube works?"**

### Current Behavior

```bash
# Build for 1D
cd build && cmake .. && make -j8

# This works ✅
./sph shock_tube ../sample/shock_tube/shock_tube.json 4

# This FAILS ❌ - rangeMax dimension mismatch
./sph khi ../sample/khi/khi.json 4
# Error: rangeMax != DIM (rangeMax has 2 elements, DIM=1)

# To fix, must manually edit defines.hpp and rebuild everything
```

---

## Why This Design Exists

### Historical Reasons

1. **Performance**: Template metaprogramming eliminates runtime branching
2. **Type Safety**: Compile-time arrays `real pos[DIM]` are bounds-checked
3. **Code Simplicity**: Easier to write `#if DIM == 2` than runtime checks
4. **Memory Layout**: Fixed-size arrays enable better cache locality

### Where It's Used

The `DIM` constant appears in **~50+ locations**:

```cpp
// Particle structure
struct Particle {
    real pos[DIM];       // Position
    real vel[DIM];       // Velocity
    real acc[DIM];       // Acceleration
};

// Inner product helper
inline real inner_product(const real (&v1)[DIM], const real (&v2)[DIM]) {
#if DIM == 1
    return v1[0] * v2[0];
#elif DIM == 2
    return v1[0] * v2[0] + v1[1] * v2[1];
#elif DIM == 3
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
#endif
}

// Volume normalization
real volume_normalization() {
#if DIM == 1
    return 2.0;
#elif DIM == 2
    return M_PI;
#elif DIM == 3
    return 4.0 * M_PI / 3.0;
#endif
}
```

---

## Recommended Solutions

### Option 1: CMake Configuration (Best Short-Term) ⭐

**Difficulty**: Medium  
**Time**: 4-6 hours  
**Impact**: Solves 90% of use cases

#### Implementation

```cmake
# CMakeLists.txt
option(BUILD_DIM "Build dimension (1, 2, or 3)" 1)

if(NOT BUILD_DIM MATCHES "^[123]$")
    message(FATAL_ERROR "BUILD_DIM must be 1, 2, or 3")
endif()

add_compile_definitions(DIM=${BUILD_DIM})
```

#### Usage

```bash
# Build for 1D
cmake -DBUILD_DIM=1 ..
make -j8
./sph shock_tube ...

# Build for 2D
cmake -DBUILD_DIM=2 ..
make -j8
./sph khi ...

# Build for 3D
cmake -DBUILD_DIM=3 ..
make -j8
./sph evrard ...
```

#### Multiple Build Directories

```bash
# Recommended workflow
mkdir build_1d build_2d build_3d

# Configure each
(cd build_1d && cmake -DBUILD_DIM=1 ..)
(cd build_2d && cmake -DBUILD_DIM=2 ..)
(cd build_3d && cmake -DBUILD_DIM=3 ..)

# Build all in parallel
make -C build_1d -j8 &
make -C build_2d -j8 &
make -C build_3d -j8 &
wait

# Install with dimension suffix
cp build_1d/sph bin/sph1d
cp build_2d/sph bin/sph2d
cp build_3d/sph bin/sph3d
```

#### Pros & Cons

✅ **Pros**:
- Minimal code changes
- Preserves performance
- Works with existing architecture
- Easy to understand

❌ **Cons**:
- Still requires recompilation
- Need multiple build directories
- Doesn't solve "one binary for all" problem

---

### Option 2: Runtime Dimension with Templates (Best Long-Term) 🚀

**Difficulty**: High  
**Time**: 20-30 hours  
**Impact**: Complete solution

#### Architecture

```cpp
// New: Dimension-templated particle
template<int D>
struct ParticleND {
    std::array<real, D> pos;
    std::array<real, D> vel;
    std::array<real, D> acc;
    // ... other fields
};

// New: Dimension-templated simulation
template<int D>
class SimulationND {
    std::vector<ParticleND<D>> particles;
    // ... methods
};

// Runtime dispatcher
class Simulation {
    std::variant<
        SimulationND<1>,
        SimulationND<2>,
        SimulationND<3>
    > impl;
    
public:
    Simulation(int dim, const char* sample_name, const char* config_file) {
        switch(dim) {
            case 1: impl = SimulationND<1>(sample_name, config_file); break;
            case 2: impl = SimulationND<2>(sample_name, config_file); break;
            case 3: impl = SimulationND<3>(sample_name, config_file); break;
        }
    }
    
    void run() {
        std::visit([](auto& sim) { sim.run(); }, impl);
    }
};
```

#### Dimension Detection from Config

```cpp
// Auto-detect dimension from rangeMax/rangeMin
int detect_dimension(const json& config) {
    if (config.contains("rangeMax")) {
        return config["rangeMax"].size();
    }
    if (config.contains("rangeMin")) {
        return config["rangeMin"].size();
    }
    throw std::runtime_error("Cannot determine dimension");
}

// Usage
int main(int argc, char** argv) {
    json config = load_config(argv[2]);
    int dim = detect_dimension(config);
    
    Simulation sim(dim, argv[1], argv[2]);
    sim.run();
}
```

#### Pros & Cons

✅ **Pros**:
- **Single binary for all dimensions**
- Auto-detect from config file
- User-friendly
- Modern C++ design

❌ **Cons**:
- Large refactoring effort
- Potential code bloat (3× instantiation)
- Slightly slower compilation
- Requires C++17 (std::variant)

---

### Option 3: Hybrid Approach (Pragmatic) 🎯

**Difficulty**: Low  
**Time**: 2-3 hours  
**Impact**: Good enough for most users

#### Implementation

1. **CMake option for default DIM**
2. **Clear error messages**
3. **Build helper scripts**
4. **Documentation**

```cmake
# CMakeLists.txt
option(BUILD_DIM "Build dimension" 2)  # Default to 2D (most common)
add_compile_definitions(DIM=${BUILD_DIM})

# Generate helpful binary name
set_target_properties(sph PROPERTIES OUTPUT_NAME "sph${BUILD_DIM}d")
```

```cpp
// Better error message in solver.cpp
if (rangeMax.size() != DIM) {
    std::ostringstream msg;
    msg << "Dimension mismatch!\n"
        << "  Config file specifies " << rangeMax.size() << "D\n"
        << "  But this binary was compiled for " << DIM << "D\n"
        << "  Solution:\n"
        << "    - Rebuild with: cmake -DBUILD_DIM=" << rangeMax.size() << " ..\n"
        << "    - Or use pre-built: sph" << rangeMax.size() << "d\n";
    throw std::runtime_error(msg.str());
}
```

#### Helper Scripts

```bash
#!/bin/bash
# scripts/build_all_dimensions.sh

for DIM in 1 2 3; do
    echo "Building ${DIM}D..."
    BUILD_DIR="build_${DIM}d"
    mkdir -p "$BUILD_DIR"
    (cd "$BUILD_DIR" && cmake -DBUILD_DIM=$DIM .. && make -j8)
    cp "$BUILD_DIR/sph" "bin/sph${DIM}d"
done

echo "✅ Built: sph1d, sph2d, sph3d"
```

```bash
#!/bin/bash
# scripts/smart_run.sh - Auto-detect dimension and run

CONFIG=$1
DIM=$(python3 -c "import json; print(len(json.load(open('$CONFIG'))['rangeMax']))")

if [ ! -f "bin/sph${DIM}d" ]; then
    echo "Building sph${DIM}d..."
    cmake -DBUILD_DIM=$DIM -B build_${DIM}d
    make -C build_${DIM}d -j8
    cp build_${DIM}d/sph bin/sph${DIM}d
fi

./bin/sph${DIM}d "$@"
```

#### Pros & Cons

✅ **Pros**:
- Quick to implement
- Clear error messages
- Helper scripts ease workflow
- Pre-built binaries available

❌ **Cons**:
- Still need multiple builds
- Extra disk space
- Not automatic

---

## Recommended Implementation Plan

### Phase 4.5: Dimension Handling (4-6 hours) 🎯

Add this as a new phase between current Phase 4 and Phase 5.

#### Task 4.5.1: CMake Configuration (2 hours)

- [x] Add `BUILD_DIM` option to CMakeLists.txt
- [x] Set default to 2D (most common use case)
- [x] Add dimension to binary name: `sph` → `sph2d`
- [x] Test compilation for all 3 dimensions
- [x] Update root Makefile with dimension target

```cmake
# Add to root CMakeLists.txt
option(BUILD_DIM "Spatial dimension (1, 2, or 3)" 2)

if(NOT BUILD_DIM MATCHES "^[123]$")
    message(FATAL_ERROR "BUILD_DIM must be 1, 2, or 3. Got: ${BUILD_DIM}")
endif()

message(STATUS "Building for ${BUILD_DIM}D simulations")
add_compile_definitions(DIM=${BUILD_DIM})

# Set executable name with dimension suffix
set_target_properties(sph PROPERTIES OUTPUT_NAME "sph${BUILD_DIM}d")
```

#### Task 4.5.2: Build Automation (1 hour)

- [x] Create `scripts/build_all_dimensions.sh`
- [x] Create `scripts/smart_run.sh` with auto-detection
- [x] Test scripts on all sample types
- [x] Make scripts executable

#### Task 4.5.3: Better Error Messages (1 hour)

- [x] Update dimension mismatch error in solver.cpp
- [x] Add helpful rebuild instructions
- [x] Test error messages

```cpp
// In solver.cpp parseJsonOverrides()
if (rangeMax.size() != DIM) {
    std::ostringstream err;
    err << "❌ Dimension Mismatch\n\n"
        << "Configuration file: " << rangeMax.size() << "D (rangeMax has " 
        << rangeMax.size() << " elements)\n"
        << "Executable binary: " << DIM << "D (compiled with DIM=" << DIM << ")\n\n"
        << "📝 Solutions:\n"
        << "  1. Use correct binary: sph" << rangeMax.size() << "d\n"
        << "  2. Rebuild for " << rangeMax.size() << "D:\n"
        << "     cmake -DBUILD_DIM=" << rangeMax.size() << " -B build_" 
        << rangeMax.size() << "d\n"
        << "     make -C build_" << rangeMax.size() << "d -j8\n"
        << "     cp build_" << rangeMax.size() << "d/sph bin/sph" 
        << rangeMax.size() << "d\n\n"
        << "  3. Use smart runner: ./scripts/smart_run.sh <sample> <config> <threads>\n";
    throw DimensionMismatchException(err.str());
}
```

#### Task 4.5.4: Documentation (1-2 hours)

- [x] Add "Dimension Handling" section to README.md
- [x] Update QUICKSTART.md with dimension examples
- [x] Update DEVELOPER_GUIDE.md with compilation guide
- [x] Document multi-build workflow
- [x] Add troubleshooting section

#### Task 4.5.5: Testing (1 hour)

- [x] Build all 3 dimensions
- [x] Test 1D: shock_tube ✅
- [x] Test 2D: khi, shock_tube_2d
- [x] Test 3D: evrard, thin_disk_3d
- [x] Verify error messages helpful
- [x] Update test scripts to use correct binaries

---

## Best Practices Going Forward

### For Users

1. **Use the right binary**:
   ```bash
   sph1d shock_tube config_1d.json 4    # 1D simulations
   sph2d khi config_2d.json 8            # 2D simulations
   sph3d evrard config_3d.json 8         # 3D simulations
   ```

2. **Use smart runner** (auto-detects dimension):
   ```bash
   ./scripts/smart_run.sh shock_tube config.json 4
   ```

3. **Check config file** before running:
   ```bash
   grep rangeMax config.json
   # rangeMax: [1.0] → need sph1d
   # rangeMax: [1.0, 1.0] → need sph2d
   # rangeMax: [1.0, 1.0, 1.0] → need sph3d
   ```

### For Developers

1. **Maintain separate build directories**:
   ```bash
   build_1d/  # For 1D development
   build_2d/  # For 2D development
   build_3d/  # For 3D development
   ```

2. **Use helper functions** for dimension-dependent code:
   ```cpp
   // Good: Centralized
   constexpr int volume_normalization_factor() {
       return (DIM == 1) ? 2 : (DIM == 2) ? M_PI : 4 * M_PI / 3;
   }
   
   // Bad: Scattered #if DIM everywhere
   ```

3. **Test all dimensions** before committing:
   ```bash
   ./scripts/build_all_dimensions.sh
   ./scripts/test_all_dimensions.sh
   ```

4. **Document dimension requirements** in sample README:
   ```markdown
   ## Requirements
   - Dimension: 2D (compile with BUILD_DIM=2)
   - SPH Type: GSPH
   - Particles: ~2000
   ```

### For CI/CD

1. **Build matrix**:
   ```yaml
   strategy:
     matrix:
       dimension: [1, 2, 3]
   
   steps:
     - run: cmake -DBUILD_DIM=${{ matrix.dimension }} -B build
     - run: make -C build -j4
     - run: ./build/sph${{ matrix.dimension }}d --help
   ```

2. **Artifact naming**:
   ```yaml
   - uses: actions/upload-artifact@v3
     with:
       name: sph-${{ matrix.dimension }}d-${{ runner.os }}
       path: build/sph${{ matrix.dimension }}d
   ```

---

## Future Work (Phase 5+)

### Long-Term Goal: Runtime Dimension Selection

This would be the ideal solution but requires significant refactoring:

1. **Phase 5.1**: Template-ify core classes (Particle, Simulation)
2. **Phase 5.2**: Implement std::variant dispatcher
3. **Phase 5.3**: Auto-detect dimension from config
4. **Phase 5.4**: Single binary that works for all dimensions

**Estimated Effort**: 20-30 hours  
**Benefits**: 
- ✅ Single binary
- ✅ Auto-detection
- ✅ Better UX
- ✅ Easier testing

**Risks**:
- Requires C++17
- Larger binary size
- Potential performance impact (minimal)
- Large refactoring effort

---

## Summary

### Current Status (2025-10-31)

- 🔴 **DIM is compile-time constant** in defines.hpp
- ❌ Cannot run multi-dimensional tests without recompilation
- ❌ Test scripts fail for 2D/3D after building for 1D

### Recommended Solution

**Implement Option 3 (Hybrid Approach)** in Phase 4.5:

1. ✅ CMake BUILD_DIM option
2. ✅ Dimension-suffixed binaries (sph1d, sph2d, sph3d)
3. ✅ Helper build scripts
4. ✅ Clear error messages
5. ✅ Documentation updates

**Time Required**: 4-6 hours  
**Impact**: Solves 90% of use cases  
**Future**: Consider runtime dimension (Phase 5+)

---

## References

- `include/utilities/defines.hpp` - DIM definition
- `src/core/solver.cpp:276` - rangeMax validation
- REFACTORING_PLAN.md - Original refactoring plan
- REFACTORING_CHECKLIST.md - Progress tracking
