/**
 * @file keplerian_disk_template.cpp
 * @brief Example: Keplerian disk using DiskTemplate
 * 
 * This demonstrates how to use the disk template to create a
 * 2D Keplerian disk simulation with minimal code.
 */

#include "samples/templates/disk_template.hpp"
#include "core/sample_registry.hpp"

namespace {

/**
 * @brief 2D Keplerian disk with power-law surface density
 * 
 * Profile:
 * - Surface density: Σ(r) = Σ₀ (r/r₀)^(-3/2)
 * - Rotation curve: v_φ(r) = √(GM/r)  [Keplerian]
 * - Temperature: Isothermal
 */
class KeplerianDiskTemplate : public sph::DiskTemplate<KeplerianDiskTemplate> {
protected:
    // Physical constants
    const sph::real G_ = 1.0;          ///< Gravitational constant
    const sph::real M_central_ = 1.0;  ///< Central mass
    const sph::real sigma0_ = 1.0;     ///< Surface density normalization
    const sph::real r0_ = 1.0;         ///< Radius normalization
    
    sph::real surface_density(sph::real r) override {
        // Power-law profile: Σ ∝ r^(-3/2)
        return sigma0_ * std::pow(r / r0_, -1.5);
    }
    
    sph::real rotation_velocity(sph::real r) override {
        // Keplerian rotation
        return std::sqrt(G_ * M_central_ / r);
    }
    
    sph::real compute_pressure(sph::real dens) override {
        // Isothermal equation of state
        // P = ρ c_s^2, where c_s^2 = temperature_
        sph::real c_s_squared = temperature_;
        return dens * c_s_squared;
    }

public:
    KeplerianDiskTemplate() {
        // Configure disk geometry
        r_inner_ = 0.5;
        r_outer_ = 2.0;
        
        // 2D disk (no vertical structure)
        use_3d_ = false;
        
        // Numerical parameters
        n_particles_ = 10000;
        
        // Isothermal sound speed
        temperature_ = 0.01;  // c_s^2
        
        // Total disk mass
        total_mass_ = 0.1;  // Much less than central mass
        
        // Adiabatic index (not used for isothermal, but needed for framework)
        gamma_ = 1.4;
    }
};

// Loader function that creates the template instance
void load_keplerian_disk_template(sph::Simulation* sim, sph::SPHParameters* param) {
    KeplerianDiskTemplate loader;
    loader.load(sim, param);
}

REGISTER_SAMPLE("keplerian_disk_template", load_keplerian_disk_template);

}  // anonymous namespace
