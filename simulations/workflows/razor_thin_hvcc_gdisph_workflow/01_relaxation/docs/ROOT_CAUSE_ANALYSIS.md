# Root Cause Analysis: Disk Relaxation Numerical Collapse

## Investigation Date
2025-11-01

## Executive Summary

**ROOT CAUSE: Unit Conversion Error**

The `disk_relaxation.cpp` plugin initializes particle positions and masses in **astronomical units** (parsecs and solar masses) but the SPH code expects **SI units** (meters and kilograms). This creates a scale error of ~10^16-10^30, causing immediate numerical collapse.

---

## Evidence

### 1. Position Data Analysis

**Expected (from code comments):**
- R_fluid = 3.0 pc = 9.258 × 10^16 m
- z_max = 0.2 pc = 6.172 × 10^15 m

**Actual (from CSV output):**
- X range: [-2.94, 2.94] m
- Y range: [-2.94, 2.94] m  
- Z range: [-0.16, 0.16] m

**Error Factor:** 3.086 × 10^16 (exactly 1 parsec in meters)

### 2. Mass Data Analysis

**Expected:**
- Total mass: 1000 M☉
- Mass per particle (9880 particles): 2.013 × 10^29 kg

**Actual:**
- Mass per particle: 1.012 × 10^-1 kg
- Total mass: 0.00 M☉

**Error Factor:** 5.03 × 10^-31 (exactly M☉/kg ratio)

### 3. Consequences of Unit Error

| Property | Effect | Result |
|----------|--------|--------|
| Particle spacing | All particles within ~3m sphere | Effective point mass |
| Neighbor finding | Only 1 neighbor found per particle | Smoothing fails |
| Smoothing length | sml ≈ 0 (uninitialized) | Division by zero |
| Neighbor list | All 9880 particles overlap | List overflow (6400 max) |
| Gravitational force | F ∝ 1/r² with r→0 | Forces → ∞ |
| Density | Collapsed volume | 100× increase in 0.2s |
| Timestep | CFL with v→∞ | dt → 0 |

---

## Failure Cascade

1. **t = 0.0s:** Particles initialized at wrong scale (parsec values treated as meters)
2. **Neighbor search:** All 9880 particles appear to be at the same point
3. **Smoothing length:** Cannot be computed (all neighbors at r ≈ 0)
4. **WARNING:** "Neighbor list full" × 1000+ times (all particles overlapping)
5. **t = 0.2s:** Density spikes to 33.8 kg/m³ (112× increase)
6. **t > 0.2s:** Total collapse - all densities → 0, time value corrupts

---

## Code Analysis

### Current Implementation (WRONG)

```cpp
// disk_relaxation.cpp lines 54-60
const real R_fluid = 3.0;      // Fluid disk radius in x-y (parsecs) ❌
const real z_max = 0.2;        // Half-thickness in z (parsecs) ❌
const real M_total = 1000.0;   // Total fluid mass (solar masses) ❌

// Later, lines 130-135
pp.pos[0] = x;  // x is in parsecs, but code expects meters! ❌
pp.pos[1] = y;
pp.pos[2] = z;
pp.mass = mpp;  // mass is in M☉, but code expects kg! ❌
```

### Required Fix

**Add unit conversions using SPH codebase constants:**

```cpp
#include "utilities/unit_conversions.hpp"

// Use physical constants
using namespace PhysicalConstants;

// Define in astronomical units
const real R_fluid_pc = 3.0;          // parsecs
const real z_max_pc = 0.2;            // parsecs  
const real M_total_msun = 1000.0;     // solar masses

// Convert to SI units
const real R_fluid = R_fluid_pc * pc_SI;      // meters
const real z_max = z_max_pc * pc_SI;          // meters
const real M_total = M_total_msun * M_sun_SI; // kilograms

// Mass per particle is now in kg
const real mpp = M_total / static_cast<real>(fluid_count);
```

---

## Verification

### Before Fix (Actual Data)
- Particle positions: ±2.94 m (should be ±9.26×10^16 m)
- Mass per particle: 0.101 kg (should be 2.01×10^29 kg)
- Neighbor count: 1 (should be ~64)
- Smoothing length: 0 m (should be ~10^15 m)

### After Fix (Expected)
- Particle positions: ±9.26×10^16 m ✓
- Mass per particle: 2.01×10^29 kg ✓
- Neighbor count: ~64 ✓
- Smoothing length: ~10^15 m ✓

---

## Related Issues

1. **Density relaxation cannot work** - requires valid neighbor lists
2. **Lane-Emden profile distorted** - scale mismatch affects density calculation
3. **Gravity disabled effectively** - G=0.0043 is meaningless without proper units
4. **Time integration unstable** - CFL conditions violated from start

---

## Recommendations

### Immediate Actions
1. ✅ Add unit conversions to `disk_relaxation.cpp`
2. ✅ Verify using `PhysicalConstants` from `unit_conversions.hpp`
3. ✅ Test with single particle to verify units before full run
4. ✅ Add unit conversion tests to prevent regression

### Long-term Improvements
1. Create a unit-aware particle initialization helper
2. Add runtime unit validation (check if positions are in valid range)
3. Document expected units in SimulationPlugin API
4. Add unit system parameter to config.json

---

## References

- Physical constants: `/include/utilities/unit_conversions.hpp`
- Unit conversion examples: `/analysis/example_units.py`
- Python unit utilities: `/analysis/units.py`

---

## Conclusion

The numerical collapse was NOT due to:
- ❌ Neighbor number too low
- ❌ CFL conditions too aggressive
- ❌ DISPH scheme instability
- ❌ Density relaxation parameters

The collapse WAS due to:
- ✅ **Unit conversion error** - positions in parsecs treated as meters
- ✅ **Scale mismatch** - 16 orders of magnitude error
- ✅ **Immediate failure** - code could not function with wrong units

**Fix complexity:** LOW (add 3 lines of unit conversion)
**Impact:** HIGH (makes simulation functional)
**Priority:** CRITICAL (blocks entire workflow)
