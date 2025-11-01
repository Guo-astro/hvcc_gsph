# Gravity-Dominated Simulations

Simulations where self-gravity plays a dominant or significant role. These test the coupling between hydrodynamics and gravitational forces.

## Subdirectories

### Lane-Emden (`lane_emden/`)

Hydrostatic equilibrium of self-gravitating polytropic spheres.

**Simulations:**
- `lane_emden_1d.cpp` - 1D spherical Lane-Emden model
- `lane_emden_2d.cpp` - 2D axisymmetric Lane-Emden model

**Physical Setup:**
- Polytropic equation of state: P = K ρ^γ
- Hydrostatic equilibrium: ∇P = -ρ∇Φ
- Various polytropic indices (n = 1/(γ-1))

**Key Tests:**
- Hydrostatic equilibrium maintenance
- Density profile matches analytical solution
- Pressure balance
- Long-term stability

**Polytropic Indices:**
- n=1 (γ=2): Isothermal sphere approximation
- n=1.5 (γ=5/3): Fully convective star
- n=3 (γ=4/3): Radiation-dominated star

### Pairing Instability (`pairing_instability/`)

Simulations related to pair-instability supernovae dynamics.

**Tests:**
- Extreme conditions (high temperature, radiation pressure)
- Explosive dynamics
- Energy release mechanisms

### Hydrostatic Tests (`hydrostatic/`)

General hydrostatic equilibrium tests.

**Tests:**
- Pressure-supported structures
- Long-term stability
- Force accuracy in equilibrium

## Common Parameters

| Parameter | Typical Value | Notes |
|-----------|---------------|-------|
| `useGravity` | true | Must enable self-gravity |
| `G` | 1.0 or 6.67e-11 | Gravitational constant (code or SI units) |
| `theta` | 0.5-0.7 | Barnes-Hut opening angle |
| `leafParticleNumber` | 16-32 | Tree refinement |
| `gamma` | 4/3, 5/3, 2 | Polytropic index |

## Gravity Solver Options

### Tree-Based (Barnes-Hut)
- Fast: O(N log N)
- Approximate but accurate
- Controlled by `theta` parameter
- **Use for**: Large-N simulations (N > 1000)

### Direct Summation (Exhaustive)
- Exact: O(N²)
- Slow for large N
- **Use for**: Small-N tests, validation

## Validation Criteria

### Lane-Emden
- ✅ Density profile matches analytical solution (< 5% error)
- ✅ Hydrostatic equilibrium maintained (force errors < 1%)
- ✅ No radial motion (velocities near zero)
- ✅ Virial equilibrium: 2K + W ≈ 0

### General Gravity Tests
- ✅ Energy conservation (grav + kinetic + thermal)
- ✅ Angular momentum conservation (if rotating)
- ✅ Momentum conservation (for isolated systems)

## Usage

```bash
# Lane-Emden 1D (n=1.5)
./sph lane_emden_1d configs/gravity/lane_emden_n1p5.json 8

# Check hydrostatic equilibrium
uv run gsph-analyze hydrostatic results/lane_emden
```

## Resolution Requirements

Gravity simulations need sufficient resolution to:
- Resolve density gradient: Δr < H/10
- Resolve gravitational softening: ε ~ h (smoothing length)
- Avoid artificial clumping: N_neighbor ≥ 50 (3D)

Minimum particles:
- **1D Lane-Emden**: 100-200 particles
- **2D Lane-Emden**: 1,000-5,000 particles  
- **3D Collapse**: 10,000-100,000 particles

## Gravitational Softening

Softening length prevents singularities in gravity calculation:
- **Too small**: Numerical noise, artificial close encounters
- **Too large**: Over-smoothed gravity, incorrect collapse

**Recommended**: ε = 2.8h (matches SPH smoothing kernel)

## Units

### Code Units (G=1)
Convenient for dimensionless analysis:
```json
{
  "G": 1.0,
  "M_total": 1.0,
  "R_initial": 1.0
}
```

### Physical Units
For direct comparison with observations:
```json
{
  "G": 6.67430e-11,  // m³ kg⁻¹ s⁻²
  "M_solar": 1.989e30,  // kg
  "pc": 3.086e16  // m
}
```

## Common Issues

### Issue: Collapse doesn't stop
- **Cause**: No pressure support, eos incorrect
- **Fix**: Check gamma, ensure thermal pressure enabled

### Issue: Artificial expansion
- **Cause**: Too much artificial viscosity
- **Fix**: Reduce avAlpha, use Balsara switch

### Issue: Force errors in equilibrium
- **Cause**: theta too large, or particle disorder
- **Fix**: Reduce theta, relax initial conditions

## Energy Components

Track these in `energy.txt`:
- **Kinetic**: K = Σ(½mv²)
- **Thermal**: U = Σ(mu/(γ-1))
- **Gravitational**: W = ½ΣΣ(Gm_i m_j/r_ij)
- **Total**: E_total = K + U + W

**Virial Theorem**: For equilibrium, 2K + W = 0

## Future Simulations

- [ ] Rotating Lane-Emden spheres
- [ ] Binary star systems
- [ ] Gravitational collapse with cooling
- [ ] Jeans instability tests
- [ ] Tidal disruption events

## References

- Lane-Emden: Chandrasekhar (1939) - "An Introduction to the Study of Stellar Structure"
- SPH Gravity: Springel (2005) - GADGET-2
- Barnes-Hut: Barnes & Hut (1986) - "A hierarchical O(N log N) force-calculation algorithm"
