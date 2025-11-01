# Unit Conversion Redesign: Flexible User-Defined Units

## Design Philosophy

**3-Layer Unit System:**
1. **Input Layer**: User chooses units (pc, M☉, Myr) - human-friendly
2. **Simulation Layer**: Always dimensionless (R=1, M=1, G=1) - numerically stable
3. **Output Layer**: User chooses units (can be different from input!) - flexible analysis

**Key Innovation**: Input and output units are completely independent!

## Example Use Cases

```
User A: Input in [pc, M☉, Myr] → Simulate → Output in [pc, M☉, Myr]
User B: Input in [pc, M☉, Myr] → Simulate → Output in [AU, M_earth, years]  
User C: Input in [kpc, M☉, Gyr] → Simulate → Output in [SI units]
```

## Architecture

### 1. Unit System Definition (leverages existing UnitFactory)

```cpp
struct CodeUnits {
    // Characteristic scales (define the unit system)
    real length_unit;    // R_disk in SI (meters)
    real mass_unit;      // M_disk in SI (kg)
    real time_unit;      // Derived: sqrt(R³/GM) in SI (seconds)
    real velocity_unit;  // Derived: R/T in SI (m/s)
    real density_unit;   // Derived: M/R³ in SI (kg/m³)
    
    // Dimensionless parameters for simulation
    real G_code;         // Usually 1.0 for convenience
    
    // Initialize from physical parameters
    CodeUnits(real R_phys, real M_phys, real G_phys) {
        using namespace PhysicalConstants;
        
        length_unit = R_phys;
        mass_unit = M_phys;
        time_unit = std::sqrt(R_phys * R_phys * R_phys / (G_phys * M_phys));
        velocity_unit = R_phys / time_unit;
        density_unit = M_phys / (R_phys * R_phys * R_phys);
        
        // In code units, we typically set G = 1
        G_code = 1.0;
    }
    
    // Convert from physical to code units
    real to_code_length(real L_phys) const { return L_phys / length_unit; }
    real to_code_mass(real M_phys) const { return M_phys / mass_unit; }
    real to_code_time(real t_phys) const { return t_phys / time_unit; }
    real to_code_velocity(real v_phys) const { return v_phys / velocity_unit; }
    real to_code_density(real rho_phys) const { return rho_phys / density_unit; }
    
    // Convert from code to physical units
    real to_phys_length(real L_code) const { return L_code * length_unit; }
    real to_phys_mass(real M_code) const { return M_code * mass_unit; }
    real to_phys_time(real t_code) const { return t_code * time_unit; }
    real to_phys_velocity(real v_code) const { return v_code * velocity_unit; }
    real to_phys_density(real rho_code) const { return rho_code * density_unit; }
    
    void print() const {
        std::cout << "Code Unit System:\n";
        std::cout << "  Length unit: " << length_unit << " m\n";
        std::cout << "  Mass unit:   " << mass_unit << " kg\n";
        std::cout << "  Time unit:   " << time_unit << " s\n";
        std::cout << "  Velocity unit: " << velocity_unit << " m/s\n";
        std::cout << "  Density unit:  " << density_unit << " kg/m³\n";
        std::cout << "  G (code):    " << G_code << " (dimensionless)\n";
    }
};
```

### Proposed Structure

