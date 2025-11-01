# DISPH Debug Analysis

## Problem
- Simulation runs 1 timestep successfully (t=0 → t=0.000114244)
- Then timesteps collapse to zero (dt_force=0, dt_sound=2.52e-79)
- Stuck in infinite loop at t=0.000114244

## Root Cause Hypothesis

After first timestep, something is corrupting particle state causing:
1. Extremely small sound speed → dt_sound near zero
2. Infinite acceleration → dt_force = 0

### Possible causes:
1. **Volume corruption**: V becoming zero or negative after update
2. **Density corruption**: ρ becoming zero causing division by zero in V=m/ρ
3. **Energy floor**: Many particles hitting energy floor (1e-10) → sound speed collapsing
4. **Missing gradh term**: Standard SPH has gradh correction, DISPH formulation might need it too
5. **Time integration order**: Volume must be updated BEFORE force calculation

## Analytical Comparison

### Standard SPH (WORKING):
```cpp
p_per_rho2 = P/ρ²
acc = -Σ m_j * (P_i/ρ_i² * gradh_i + P_j/ρ_j² * gradh_j) * ∇W
```

### DISPH (BROKEN):
```cpp
P_over_V2 = P/V² where V=m/ρ  
acc = -Σ m_j * (P_i/V_i² + P_j/V_j²) * ∇W
```

Note: P/V² = P/(m/ρ)² = Pρ²/m² which is different from P/ρ²

## Next Steps
1. Add debug output showing particle states before/after timestep
2. Check if volume is being recalculated in PreInteraction every step
3. Verify no particles have V→0 or ρ→0
4. Compare with paper's implementation details
5. Check if gradh term is needed in DISPH formulation
