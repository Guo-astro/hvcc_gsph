#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>

#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/defines.hpp"
#include "utilities/exception.hpp"
#include "utilities/flexible_unit_params.hpp"  // Flexible unit system
#include "lane_emden_2d.hpp"  // 2D Lane-Emden functions

namespace sph
{

/**
 * @brief Disk Relaxation Plugin - Step 1 of Razor-Thin HVCC Workflow
 * 
 * Generates a 2.5D polytropic disk using Lane-Emden profile (n=1.5) and
 * applies density relaxation to reach hydrostatic equilibrium.
 * 
 * This is the first step of a 2-step workflow:
 * 1. This plugin: Create relaxed disk initial conditions
 * 2. razor_thin_hvcc: Run IMBH flyby with relaxed disk
 * 
 * Output: Relaxed disk particles saved to initial_conditions/
 */
class DiskRelaxationPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "disk_relaxation";
    }
    
    std::string get_description() const override {
        return "2.5D Polytropic Disk Relaxation - Generates equilibrium disk for HVCC workflow";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 3
        THROW_ERROR("Disk relaxation requires DIM = 3 (uses 2.5D mode)");
#endif
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║     2.5D Polytropic Disk Relaxation - HVCC Workflow Step 1    ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 1: Define Physical Parameters (in astronomical units)
        // ═══════════════════════════════════════════════════════════════
        
        const real R_disk_pc = 3.0;       // Disk radius in parsecs
        const real z_max_pc = 0.2;        // Half-thickness in parsecs
        const real M_disk_Msun = 1000.0;  // Total disk mass in solar masses
        const real gamma = 5.0 / 3.0;     // Adiabatic index
        const real n_poly = 1.5;          // Polytropic index (n=1.5 → γ=5/3)
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 2: Create Flexible Unit System
        // ═══════════════════════════════════════════════════════════════
        
        // Input and output in astronomical units (pc, M☉)
        // Simulation uses dimensionless units internally (R=1, M=1, G=1)
        FlexibleUnitParams units(
            R_disk_pc,                      // Characteristic length: 3 pc
            M_disk_Msun,                    // Characteristic mass: 1000 M☉
            UnitSystemType::GALACTIC_PC,    // Input in pc/M☉
            UnitSystemType::GALACTIC_PC     // Output in pc/M☉ (same for simplicity)
        );
        
        // Show complete unit configuration
        units.print();
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 3: Convert Input Parameters to Code Units (Dimensionless)
        // ═══════════════════════════════════════════════════════════════
        
        const real R_disk = units.input_to_code_length(R_disk_pc);    // = 1.0
        const real z_max = units.input_to_code_length(z_max_pc);      // = 0.0667
        const real M_disk = units.input_to_code_mass(M_disk_Msun);    // = 1.0
        
        std::cout << "\n";
        std::cout << "Physical Parameters (Input):\n";
        std::cout << "  Disk radius:      " << R_disk_pc << " pc\n";
        std::cout << "  Half-thickness:   " << z_max_pc << " pc\n";
        std::cout << "  Total mass:       " << M_disk_Msun << " M☉\n";
        std::cout << "  Polytropic index: n = " << n_poly << "\n";
        std::cout << "  Adiabatic index:  γ = " << gamma << "\n";
        std::cout << "\n";
        std::cout << "Code Units (Dimensionless - normalized for simulation):\n";
        std::cout << "  Disk radius:      " << R_disk << " (normalized)\n";
        std::cout << "  Half-thickness:   " << z_max << " (normalized)\n";
        std::cout << "  Total mass:       " << M_disk << " (normalized)\n";
        std::cout << "  G:                " << units.code.G << " (normalized)\n";
        std::cout << "\n";
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 4: Load Lane-Emden Profile
        // ═══════════════════════════════════════════════════════════════
        
        std::string lane_emden_csv = "lane_emden_2d_data.csv";
        std::cout << "Loading Lane-Emden table: " << lane_emden_csv << "\n";
        loadLaneEmdenTableFromCSV_2d(lane_emden_csv);
        
        const real xi1 = laneEmden_x_2d.back();  // First zero of θ (dimensionless)
        const real alpha = R_disk / xi1;          // Radial scaling factor
        
        std::cout << "  ξ₁ (first zero):  " << xi1 << "\n";
        std::cout << "  Scaling factor α: " << alpha << "\n";
        std::cout << "\n";
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 5: Setup Particle Grid
        // ═══════════════════════════════════════════════════════════════
        
        int Nx = 50, Ny = 50, Nz = 5;  // Grid resolution
        
        const real dx = (2.0 * R_disk) / Nx;
        const real dy = (2.0 * R_disk) / Ny;
        const real dz = (2.0 * z_max) / Nz;
        
        std::cout << "Particle Grid Setup:\n";
        std::cout << "  Resolution: " << Nx << " × " << Ny << " × " << Nz << "\n";
        std::cout << "  Cell size:  Δx=" << dx << ", Δy=" << dy << ", Δz=" << dz << " (code units)\n";
        std::cout << "\n";
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 6: Generate Particle Positions
        // ═══════════════════════════════════════════════════════════════
        
        std::vector<vec_t> fluid_positions;
        fluid_positions.reserve(Nx * Ny * Nz);
        
        for (int ix = 0; ix < Nx; ++ix) {
            for (int iy = 0; iy < Ny; ++iy) {
                real x = -R_disk + (ix + 0.5) * dx;
                real y = -R_disk + (iy + 0.5) * dy;
                
                // Only include particles within disk radius
                if (x * x + y * y > R_disk * R_disk)
                    continue;
                
                for (int iz = 0; iz < Nz; ++iz) {
                    real z = -z_max + (iz + 0.5) * dz;
                    fluid_positions.push_back({x, y, z});
                }
            }
        }
        
        size_t fluid_count = fluid_positions.size();
        const real mpp = M_disk / static_cast<real>(fluid_count);  // Mass per particle (code units)
        
        std::cout << "Particle Generation:\n";
        std::cout << "  Total particles:   " << fluid_count << "\n";
        std::cout << "  Mass per particle: " << mpp << " (code units)\n";
        std::cout << "                     " << units.code_to_output_mass(mpp) << " M☉\n";
        std::cout << "\n";
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 7: Initialize Particles with Lane-Emden Density Profile
        // ═══════════════════════════════════════════════════════════════
        
        const real K = 1.0;       // Polytropic constant (code units)
        const real rho_c = 1.0;   // Central density (code units)
        
        std::vector<SPHParticle> particles;
        particles.reserve(fluid_count);
        
        int pid = 0;
        for (const auto &pos : fluid_positions) {
            real x = pos[0];
            real y = pos[1];
            real z = pos[2];
            
            // Compute radial position in x-y plane
            real r_xy = std::sqrt(x * x + y * y);
            real xi = r_xy / alpha;  // Dimensionless radial coordinate
            real thetaVal = getTheta_2d(xi);
            
            if (thetaVal < 0.0)
                thetaVal = 0.0;  // Ensure non-negative
            
            // Polytropic relations (all in code units)
            real dens = rho_c * std::pow(thetaVal, n_poly);
            real pres = K * std::pow(dens, 1.0 + 1.0 / n_poly);
            real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;
            
            // Estimate initial smoothing length based on particle spacing
            // For a uniform grid, h ≈ 2 * spacing (to ensure ~50-100 neighbors)
            real h_initial = 2.0 * dx;  // Use 2x the grid spacing
            
            // Create particle (all quantities in code units)
            SPHParticle pp;
            pp.pos[0] = x;
            pp.pos[1] = y;
            pp.pos[2] = z;
            pp.vel[0] = 0.0;
            pp.vel[1] = 0.0;
            pp.vel[2] = 0.0;
            pp.mass = mpp;
            pp.dens = dens;
            pp.pres = pres;
            pp.ene = ene;
            pp.sml = h_initial;  // CRITICAL: Set initial smoothing length
            pp.id = pid++;
            // Note: pp.volume will be calculated by GDISPH during pre-interaction
            // Do NOT set it here to avoid division by zero when dens=0
            pp.is_wall = false;
            pp.is_point_mass = false;
            
            particles.push_back(pp);
        }
        
        sim->set_particles(particles);
        sim->set_particle_num(static_cast<int>(particles.size()));
        
        std::cout << "Initialization Complete:\n";
        std::cout << "  Central density:   ρ_c = " << rho_c << " (code units)\n";
        std::cout << "  Polytropic const:  K = " << K << " (code units)\n";
        std::cout << "  All particles initialized with Lane-Emden profile\n";
        std::cout << "\n";
        
        real gradh; // grad-h term
        real volume; // volume element V = m/ρ (for GDISPH)
        real q; // smoothed internal energy density (for GDISPH): q = Σ_j (m_j*u_j)*W_ij
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 8: Configure Simulation Parameters
        // ═══════════════════════════════════════════════════════════════
        
        param->type = SPHType::GDISPH;
        param->physics.gamma = gamma;
        param->physics.neighbor_number = 64;
        param->gravity.is_valid = true;
        param->gravity.constant = units.code.G;  // = 1.0 in code units
        param->two_and_half_sim = true;          // Enable 2.5D mode
        
        std::cout << "Simulation Configuration:\n";
        std::cout << "  SPH Type:          GDISPH\n";
        std::cout << "  Adiabatic index:   γ = " << gamma << "\n";
        std::cout << "  Neighbor number:   " << param->physics.neighbor_number << "\n";
        std::cout << "  Gravity enabled:   " << (param->gravity.is_valid ? "Yes" : "No") << "\n";
        std::cout << "  G (code units):    " << param->gravity.constant << "\n";
        std::cout << "  2.5D mode:         " << (param->two_and_half_sim ? "Enabled" : "Disabled") << "\n";
        std::cout << "\n";
        
        // ═══════════════════════════════════════════════════════════════
        // STEP 9: Reminder for Density Relaxation
        // ═══════════════════════════════════════════════════════════════
        
        if (!param->density_relaxation.is_valid) {
            std::cout << "⚠️  IMPORTANT: Enable density relaxation in config.json:\n";
            std::cout << "  \"densityRelaxation\": {\n";
            std::cout << "    \"enabled\": true,\n";
            std::cout << "    \"maxIterations\": 1000,\n";
            std::cout << "    \"tolerance\": 0.1,\n";
            std::cout << "    \"dampingFactor\": 0.2\n";
            std::cout << "  }\n";
        } else {
            std::cout << "✓ Density relaxation enabled\n";
        }
        std::cout << "\n";
        
        std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  Disk Initialization Complete - Ready for Relaxation          ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
        std::cout << "Next Steps:\n";
        std::cout << "  1. Run simulation to relax to hydrostatic equilibrium\n";
        std::cout << "  2. Final snapshot will be used as IC for IMBH flyby\n";
        std::cout << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"disk_relaxation.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::DiskRelaxationPlugin)
