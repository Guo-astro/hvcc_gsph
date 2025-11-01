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
 * @brief Sod Shock Tube Problem
 * 
 * Classic 1D Riemann problem with discontinuity at x=0:
 * - Left state (x < 0): ρ=1.0, P=1.0, v=0
 * - Right state (x > 0): ρ=0.125, P=0.1, v=0
 * 
 * This creates a shock wave, contact discontinuity, and rarefaction wave.
 */
class ShockTubePlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "shock_tube";
    }
    
    std::string get_description() const override {
        return "Sod shock tube problem - 1D Riemann problem";
    }
    
    std::string get_version() const override {
        return "2.0.0";  // Version 2.0 - plugin-based architecture
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 1
        THROW_ERROR("Shock tube requires DIM=1");
#endif

        std::cout << "Initializing Sod shock tube problem...\n";
        
        // Particle resolution
        int N = 400;  // Total number of particles
        
        // Domain: [-0.5, 0.5], discontinuity at x=0
        const real x_min = -0.5;
        const real x_max = 0.5;
        const real domain_length = x_max - x_min;
        const real dx = domain_length / N;

        std::vector<SPHParticle> p(N);

        // Physical parameters
        const real gamma = param->physics.gamma;
        
        // Left state (x < 0)
        const real rho_L = 1.0;
        const real P_L = 1.0;
        const real v_L = 0.0;
        
        // Right state (x > 0)
        const real rho_R = 0.125;
        const real P_R = 0.1;
        const real v_R = 0.0;

        // Create particles
        for (int i = 0; i < N; ++i) {
            auto &p_i = p[i];
            
            // Position: uniform spacing
            p_i.pos[0] = x_min + (i + 0.5) * dx;
            
            // Set state based on position (discontinuity at x=0)
            if (p_i.pos[0] < 0.0) {
                // Left state
                p_i.dens = rho_L;
                p_i.pres = P_L;
                p_i.vel[0] = v_L;
                p_i.mass = rho_L * dx;  // mass = ρ * dx in 1D
            } else {
                // Right state
                p_i.dens = rho_R;
                p_i.pres = P_R;
                p_i.vel[0] = v_R;
                p_i.mass = rho_R * dx;  // mass = ρ * dx in 1D
            }
            
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;
            
            // Initialize volume element for DISPH: V = m/ρ
            p_i.volume = p_i.mass / p_i.dens;
        }

        sim->set_particles(p);
        sim->set_particle_num(N);
        
        std::cout << "Created " << N << " particles\n";
        std::cout << "  Domain: [" << x_min << ", " << x_max << "]\n";
        std::cout << "  Particle spacing: " << dx << "\n";
        std::cout << "  SPH Type: " << (param->type == SPHType::GDISPH ? "GDISPH" : 
                                       param->type == SPHType::SSPH ? "SSPH" : 
                                       param->type == SPHType::DISPH ? "DISPH" : "UNKNOWN") << "\n";
        std::cout << "  Left state:  ρ=" << rho_L << ", P=" << P_L << "\n";
        std::cout << "  Right state: ρ=" << rho_R << ", P=" << P_R << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"shock_tube.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::ShockTubePlugin)
