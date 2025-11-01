# Answer: Do We Need "No Units" in Simulation?

## Short Answer

**NO** - Your codebase uses **physical SI units throughout the simulation**.

You're confusing two different approaches that are both valid in astrophysics:

## The Two Approaches

### Approach 1: Code Units (Dimensionless) ❌ Not what you're using

```
During simulation: Everything scaled to O(1)
  - G = 1.0
  - M_total = 1.0  
  - R = 1.0
  - No units during computation

After simulation: Add units back
  - time_physical = time_code × T_unit
  - mass_physical = mass_code × M_unit
```

**Used by:** GADGET, GIZMO, some research codes

**Why:** Numerical stability, no huge/tiny numbers

### Approach 2: Physical Units (SI) ✅ YOUR CODEBASE

```
During simulation: Real SI values
  - positions in meters (10^16 m)
  - masses in kilograms (10^30 kg)
  - G = 6.674e-11 m³ kg⁻¹ s⁻²
  
After simulation: Already physical
  - Output: "pos_x [m] = 9.26e16"
  - Can convert to any unit system
```

**Used by:** Your codebase, FLASH, many production codes

**Why:** Direct physical interpretation, no confusion

---

## Evidence Your Code Uses SI (Approach 2)

1. **Output headers have units:**
   ```
   time [s], pos_x [m], mass [kg], dens [kg/m^3]
   ```

2. **Unit conversion utilities exist:**
   ```cpp
   #include "utilities/unit_conversions.hpp"
   PhysicalConstants::pc_SI = 3.086e16  // meters
   PhysicalConstants::M_sun_SI = 1.988e30  // kg
   ```

3. **Documentation states:**
   > "The SPH code now includes a flexible unit conversion system that supports:
   > - **SI units** (m, kg, s)"

---

## Your Misconception

You thought:
> "During simulation use no units, after simulation add physical meaning"

But actually:
- ✅ During simulation: Use SI units (10^16 m, 10^30 kg)
- ✅ After simulation: Convert SI → whatever you want (pc, kpc, M☉)

---

## The Correct Workflow for YOUR Code

### Step 1: Initialize with SI Conversion

```cpp
// Define in astronomical units (human-readable)
const real R_fluid_pc = 3.0;       // parsecs
const real M_total_msun = 1000.0;  // solar masses

// Convert to SI for simulation
const real R_fluid = R_fluid_pc * PhysicalConstants::pc_SI;      // meters
const real M_total = M_total_msun * PhysicalConstants::M_sun_SI; // kg

// Assign to particles
pp.pos[0] = x * PhysicalConstants::pc_SI;  // x in pc → meters
pp.mass = mpp;  // already in kg
```

### Step 2: Simulate in SI

```
Simulation runs with:
- Positions: 10^16 to 10^17 m (parsec scale)
- Masses: 10^29 to 10^30 kg (solar mass scale)
- Forces computed in SI
- Integration in SI
```

### Step 3: Output is SI

```csv
time [s], pos_x [m], pos_y [m], mass [kg], dens [kg/m^3]
0.0, 9.26e16, 1.85e16, 2.01e29, 3.01e-01
```

### Step 4: Analysis Converts to Any Units

```python
import pandas as pd
from analysis.units import UnitFactory

df = pd.read_csv("output.csv")

# Convert SI → parsecs
pc = 3.086e16
x_pc = df['pos_x [m]'] / pc

# Convert SI → kpc  
kpc = 3.086e19
x_kpc = df['pos_x [m]'] / kpc

# Convert SI → solar masses
M_sun = 1.988e30
mass_msun = df['mass [kg]'] / M_sun
```

---

## Why Your Code Failed

You did NOT convert units during initialization:

```cpp
// ❌ WRONG - No conversion
const real R_fluid = 3.0;  // Comment says "parsecs"
pp.pos[0] = x;  // x = 3.0 assigned directly

// Code interprets as:
// pos_x [m] = 3.0 meters  (WRONG! Should be 9.26e16 m)
```

Should have been:

```cpp
// ✅ CORRECT - With conversion
const real R_fluid = 3.0 * PhysicalConstants::pc_SI;  // parsecs → meters
pp.pos[0] = x * PhysicalConstants::pc_SI;  // x in pc → meters

// Code interprets as:
// pos_x [m] = 9.26e16 meters  (CORRECT!)
```

---

## Summary Table

| Stage | Code Units Approach | SI Units Approach (YOURS) |
|-------|---------------------|---------------------------|
| **Initialization** | Scale to O(1) | Convert astronomical → SI |
| **Simulation** | Dimensionless | SI values (huge numbers) |
| **Output** | Dimensionless | SI with unit labels |
| **Analysis** | Scale back to physical | Convert SI → any units |
| **Your error** | N/A | Forgot to convert to SI! |

---

## The Fix

**Just add unit conversions during initialization. That's it.**

The simulation will run in SI, output in SI, and you can convert to any units you want during analysis using the built-in `units.py` module.

---

## Bottom Line

**You do NOT need to use "no units" in your simulation.**

Your codebase is DESIGNED to use SI units throughout. The flexibility to convert to other unit systems comes in the **analysis phase**, not by making the simulation dimensionless.

The root cause was simply: **forgot to multiply by conversion factors during initialization**.

Fix: 3 lines of code to convert parsecs→meters and M☉→kg.

