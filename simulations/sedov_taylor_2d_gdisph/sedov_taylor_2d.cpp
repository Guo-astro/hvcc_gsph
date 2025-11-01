#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/exception.hpp"
#include "utilities/defines.hpp"
#include <iostream>
#include <cmath>
#include <vector>

namespace sph {

/**
 * @brief 2D Sedov-Taylor Blast Wave Problem using GDISPH
 * 
 * 2D point explosion in a uniform medium using Godunov DISPH (GDISPH).
 * This variant uses GDISPH for comparison with standard DISPH version.
 * 
 * Initial conditions:
 * - Uniform density ρ=1.0
 * - Energy E₀=1.0 injected in central region
 * - Zero initial velocity
 * 
 * Creates a circular blast wave expanding outward. The analytical solution
 * is self-similar and can be compared with simulation results.
 * 
 * GDISPH features:
 * - Godunov-type Riemann solver for flux calculation
 * - Better shock capturing than standard DISPH
 * - Improved handling of strong discontinuities
 * - Comparison variant for algorithm testing
 */
class SedovTaylor2DGDISPHPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "sedov_taylor_2d_gdisph";
    }
    
    std::string get_description() const override {
        return "2D Sedov-Taylor blast wave using GDISPH - Algorithm comparison variant";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 2
        THROW_ERROR("Sedov-Taylor 2D requires DIM=2");
#endif

        std::cout << "Initializing 2D Sedov-Taylor blast wave (GDISPH)...\n";
        
        // Blast wave parameters
        const real E0 = 1.0;          // Total energy of explosion
        const real rho0 = 1.0;        // Background density
        const real gamma = 1.4;       // Adiabatic index (diatomic gas)
        
        // Domain: square [-0.5, 0.5]²
        const real L = 1.0;
        const int N_1D = 100;         // Particles per dimension (100x100 = 10k particles)
        const int N = N_1D * N_1D;
        
        const real dx = L / N_1D;
        const real mass = (L*L) * rho0 / N;  // Uniform mass
        
        // Energy injection region (central particles)
        const real r_inject = 2.5 * dx;  // Slightly larger for 2D
        
        std::vector<SPHParticle> p(N);
        int idx = 0;
        
        // Create uniform lattice
        for (int i = 0; i < N_1D; ++i) {
            for (int j = 0; j < N_1D; ++j) {
                auto &p_i = p[idx];
                
                // Position: uniform grid
                p_i.pos[0] = -0.5*L + (i + 0.5) * dx;
                p_i.pos[1] = -0.5*L + (j + 0.5) * dx;
#if DIM == 3
                p_i.pos[2] = 0.0;
#endif
                
                // Distance from center
                real r = std::sqrt(p_i.pos[0]*p_i.pos[0] + p_i.pos[1]*p_i.pos[1]);
                
                // Velocity: initially zero
                p_i.vel[0] = 0.0;
                p_i.vel[1] = 0.0;
#if DIM == 3
                p_i.vel[2] = 0.0;
#endif
                
                // Density and mass: uniform
                p_i.dens = rho0;
                p_i.mass = mass;
                
                // Pressure and energy: inject energy in central region
                if (r < r_inject) {
                    // Number of particles in injection region (approx)
                    real A_inject = M_PI * r_inject * r_inject;  // Circular area
                    int N_inject = static_cast<int>(A_inject / (dx*dx));
                    if (N_inject < 1) N_inject = 1;
                    
                    // Distribute energy among central particles
                    real e_inject = E0 / (N_inject * mass);
                    p_i.ene = e_inject;
                    p_i.pres = (gamma - 1.0) * p_i.dens * p_i.ene;
                } else {
                    // Background: low energy
                    p_i.ene = 1.0e-6;
                    p_i.pres = (gamma - 1.0) * p_i.dens * p_i.ene;
                }
                
                p_i.id = idx;
                p_i.volume = p_i.mass / p_i.dens;
                
                ++idx;
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(N);
        
        // Set simulation parameters for DISPH
        param->type = SPHType::DISPH;  // Use DISPH instead of GSPH
        param->time.end = 0.1;
        param->time.output = 0.01;
        param->cfl.sound = 0.3;
        param->cfl.force = 0.125;
        param->physics.neighbor_number = 32;  // Typical for 2D
        param->physics.gamma = gamma;
        param->av.alpha = 1.0;
        param->av.use_time_dependent_av = false;  // DISPH doesn't need time-dependent AV
        param->gsph.force_correction = true;
        
        std::cout << "Created " << N << " particles\n";
        std::cout << "  Using GDISPH (Godunov DISPH) for shock capturing\n";
        std::cout << "  Initial energy: E = " << E0 << "\n";
        std::cout << "  Energy injection radius: r_inj = " << r_inject << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"sedov_taylor_2d.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::SedovTaylor2DGDISPHPlugin)
