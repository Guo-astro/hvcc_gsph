#include "core/simulation_plugin.hpp"
#include <iostream>
#include <cmath>

namespace sph {

/**
 * @brief Template simulation plugin
 * 
 * This is a template showing how to create a self-contained simulation.
 * Copy this file and modify it to create your own simulation case.
 */
class TemplateSimulationPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "template";
    }
    
    std::string get_description() const override {
        return "Template simulation - modify this to create your own case";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation> sim, 
                   std::shared_ptr<SPHParameters> params) override {
        std::cout << "Initializing template simulation...\n";
        
        // Example: Create particles in a simple 1D setup
        const int n_particles = params->total_particle_count;
        const double dx = 1.0 / n_particles;
        
        for (int i = 0; i < n_particles; ++i) {
            double x = (i + 0.5) * dx;
            
            // Add particle at position x
            sim->add_particle(
                x, 0.0, 0.0,        // position
                0.0, 0.0, 0.0,      // velocity
                1.0,                 // density
                0.0,                 // pressure (will be computed)
                1.0,                 // mass
                dx                   // smoothing length
            );
        }
        
        std::cout << "Created " << n_particles << " particles\n";
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"template_simulation.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::TemplateSimulationPlugin)
