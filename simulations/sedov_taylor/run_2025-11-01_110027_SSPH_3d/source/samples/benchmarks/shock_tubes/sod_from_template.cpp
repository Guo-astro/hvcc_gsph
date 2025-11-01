/**
 * @file sod_from_template.cpp
 * @brief Example: Sod shock tube using ShockTubeTemplate
 * 
 * This demonstrates how to use the shock tube template to quickly
 * create a new shock tube simulation with minimal code.
 * 
 * Compare this file to the full implementation in the legacy samples
 * to see the reduction in boilerplate code.
 */

#include "samples/templates/shock_tube_template.hpp"
#include "core/sample_registry.hpp"

namespace {

/**
 * @brief Sod shock tube (standard test problem)
 * 
 * Initial conditions:
 * - Left state:  ρ=1.0, P=1.0, v=0.0
 * - Right state: ρ=0.125, P=0.1, v=0.0
 * - Adiabatic index: γ=1.4
 * - Discontinuity at x=0
 */
class SodShockTubeTemplate : public sph::ShockTubeTemplate<SodShockTubeTemplate> {
protected:
    void set_left_state(sph::real& dens, sph::real& pres, sph::real& vel) override {
        dens = 1.0;
        pres = 1.0;
        vel = 0.0;
    }
    
    void set_right_state(sph::real& dens, sph::real& pres, sph::real& vel) override {
        dens = 0.125;
        pres = 0.1;
        vel = 0.0;
    }

public:
    SodShockTubeTemplate() {
        // Customize default parameters if needed
        n_particles_ = 400;
        x_min_ = -0.5;
        x_max_ = 0.5;
        x_discontinuity_ = 0.0;
        gamma_ = 1.4;
    }
};

// Loader function that creates the template instance
void load_sod_template(sph::Simulation* sim, sph::SPHParameters* param) {
    SodShockTubeTemplate loader;
    loader.load(sim, param);
}

REGISTER_SAMPLE("sod_template", load_sod_template);

}  // anonymous namespace