```cpp
struct DiskInitializationParams {
    // Input: Astronomical units (what user specifies)
    struct Input {
        real radius_pc;           // Disk radius (parsecs)
        real half_thickness_pc;   // Half-thickness (parsecs)
        real total_mass_msun;     // Total mass (solar masses)
        real polytropic_index;    // Polytropic index n
        int grid_nx, grid_ny, grid_nz;
    } input;
    
    // Code units: Dimensionless (what simulation uses)
    struct CodeUnits {
        real radius;              // Disk radius = 1.0 (dimensionless)
        real half_thickness;      // Half-thickness = z_max/R (dimensionless)
        real total_mass;          // Total mass = 1.0 (dimensionless)
        real G;                   // G = 1.0 (dimensionless)
        real polytropic_index;    // n (dimensionless)
        int grid_nx, grid_ny, grid_nz;
    } code;
    
    // Unit conversion factors
    struct UnitSystem {
        real length_unit;         // meters (R_disk in physical units)
        real mass_unit;           // kg (M_disk in physical units)
        real time_unit;           // seconds (derived)
        real velocity_unit;       // m/s (derived)
        real density_unit;        // kg/m³ (derived)
    } units;
    
    DiskInitializationParams(real R_pc, real z_pc, real M_msun, real n,
                            int nx, int ny, int nz) {
        using namespace PhysicalConstants;
        
        // Store input
        input.radius_pc = R_pc;
        input.half_thickness_pc = z_pc;
        input.total_mass_msun = M_msun;
        input.polytropic_index = n;
        input.grid_nx = nx;
        input.grid_ny = ny;
        input.grid_nz = nz;
        
        // Set up unit system (physical values)
        units.length_unit = R_pc * pc_SI;           // meters
        units.mass_unit = M_msun * M_sun_SI;        // kg
        units.time_unit = std::sqrt(units.length_unit * units.length_unit * units.length_unit 
                                    / (G_SI * units.mass_unit));
        units.velocity_unit = units.length_unit / units.time_unit;
        units.density_unit = units.mass_unit / (units.length_unit * units.length_unit * units.length_unit);
        
        // Code units (dimensionless) - used in simulation
        code.radius = 1.0;                          // Normalized
        code.half_thickness = z_pc / R_pc;          // Ratio
        code.total_mass = 1.0;                      // Normalized
        code.G = 1.0;                               // Normalized
        code.polytropic_index = n;                  // Dimensionless
        code.grid_nx = nx;
        code.grid_ny = ny;
        code.grid_nz = nz;
    }
    
    void print() const {
        std::cout << "\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           Disk Initialization Parameters                    ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ INPUT (Astronomical Units):                                  ║\n";
        std::cout << "║   Radius:          " << std::setw(10) << input.radius_pc << " pc              ║\n";
        std::cout << "║   Half-thickness:  " << std::setw(10) << input.half_thickness_pc << " pc              ║\n";
        std::cout << "║   Total mass:      " << std::setw(10) << input.total_mass_msun << " M☉              ║\n";
        std::cout << "║   Grid:            " << input.grid_nx << "×" << input.grid_ny << "×" << input.grid_nz << "                    ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ CODE UNITS (Dimensionless - used in simulation):            ║\n";
        std::cout << "║   Radius:          " << code.radius << " (normalized)                          ║\n";
        std::cout << "║   Half-thickness:  " << code.half_thickness << " (R_ratio)                           ║\n";
        std::cout << "║   Total mass:      " << code.total_mass << " (normalized)                          ║\n";
        std::cout << "║   G:               " << code.G << " (normalized)                          ║\n";
        std::cout << "╠══════════════════════════════════════════════════════════════╣\n";
        std::cout << "║ UNIT CONVERSION FACTORS:                                     ║\n";
        std::cout << "║   [L] = " << units.length_unit << " m       ║\n";
        std::cout << "║   [M] = " << units.mass_unit << " kg      ║\n";
        std::cout << "║   [T] = " << units.time_unit << " s         ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    }
};
```

### Usage in disk_relaxation.cpp

