# Disk Simulations

Thin disk and quasi-3D simulations, particularly relevant for protoplanetary disks, accretion disks, and galactic disks.

## Subdirectories

### Thin Disk 3D (`thin_disk_3d/`)

Full 3D simulation of a thin disk.

**Simulations:**
- `thin_disk_3d.cpp` - 3D razor-thin disk

**Physical Setup:**
- Vertically thin disk (H/R << 1)
- Differential rotation (Keplerian or similar)
- Self-gravity optional
- Pressure support in vertical direction

**Key Tests:**
- Vertical hydrostatic equilibrium
- Rotation curve maintenance
- Long-term stability
- Gravitational instability (if enabled)

### Thin Slice (`thin_slice/`)

2.5D simulations using thin vertical slices.

**Simulations:**
- `thin_slice_poly_2_5d.cpp` - Polytropic thin slice
- `thin_slice_relax.cpp` - Relaxation to equilibrium
- `thin_slice_anisotropic_relax.cpp` - With anisotropic pressure

**2.5D Approach:**
- Particles in 2D (r, φ) but track z-velocity
- Vertical structure from polytropic relation
- Computationally cheaper than full 3D
- Good for studying radial/azimuthal dynamics

## Physical Parameters

### Disk Structure

| Parameter | Description | Typical Value |
|-----------|-------------|---------------|
| R_in | Inner radius | 0.1-1.0 (code units) |
| R_out | Outer radius | 10-100 (code units) |
| H/R | Aspect ratio | 0.03-0.1 |
| M_disk | Disk mass | 0.01-0.1 M_central |
| Q | Toomre Q parameter | 1.5-2.0 (stable) |

### Rotation

**Keplerian**: Ω² = GM/r³
**Modified**: Ω² = GM/r³ + pressure term

### Gravitational Stability

**Toomre Q parameter**: Q = cs Ω / (π G Σ)

- Q > 1: Stable to axisymmetric perturbations
- Q ~ 1: Marginally stable, prone to spiral arms
- Q < 1: Gravitationally unstable, fragments

## Common Configurations

### Stable Disk
```json
{
  "gamma": 1.4,
  "Q_target": 2.0,
  "H_over_R": 0.05,
  "useGravity": false
}
```

### Unstable Disk (Self-Gravity)
```json
{
  "gamma": 1.4,
  "Q_target": 1.2,
  "H_over_R": 0.05,
  "useGravity": true,
  "theta": 0.5
}
```

### Protoplanetary Disk
```json
{
  "gamma": 1.4,
  "H_over_R": 0.03,
  "alpha_viscosity": 0.001,
  "M_star": 1.0,  // Solar masses
  "M_disk": 0.01
}
```

## Resolution Requirements

Critical for thin disks:

**Radial Direction:**
- N_r ≥ 50 per decade in radius
- More near features (gaps, spirals)

**Azimuthal Direction:**
- N_φ ≥ 100-200 for full disk
- N_φ ≥ 50 for sector simulations

**Vertical Direction (3D):**
- N_z ≥ 10-20 in scale height
- Even for H/R = 0.05

**Total Particles:**
- **Minimum**: 10^4 (quick tests)
- **Standard**: 10^5 (reasonable)
- **High-res**: 10^6+ (publication quality)

## Validation Criteria

### Equilibrium Disks
- ✅ Rotation curve maintained (< 1% drift)
- ✅ Vertical scale height constant
- ✅ No radial migration (unless intended)
- ✅ Surface density profile preserved

### Self-Gravitating Disks
- ✅ Q parameter calculated correctly
- ✅ Spiral arms if Q ~ 1
- ✅ Fragmentation if Q < 1
- ✅ Angular momentum conservation

### Energy Conservation
For isolated disk (no accretion):
- ✅ Total energy constant (< 1% drift)
- ✅ Angular momentum conserved (< 0.1%)

## Initial Condition Setup

### Relaxation Approach
1. Start with approximate analytical profile
2. Let system relax (10-100 orbits)
3. Use relaxed state as IC for science run
4. Helps remove transients and artificial modes

### Surface Density Profiles

**Power Law**: Σ(r) = Σ₀ (r/r₀)^(-p)
- p = 0: Uniform surface density
- p = 1: Minimum mass solar nebula (MMSN)
- p = 1.5: Steep profile

**Tapered**: Σ(r) = Σ₀ (r/r₀)^(-p) exp(-r/r_t)
- Smooth cutoff at outer edge

## Boundary Conditions

### Inner Boundary
- **Sink particle**: Accrete material at r < r_in
- **Reflecting**: Hard wall (non-physical)
- **Open**: Particles can leave domain

### Outer Boundary
- **Wave-killing**: Damp perturbations
- **Open**: Allow outflow
- **Periodic** (for annuli): Not common for disks

## Vertical Structure (2.5D)

Approximate 3D effects in 2D:

**Scale Height**: H(r) = cs/Ω = (H/R) × r

**Column Density**: Σ = ρ × √(2π) H

**Effective Thickness**: Δz ~ 2-3 H

## Common Issues

### Issue: Disk disperses quickly
- **Cause**: Artificial viscosity too high
- **Fix**: Reduce avAlpha, use Balsara switch

### Issue: Spurious clumping
- **Cause**: Gravitational softening too small
- **Fix**: ε ≥ 2h, ensure Q > 1 for stable disk

### Issue: Non-Keplerian rotation
- **Cause**: Incorrect gravity or pressure setup
- **Fix**: Check force balance, verify rotation curve

### Issue: Vertical structure unstable
- **Cause**: Incorrect z-force or pressure
- **Fix**: Ensure polytropic equilibrium, check gamma

## Diagnostics

### Rotation Curve
Plot Ω(r) vs r, compare to expected Keplerian

### Toomre Q
Plot Q(r), identify unstable regions

### Surface Density
Track Σ(r,t), check for mass redistribution

### Spiral Arms
- Fourier analyze m=1,2,3,... modes
- Measure pattern speed

## Physics Extensions

Optional physics to include:

- [ ] **Radiative cooling**: Energy loss from disk surface
- [ ] **Heating**: Stellar irradiation, viscous dissipation
- [ ] **Magnetic fields**: MRI turbulence (MHD)
- [ ] **Planets**: Embedded planets, gap opening
- [ ] **Dust**: Two-fluid (gas+dust) or live dust particles

## Usage

```bash
# Thin disk 3D
./sph thin_disk_3d configs/disks/thin_disk_3d.json 8

# Relax initial conditions
./sph thin_slice_relax configs/disks/relax.json 8

# Run with relaxed IC
./sph thin_slice_poly_2_5d configs/disks/thin_slice.json 8
```

## Future Simulations

- [ ] Eccentric disks
- [ ] Warped disks  
- [ ] Planet-disk interaction
- [ ] Binary system disks
- [ ] Magnetized disk (MHD)

## References

- Toomre Q: Toomre (1964) - "On the gravitational stability of a disk of stars"
- Disk Structure: Pringle & King (2007) - "Astrophysical Flows"
- SPH Disks: Lodato & Price (2010) - "On the diffusive propagation of warps in thin accretion discs"
