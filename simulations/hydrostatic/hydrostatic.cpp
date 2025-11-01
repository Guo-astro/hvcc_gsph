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
 * @brief Hydrostatic Equilibrium Test
 * 
 * Tests pressure balance in 2D configuration with:
 * - Dense central region: ρ=4.0, r < 0.1
 * - Ambient medium: ρ=1.0, r ≥ 0.1
 * - Uniform pressure: P=2.5 everywhere
 * 
 * Particles should remain nearly stationary if pressure forces balance density gradients.
 */
class HydrostaticPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "hydrostatic";
    }
    
    std::string get_description() const override {
        return "2D Hydrostatic Equilibrium Test - Tests pressure balance with dense center and ambient medium";
    }
    
    std::string get_version() const override {
        return "2.0.0";  // Version 2.0 - plugin-based architecture
    }
    
    void initialize(std::shared_ptr<Simulation> sim, 
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 2
        THROW_ERROR("Hydrostatic test requires DIM = 2");
#endif
        
        std::cout << "Initializing hydrostatic equilibrium test...\n";
        
        // Get N parameter from config (via rangeMax/rangeMin and spacing)
        // For now, use default or could add custom parameter
        int N = 32;
        
        // Setup grid
        const int num = N * N;
        const real dx = 1.0 / N;
        const real mass = 1.0 / num;
        const real gamma = param->physics.gamma;
        
        std::vector<SPHParticle> particles(num);
        
        // Initialize particles in grid pattern
        int idx = 0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                auto& p = particles[idx];
                
                // Position
                p.pos[0] = (i + 0.5) * dx - 0.5;
                p.pos[1] = (j + 0.5) * dx - 0.5;
                
                // Velocity (initially at rest)
                p.vel[0] = 0.0;
                p.vel[1] = 0.0;
                
                // Density: dense region in center (r < 0.1), ambient elsewhere
                real r = std::sqrt(p.pos[0] * p.pos[0] + p.pos[1] * p.pos[1]);
                if (r < 0.1) {
                    p.dens = 4.0;  // Dense center
                } else {
                    p.dens = 1.0;  // Ambient medium
                }
                
                // Mass
                p.mass = mass;
                
                // Pressure (constant everywhere for equilibrium)
                p.pres = 2.5;
                
                // Internal energy from pressure and density
                p.ene = p.pres / ((gamma - 1.0) * p.dens);
                
                // Volume element for DISPH
                p.volume = p.mass / p.dens;
                
                // Particle ID
                p.id = idx;
                
                ++idx;
            }
        }
        
        sim->set_particles(particles);
        sim->set_particle_num(num);
        
        // Set simulation parameters
        param->type = SPHType::DISPH;
        param->time.end = 8.0;
        param->time.output = 0.1;
        param->physics.neighbor_number = 32;
        
        std::cout << "Created " << num << " particles\n";
        std::cout << "  Dense center: ρ=4.0, r < 0.1\n";
        std::cout << "  Ambient: ρ=1.0, r ≥ 0.1\n";
        std::cout << "  Uniform pressure: P=2.5\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"hydrostatic.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::HydrostaticPlugin)
