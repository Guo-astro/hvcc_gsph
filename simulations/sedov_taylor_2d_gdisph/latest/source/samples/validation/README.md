# Validation Tests

Code validation tests that check specific SPH implementation details and edge cases.

## Purpose

These simulations test the correctness of the SPH implementation under extreme or special conditions, rather than comparing to analytical solutions.

## Subdirectories

### Vacuum Test (`vacuum_test/`)

Tests SPH behavior with vacuum boundaries or very low density regions.

**Physical Setup:**
- High density region adjacent to vacuum (ρ → 0)
- Tests particle behavior at density discontinuities
- No physical boundaries

**Key Tests:**
- Pressure calculation in low-density limit
- Kernel evaluation near vacuum
- Neighbor search in sparse regions
- No spurious forces from vacuum

**Expected Behavior:**
- Particles expand into vacuum
- No artificial forces from void
- Smooth density transition
- Energy conservation

### Cooling Test (`cooling_test/`)

Tests heating/cooling module implementation.

**Physical Setup:**
- Uniform or stratified medium
- Heating/cooling terms active
- May include radiative processes

**Key Tests:**
- Thermal energy evolution
- Cooling timescales
- Temperature floors/ceilings
- Energy conservation (with source terms)

**Expected Behavior:**
- Correct cooling/heating rates
- Thermal equilibrium (if expected)
- No negative temperatures
- Proper handling of cooling catastrophe

## Validation Criteria

### Vacuum Test
- ✅ No NaN or inf values
- ✅ Smooth density field (no discontinuities in calculation)
- ✅ Momentum conserved
- ✅ Energy conserved (accounting for expansion work)
- ✅ Particles don't "stick" at vacuum boundary

### Cooling Test
- ✅ Cooling rate matches analytical prescription
- ✅ Temperature stays within physical bounds
- ✅ Energy equation includes cooling term correctly
- ✅ No thermal runaway

## Common Parameters

### Vacuum Test
```json
{
  "SPHType": "gsph",
  "gamma": 1.4,
  "avAlpha": 1.0,
  "neighborNumber": 32,
  "periodic": false
}
```

### Cooling Test
```json
{
  "SPHType": "gsph",
  "gamma": 1.666667,
  "useHeatingCooling": true,
  "coolingFunction": "constant",  // or "power_law", "table"
  "coolingRate": 1.0e-3
}
```

## Debugging Tips

### Issue: NaN in density calculation
- **Cause**: Division by zero when ρ → 0
- **Fix**: Add density floor: ρ_min ~ 1e-10
- **Check**: Kernel sum normalization

### Issue: Particles clump at vacuum boundary
- **Cause**: Artificial pressure or surface tension
- **Fix**: Check AV formulation, pressure calculation

### Issue: Cooling too fast
- **Cause**: Timestep too large for cooling timescale
- **Fix**: Reduce dt or subcycle cooling

### Issue: Temperature goes negative
- **Cause**: Cooling rate exceeds thermal energy
- **Fix**: Limit cooling rate, enforce T_min

## Physical Considerations

### Vacuum Boundaries

**Not True Vacuum**: SPH cannot handle true ρ=0
- Use very low density: ρ_vac ~ 1e-6 ρ_0
- Particles still exist in "vacuum" regions
- Provides smooth kernel support

**Free Expansion**: No pressure support in vacuum
- Particles accelerate outward
- Expansion follows momentum conservation
- May develop instabilities at interface

### Heating/Cooling Physics

**Cooling Functions:**
- **Constant**: du/dt = -Λ
- **Power Law**: Λ ∝ ρ^α T^β
- **Tabulated**: Λ(ρ, T) from data

**Subcycling**: Cool on shorter timestep
```
dt_cool = min(dt_hydro, u / |du/dt|)
```

**Temperature Floors**:
- Minimum: T_min ~ 10 K (avoid cooling catastrophe)
- Maximum: T_max ~ 10^9 K (relativistic limit)

## Usage

```bash
# Vacuum test
./sph vacuum_test configs/validation/vacuum_test.json 8

# Cooling test
./sph cooling_test configs/validation/cooling_test.json 8

# Analyze thermal evolution
uv run gsph-analyze energy results/cooling_test
```

## Output Analysis

### Vacuum Test
- Plot density vs position: Check smooth transition
- Plot pressure vs position: No spikes at boundary
- Energy history: Should be conserved

### Cooling Test
- Plot temperature vs time: Check cooling timescale
- Plot thermal energy: Should decrease
- Compare to analytical cooling curve

## Edge Cases to Test

- [ ] Very low density (ρ < 1e-10)
- [ ] Very high density contrast (10^6:1)
- [ ] Rapid cooling (t_cool << t_dyn)
- [ ] Heating and cooling balance
- [ ] Zero pressure regions
- [ ] Supersonic expansion into vacuum

## Future Validation Tests

- [ ] Particle disorder test (glass vs. lattice IC)
- [ ] Kernel integral test (∑W = 1?)
- [ ] Gradient operator accuracy test
- [ ] Time step convergence test
- [ ] Resolution convergence test
- [ ] Galilean invariance test

## References

- Vacuum: Agertz+ (2007) - "Fundamental differences between SPH and grid methods"
- Cooling: Katz+ (1996) - "Cosmological Simulations with TreeSPH"
- SPH Basics: Monaghan (2005) - "Smoothed particle hydrodynamics" (review)
