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
        int N = 50;  // Particles per unit length
        const real dx_r = 0.5 / N;  // Right side spacing
        const real dx_l = dx_r * 0.25;  // Left side spacing (higher resolution)

        const int num = N * 10;
        std::vector<SPHParticle> p(num);

        // Initial conditions
        real x = -0.5 + dx_l * 0.5;
        real dx = dx_l;
        real dens = 1.0;   // Left state density
        real pres = 1.0;   // Left state pressure
        const real mass = 0.5 / N * 0.25;
        const real gamma = param->physics.gamma;
        bool left = true;

        // Create particles
        for (int i = 0; i < num; ++i) {
            auto &p_i = p[i];
            p_i.pos[0] = x;
            p_i.vel[0] = 0.0;
            p_i.dens = dens;
            p_i.pres = pres;
            p_i.mass = mass;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;
            // Initialize volume element for DISPH: V = m/ρ
            p_i.volume = p_i.mass / p_i.dens;

            x += dx;
            
            // Switch to right state at x = 0
            if (x > 0.0 && left) {
                x = 0.0 + dx_r * 0.5;
                dx = dx_r;
                dens = 0.125;  // Right state density
                pres = 0.1;    // Right state pressure
                left = false;
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(num);
        
        // Set simulation parameters
        param->type = SPHType::GDISPH;
        param->time.end = 0.2;
        param->time.output = 0.02;
        param->cfl.sound = 0.3;
        param->physics.neighbor_number = 50;
        
        std::cout << "Created " << num << " particles\n";
        std::cout << "  Left state:  ρ=" << 1.0 << ", P=" << 1.0 << "\n";
        std::cout << "  Right state: ρ=" << 0.125 << ", P=" << 0.1 << "\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"shock_tube.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::ShockTubePlugin)
