# Hydrodynamic Instability Simulations

Simulations of fluid instabilities that test the code's ability to capture complex hydrodynamic phenomena.

## Subdirectories

### Kelvin-Helmholtz Instability (`kelvin_helmholtz/`)

Shear flow instability between two fluid layers with different velocities.

**Simulations:**
- `khi_2d.cpp` - Classic 2D KHI setup

**Physical Setup:**
- Two layers with velocity shear
- Density contrast (typically 2:1)
- Small perturbation to seed instability
- Periodic boundaries

**Key Tests:**
- Instability growth rate
- Vortex roll-up formation
- Mixing of fluids
- Conservation during turbulent evolution
- Resolution dependence

**Expected Behavior:**
- Initial perturbation grows exponentially
- Formation of characteristic vortex rolls
- Eventually turbulent mixing
- Energy transfer from kinetic to internal

### Perturbation Tests (`perturbation_tests/`)

Tests of perturbation damping and wave propagation.

**Simulations:**
- `perturbation_damping.cpp` - Damping of small amplitude waves

**Tests:**
- Sound wave propagation
- Damping rates
- Phase velocity accuracy
- Dispersion relation

## Common Parameters

| Parameter | Typical Value | Notes |
|-----------|---------------|-------|
| `neighborNumber` | 32-50 | Higher for better mixing resolution |
| `useBalsaraSwitch` | true | Reduces AV in vortical flows |
| `useTimeDependentAV` | false | Can test with/without |
| `gamma` | 1.4-1.67 | Depends on equation of state |

## Validation Criteria

### KHI
- ✅ Vortex roll-up at expected time
- ✅ Growth rate matches linear theory (early times)
- ✅ Energy conservation within 1%
- ✅ No excessive diffusion (vortices persist)

### Perturbation Tests
- ✅ Correct wave speed
- ✅ Expected damping rate
- ✅ No spurious reflections

## Resolution Requirements

Instabilities require high resolution:
- **KHI**: Minimum 100 particles across shear layer
- **Better**: 200+ particles across shear layer
- **Production**: 400+ particles across shear layer

## Usage

```bash
# 2D KHI
./sph khi_2d configs/benchmarks/khi.json 8

# Analyze vorticity and mixing
uv run gsph-analyze khi results/khi --quantity vorticity
```

## Visualization Recommendations

- **Density**: Shows interface evolution
- **Vorticity**: Highlights vortex structure
- **Velocity field**: Vector plots show flow structure
- **Tracer particles**: Visualize mixing

## Common Issues

### Issue: Instability doesn't grow
- **Cause**: Perturbation too small or resolution too low
- **Fix**: Increase perturbation amplitude or resolution

### Issue: Excessive diffusion
- **Cause**: Artificial viscosity too high
- **Fix**: Use Balsara switch, reduce avAlpha

### Issue: Spurious vorticity
- **Cause**: Particle disorder or AV
- **Fix**: Ensure initial conditions are in equilibrium

## Future Tests

- [ ] Rayleigh-Taylor instability
- [ ] Richtmyer-Meshkov instability  
- [ ] Turbulence cascade tests
- [ ] Stratified shear flows

## References

- Kelvin-Helmholtz: McNally+ (2012) - SPH KHI tests
- Price (2008) - "Modelling discontinuities and Kelvin-Helmholtz instabilities in SPH"
