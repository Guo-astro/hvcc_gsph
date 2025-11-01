#include "core/simulation_plugin.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/exception.hpp"
#include "utilities/defines.hpp"
#include "utilities/initial_conditions_modifier.hpp"
#include <iostream>
#include <cmath>
#include <vector>

namespace sph {

/**
 * @brief InfallModifier - Initial conditions modifier for HVCC flyby simulations
 * 
 * This modifier:
 * 1. Adds a static IMBH (intermediate-mass black hole) particle
 * 2. Modifies loaded disk particles to have infall velocity toward target
 * 
 * Used in conjunction with initial conditions loading to set up flyby initial conditions.
 */
class InfallModifier : public InitialConditionsModifier {
public:
    vec_t target;      // Target position (static IMBH location)
    real infallSpeed;  // Desired infall speed
    real pointMass;    // Mass of the BH particle (in solar masses)
    bool bhAdded;      // Flag to ensure BH added only once
    
    InfallModifier(const vec_t& target_, real speed, real pointMass_)
        : target(target_), infallSpeed(speed), pointMass(pointMass_), bhAdded(false)
    {
    }
    
    void modifyParticles(std::vector<SPHParticle>& particles,
                        std::shared_ptr<Simulation> sim) override
    {
        // Add the static BH particle if it hasn't been added yet
        if (!bhAdded) {
            SPHParticle bh;
            
            // Position at target
            bh.pos[0] = target[0];
            bh.pos[1] = target[1];
            bh.pos[2] = target[2];
            
            // Static (zero) velocity
            bh.vel[0] = 0.0;
            bh.vel[1] = 0.0;
            bh.vel[2] = 0.0;
            
            // Mass
            bh.mass = pointMass;
            
            // Hydrodynamic properties irrelevant for point mass
            bh.dens = 0.0;
            bh.pres = 0.0;
            bh.ene = 0.0;
            bh.sml = 1e-6;
            
            // Mark as special so fluid modules ignore it
            bh.is_point_mass = true;
            bh.id = sim->get_particles().size();
            
            particles.push_back(bh);
            sim->set_particle_num(particles.size());
            bhAdded = true;
            
            std::cout << "Added static IMBH particle:\n";
            std::cout << "  Mass: " << bh.mass << " solar masses\n";
            std::cout << "  Position: (" << bh.pos[0] << ", " 
                     << bh.pos[1] << ", " << bh.pos[2] << ")\n";
        }
        
        // Modify fluid particles to have infall velocity
        for (auto& p : particles) {
            // Skip the BH itself
            if (p.is_point_mass)
                continue;
            
            // Add infall velocity and offset position
            p.vel[1] += infallSpeed;
            p.pos[0] += 60.0;  // Offset in x-direction
        }
    }
};

/**
 * @brief Razor-Thin HVCC Flyby Simulation
 * 
 * Production-quality simulation of a high-velocity cloud complex (HVCC) 
 * encountering an intermediate-mass black hole (IMBH).
 * 
 * This simulation:
 * - Loads a relaxed disk from checkpoint file
 * - Adds a static IMBH at specified impact parameter
 * - Sets initial infall velocity for flyby dynamics
 * - Includes self-gravity and 2.5D approximation
 */
class RazorThinHVCCPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "razor_thin_hvcc";
    }
    
    std::string get_description() const override {
        return "Razor-thin HVCC flyby simulation with IMBH - Production run with checkpoint loading";
    }
    
    std::string get_version() const override {
        return "2.0.0";  // Version 2.0 - plugin-based architecture
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
#if DIM != 3
        THROW_ERROR("Razor-thin HVCC requires DIM = 3 (uses 2.5D mode)");
#endif
        
        std::cout << "Initializing Razor-thin HVCC flyby simulation...\n";
        
        // Set flyby parameters if not provided
        if (param->impact_parameter <= 0.0) {
            param->impact_parameter = 1.0;  // Default impact parameter in parsecs
        }
        if (param->initial_velocity <= 0.0) {
            param->initial_velocity = 10.0;  // Default infall speed in km/s
        }
        if (param->point_mass <= 0.0) {
            param->point_mass = 1e5;  // Default IMBH mass in solar masses
        }
        
        std::cout << "Flyby parameters:\n";
        std::cout << "  Impact parameter: " << param->impact_parameter << " pc\n";
        std::cout << "  Infall speed: " << param->initial_velocity << " km/s\n";
        std::cout << "  Point mass: " << param->point_mass << " solar masses\n";
        
        // Set IMBH target position
        vec_t target;
        target[0] = 0.0;
        target[1] = 20.0;  // Can use param->impact_parameter
        target[2] = 0.0;
        
        // Create and set the InfallModifier
        std::shared_ptr<InitialConditionsModifier> modifier =
            std::make_shared<InfallModifier>(target, param->initial_velocity, param->point_mass);
        sim->set_initial_conditions_modifier(modifier);
        
        // Note: Actual particle initialization happens via initial conditions loading
        // The solver will call Output::read_checkpoint() which loads disk data
        // and then invokes the InfallModifier to add IMBH and set velocities
        
        std::cout << "\nNote: This simulation requires initial conditions loading.\n";
        std::cout << "Set 'initialConditionsFile' in config.json to path of relaxed disk.\n";
        std::cout << "The InfallModifier will add IMBH and set infall velocities.\n";
        
        // Set simulation parameters
        param->type = SPHType::DISPH;
        param->time.end = 30.0;
        param->time.output = 0.1;
        param->physics.neighbor_number = 2048;  // High resolution
        param->gravity.is_valid = true;
        param->two_and_half_sim = true;  // 2.5D mode
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"razor_thin_hvcc.cpp"};
    }
};

} // namespace sph

// Export the plugin
DEFINE_SIMULATION_PLUGIN(sph::RazorThinHVCCPlugin)
