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
 * @brief Kelvin-Helmholtz Instability
 * 
 * Classic hydrodynamic instability test with shear flow:
 * - Two horizontal layers with opposite velocities
 * - Sinusoidal velocity perturbations to seed instability
 * - Dense region (ρ=2) in center with v_x = +0.5
 * - Ambient regions (ρ=1) at top/bottom with v_x = -0.5
 * 
 * The shear interface becomes unstable and develops characteristic vortex rolls.
 */
class KelvinHelmholtzPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "kelvin_helmholtz";
    }
    
    std::string get_description() const override {
        return "2D Kelvin-Helmholtz Instability - Shear flow instability with vortex formation";
    }
    
    std::string get_version() const override {
        return "2.0.0";  // Version 2.0 - plugin-based architecture
    }
    
    void initialize(std::shared_ptr<Simulation> sim, 
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 2
        THROW_ERROR("Kelvin-Helmholtz instability requires DIM = 2");
#endif
        
        std::cout << "Initializing Kelvin-Helmholtz instability...\n";
        
        // Resolution parameter
        int N = 50;  // Base resolution
        const int num = N * N * 3 / 4;  // 3/4 filling for variable spacing
        const real dx = 1.0 / N;
        const real mass = 1.5 / num;  // Total mass = 1.5
        const real gamma = param->physics.gamma;
        
        std::vector<SPHParticle> particles(num);
        
        // Velocity perturbation function
        auto vy = [](real xx, real yy) -> real {
            constexpr real sigma2_inv = 2.0 / (0.05 * 0.05);
            return 0.1 * std::sin(4.0 * M_PI * xx) * 
                   (std::exp(-sqr(yy - 0.25) * 0.5 * sigma2_inv) + 
                    std::exp(-sqr(yy - 0.75) * 0.5 * sigma2_inv));
        };
        
        // Initialize particles with variable spacing
        real x = dx * 0.5;
        real y = dx * 0.5;
        int region = 1;  // 1 = ambient (top/bottom), 2 = dense (center)
        bool odd = true;
        
        for (int i = 0; i < num; ++i) {
            auto& p = particles[i];
            
            // Position
            p.pos[0] = x;
            p.pos[1] = y;
            
            // Velocity: horizontal shear + vertical perturbation
            p.vel[0] = (region == 1 ? -0.5 : 0.5);  // Opposite flows
            p.vel[1] = vy(x, y);  // Sinusoidal perturbation
            
            // Mass
            p.mass = mass;
            
            // Density: region-dependent
            p.dens = static_cast<real>(region);  // ρ=1 or ρ=2
            
            // Pressure: uniform
            p.pres = 2.5;
            
            // Internal energy
            p.ene = p.pres / ((gamma - 1.0) * p.dens);
            
            // Volume element for DISPH/GSPH
            p.volume = p.mass / p.dens;
            
            // Particle ID
            p.id = i;
            
            // Step x depending on region (variable spacing for better resolution)
            x += (region == 1 ? 2.0 * dx : dx);
            
            // Wrap to next row
            if (x > 1.0) {
                y += dx;
                
                // Switch region based on y position
                // Center strip (0.25 < y < 0.75) is dense region
                if (y > 0.25 && y < 0.75)
                    region = 2;
                else
                    region = 1;
                
                // Staggered grid pattern for region 1
                if (region == 1) {
                    if (odd) {
                        odd = false;
                        x = dx * 1.5;
                    } else {
                        odd = true;
                        x = dx * 0.5;
                    }
                } else {
                    x = dx * 0.5;
                }
            }
        }
        
        sim->set_particles(particles);
        sim->set_particle_num(num);
        
        // Set simulation parameters
        param->type = SPHType::GSPH;  // Use GSPH for this test
        param->time.end = 10.0;
        param->time.output = 0.1;
        param->physics.neighbor_number = 32;
        
        std::cout << "Created " << num << " particles\n";
        std::cout << "  Ambient regions: ρ=1.0, v_x=-0.5\n";
        std::cout << "  Dense center: ρ=2.0, v_x=+0.5\n";
        std::cout << "  Perturbation amplitude: δv_y=0.1\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"kelvin_helmholtz.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::KelvinHelmholtzPlugin)