```cpp
void initialize(std::shared_ptr<Simulation> sim,
               std::shared_ptr<SPHParameters> param) override {
    
    // Create parameter struct - conversions happen ONCE here
    DiskInitializationParams params(
        /* R_pc   */ 3.0,    // 3 parsecs
        /* z_pc   */ 0.2,    // 0.2 parsecs  
        /* M_msun */ 1000.0, // 1000 solar masses
        /* n      */ 1.5,    // Polytropic index
        /* nx, ny, nz */ 50, 50, 5
    );
    
    params.print();  // Show input, code units, and conversion factors
    
    // ═══════════════════════════════════════════════════════════
    // From here on: ONLY use params.code.* (dimensionless values)
    // ═══════════════════════════════════════════════════════════
    
    const real R_disk = params.code.radius;           // = 1.0
    const real z_max = params.code.half_thickness;    // = 0.2/3.0 ≈ 0.067
    const real M_total = params.code.total_mass;      // = 1.0
    const real G = params.code.G;                     // = 1.0
    
    // Grid generation in CODE UNITS (dimensionless)
    const real dx = (2.0 * R_disk) / params.code.grid_nx;  // ≈ 0.04
    const real dy = (2.0 * R_disk) / params.code.grid_ny;  // ≈ 0.04
    const real dz = (2.0 * z_max) / params.code.grid_nz;   // ≈ 0.027
    
    // Generate particles in CODE UNITS
    std::vector<vec_t> positions_code;
    for (int ix = 0; ix < params.code.grid_nx; ++ix) {
        for (int iy = 0; iy < params.code.grid_ny; ++iy) {
            real x = -R_disk + (ix + 0.5) * dx;  // range: [-1, 1]
            real y = -R_disk + (iy + 0.5) * dy;  // range: [-1, 1]
            
            if (x*x + y*y > R_disk*R_disk) continue;  // r > 1.0
            
            for (int iz = 0; iz < params.code.grid_nz; ++iz) {
                real z = -z_max + (iz + 0.5) * dz;  // range: ~[-0.067, 0.067]
                positions_code.push_back({x, y, z});
            }
        }
    }
    
    const real mpp = M_total / positions_code.size();  // ≈ 1e-4 (code units)
    
    // Create particles
    std::vector<SPHParticle> particles;
    for (const auto &pos : positions_code) {
        real x = pos[0], y = pos[1], z = pos[2];
        
        // Lane-Emden profile (in code units)
        real r_xy = std::sqrt(x*x + y*y);
        real xi = r_xy / alpha;  // alpha = R_disk / xi1
        real theta = getTheta_2d(xi);
        
        // Polytropic relations (dimensionless)
        real dens_code = rho_c * std::pow(theta, n_poly);
        real pres_code = K * std::pow(dens_code, 1.0 + 1.0/n_poly);
        real ene_code = pres_code / ((gamma - 1.0) * dens_code);
        
        SPHParticle pp;
        pp.pos[0] = x;            // CODE UNITS (dimensionless)
        pp.pos[1] = y;            // CODE UNITS (dimensionless)
        pp.pos[2] = z;            // CODE UNITS (dimensionless)
        pp.mass = mpp;            // CODE UNITS (dimensionless)
        pp.dens = dens_code;      // CODE UNITS (dimensionless)
        pp.pres = pres_code;      // CODE UNITS (dimensionless)
        pp.ene = ene_code;        // CODE UNITS (dimensionless)
        // ... rest
        
        particles.push_back(pp);
    }
    
    sim->set_particles(particles);
    
    // Set G in code units
    param->gravity.constant = params.code.G;  // = 1.0
    
    // Store unit conversion info for output
    sim->set_unit_system(params.units);  // For converting back to physical
}
```

### Output Conversion (in CSV writer)

```cpp
// When writing CSV, convert back to physical units
void writeCSV(const std::vector<SPHParticle> &particles, const UnitSystem &units) {
    for (const auto &p : particles) {
        // Convert from code units to physical (SI)
        real pos_x_m = p.pos[0] * units.length_unit;   // meters
        real pos_y_m = p.pos[1] * units.length_unit;   // meters
        real pos_z_m = p.pos[2] * units.length_unit;   // meters
        real mass_kg = p.mass * units.mass_unit;       // kilograms
        real dens_kg_m3 = p.dens * units.density_unit; // kg/m³
        
        // Write to CSV with physical units
        csv << pos_x_m << "," << pos_y_m << "," << pos_z_m << ","
            << mass_kg << "," << dens_kg_m3 << "\n";
    }
}
```

## Benefits

1. **Numerical Stability**: All values O(1) in simulation (pos ~ 1, mass ~ 1)
2. **No Neighbor Overflow**: Reasonable smoothing lengths (~0.01-0.1)
3. **Single Point Conversion**: Input → code units (once), code units → output (once)
4. **Clear Separation**: 
   - `params.input.*` = astronomical units (user-facing)
   - `params.code.*` = dimensionless (simulation)
   - `params.units.*` = conversion factors (for output)
5. **Self-Documenting**: Variable names show which system
6. **Standard Practice**: This is how modern SPH codes work (GADGET, SWIFT, etc.)

## Key Differences from SI Approach

| Aspect | SI Units | Code Units (Dimensionless) |
|--------|----------|----------------------------|
| Position | 10^16 m | 0.0-1.0 |
| Mass | 10^30 kg | 0.0-1.0 |
| G constant | 6.67e-11 | 1.0 |
| Neighbor search | Overflow! | Stable |
| Smoothing length | 10^15 m | ~0.01 |
| **Simulation** | Hard | Easy |
| **Output** | Direct | Need conversion |

## Migration Steps

1. ✅ Create `disk_initialization_params.hpp` with code units
2. Update `disk_relaxation.cpp` to use dimensionless values
3. Modify CSV output to convert back to physical units
4. Update config: `G = 1.0` instead of `G = 0.0043`
5. Verify: positions in [-1, 1], masses sum to 1.0
6. Run simulation - should be stable with O(1) values

## Testing

Verify dimensionless simulation:
- Positions in range [-1, 1]
- Total mass = 1.0
- G = 1.0 in simulation
- CSV output shows ~10^16 m (after conversion)
- CSV output shows ~10^30 kg (after conversion)
- No neighbor list warnings
- Stable evolution
