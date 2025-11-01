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
        
        std::cout << "Initializing 2.5D Polytropic Disk Relaxation...\n";
        std::cout << "This is Step 1 of the Razor-Thin HVCC Workflow\n";
        std::cout << "Output will be used as initial conditions for flyby simulation\n\n";
        
        // Fluid parameters
        const real gamma = 5.0 / 3.0;  // Polytrope with γ = 5/3
        const real n_poly = 1.5;       // Polytropic index
        const real R_fluid = 3.0;      // Fluid disk radius in x-y (parsecs)
        const real z_max = 0.2;        // Half-thickness in z (parsecs)
        const real M_total = 1000.0;   // Total fluid mass (solar masses)
        const real K = 1.0;            // Polytropic constant
        const real rho_c = 1.0;        // Central density
        
        std::cout << "Disk parameters:\n";
        std::cout << "  Radius: " << R_fluid << " pc\n";
        std::cout << "  Half-thickness: " << z_max << " pc\n";
        std::cout << "  Total mass: " << M_total << " M☉\n";
        std::cout << "  Polytropic index: n = " << n_poly << "\n";
        std::cout << "  Gamma: γ = " << gamma << "\n\n";
        
        // Lane-Emden setup - load 2D table
        std::string lane_emden_csv = "simulations/workflows/razor_thin_hvcc_workflow/01_relaxation/lane_emden_2d_data.csv";
        loadLaneEmdenTableFromCSV_2d(lane_emden_csv);
        
        const real xi1 = laneEmden_x_2d.back();  // First zero of θ
        const real alpha = R_fluid / xi1;         // Radial scaling factor
        
        // Grid setup - adjust resolution here
        int Nx = 50, Ny = 50, Nz = 5;  // Default: ~10k particles
        
        // Note: Particle count determined by grid resolution
        // Total particles = Nx * Ny * Nz (approximately, after filtering)
        
        const real dx = (2.0 * R_fluid) / Nx;
        const real dy = (2.0 * R_fluid) / Ny;
        const real dz = (2.0 * z_max) / Nz;
        
        std::cout << "Grid resolution: " << Nx << " × " << Ny << " × " << Nz << "\n";
        std::cout << "Cell size: dx=" << dx << ", dy=" << dy << ", dz=" << dz << "\n\n";
        
        // Generate fluid particle positions
        std::vector<vec_t> fluid_positions;
        fluid_positions.reserve(Nx * Ny * Nz);
        
        for (int ix = 0; ix < Nx; ++ix) {
            for (int iy = 0; iy < Ny; ++iy) {
                real x = -R_fluid + (ix + 0.5) * dx;
                real y = -R_fluid + (iy + 0.5) * dy;
                
                // Only include particles within disk radius
                if (x * x + y * y > R_fluid * R_fluid)
                    continue;
                
                for (int iz = 0; iz < Nz; ++iz) {
                    real z = -z_max + (iz + 0.5) * dz;
                    fluid_positions.push_back({x, y, z});
                }
            }
        }
        
        size_t fluid_count = fluid_positions.size();
        const real mpp = M_total / static_cast<real>(fluid_count);  // Mass per particle
        
        std::cout << "Generated " << fluid_count << " particle positions\n";
        std::cout << "Mass per particle: " << mpp << " M☉\n\n";
        
        // Initialize particles with Lane-Emden profile
        std::vector<SPHParticle> particles;
        particles.reserve(fluid_count);
        
        int pid = 0;
        for (const auto &pos : fluid_positions) {
            real x = pos[0];
            real y = pos[1];
            real z = pos[2];
            
            // Compute polytropic properties based on radial position
            real r_xy = std::sqrt(x * x + y * y);
            real xi = r_xy / alpha;
            real thetaVal = getTheta_2d(xi);
            
            if (thetaVal < 0.0)
                thetaVal = 0.0;  // Ensure non-negative
            
            // Polytropic relations
            real dens = rho_c * std::pow(thetaVal, n_poly);
            real pres = K * std::pow(dens, 1.0 + 1.0 / n_poly);
            real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;
            
            // Create particle
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
            pp.id = pid++;
            pp.volume = pp.mass / pp.dens;  // For DISPH
            pp.is_wall = false;
            pp.is_point_mass = false;
            
            particles.push_back(pp);
        }
        
        sim->set_particles(particles);
        sim->set_particle_num(static_cast<int>(particles.size()));
        
        // Set simulation parameters for relaxation
        param->type = SPHType::DISPH;
        param->physics.gamma = gamma;
        param->physics.neighbor_number = 64;  // Good for relaxation
        param->gravity.is_valid = true;
        param->gravity.constant = 0.0043;  // Gravitational constant (field name: constant, not G)
        param->two_and_half_sim = true;  // 2.5D mode
        
        // Enable density relaxation if not already set via config
        if (!param->density_relaxation.is_valid) {
            std::cout << "NOTE: Enable density relaxation in config.json:\n";
            std::cout << "  \"densityRelaxation\": {\n";
            std::cout << "    \"enabled\": true,\n";
            std::cout << "    \"maxIterations\": 1000,\n";
            std::cout << "    \"tolerance\": 0.1,\n";
            std::cout << "    \"dampingFactor\": 0.2\n";
            std::cout << "  }\n\n";
        }
        
        std::cout << "Disk initialization complete!\n";
        std::cout << "Run simulation to relax to hydrostatic equilibrium.\n";
        std::cout << "Final snapshot will be used as IC for flyby simulation.\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"disk_relaxation.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::DiskRelaxationPlugin)
