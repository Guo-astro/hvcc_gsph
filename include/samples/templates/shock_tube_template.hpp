#ifndef SHOCK_TUBE_TEMPLATE_HPP
#define SHOCK_TUBE_TEMPLATE_HPP

/**
 * @file shock_tube_template.hpp
 * @brief Template for creating shock tube (Riemann problem) simulations
 * 
 * This file provides a CRTP (Curiously Recurring Template Pattern) base class
 * for shock tube simulations. Inherit from this class and implement the
 * set_left_state() and set_right_state() methods to create new shock tube variants.
 * 
 * Example usage:
 * 
 * class SodShockTube : public ShockTubeTemplate<SodShockTube> {
 * protected:
 *     void set_left_state(real& dens, real& pres, real& vel) override {
 *         dens = 1.0;
 *         pres = 1.0;
 *         vel = 0.0;
 *     }
 *     
 *     void set_right_state(real& dens, real& pres, real& vel) override {
 *         dens = 0.125;
 *         pres = 0.1;
 *         vel = 0.0;
 *     }
 * };
 * 
 * REGISTER_SAMPLE("sod_shock_tube", SodShockTube);
 */

#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/defines.hpp"

#include <cmath>
#include <vector>

/**
 * @brief CRTP base class for shock tube simulations
 * 
 * @tparam Derived The derived class implementing specific initial conditions
 */
template<typename Derived>
class ShockTubeTemplate {
protected:
    // Configuration parameters
    real x_min_ = -0.5;       ///< Left boundary
    real x_max_ = 0.5;        ///< Right boundary
    real x_discontinuity_ = 0.0;  ///< Position of initial discontinuity
    
    int n_particles_ = 400;   ///< Total number of particles
    real gamma_ = 1.4;        ///< Adiabatic index
    
    /**
     * @brief Set the state on the left side of discontinuity
     * @param dens Density
     * @param pres Pressure
     * @param vel Velocity (x-component)
     */
    virtual void set_left_state(real& dens, real& pres, real& vel) = 0;
    
    /**
     * @brief Set the state on the right side of discontinuity
     * @param dens Density
     * @param pres Pressure
     * @param vel Velocity (x-component)
     */
    virtual void set_right_state(real& dens, real& pres, real& vel) = 0;
    
    /**
     * @brief Optional: Customize particle spacing
     * @return Particle mass
     */
    virtual real compute_particle_mass(real dens_left, real dens_right) {
        // Equal mass particles by default
        real total_mass = dens_left * (x_discontinuity_ - x_min_) + 
                         dens_right * (x_max_ - x_discontinuity_);
        return total_mass / n_particles_;
    }
    
    /**
     * @brief Optional: Customize smoothing length
     * @param dx Average particle spacing
     * @return Initial smoothing length
     */
    virtual real compute_smoothing_length(real dx) {
        return 2.0 * dx;  // Default: ~2 particle spacings
    }

public:
    /**
     * @brief Load the shock tube simulation
     * @param sim Simulation object to initialize
     * @param param Parameters object to configure
     */
    void load(Simulation* sim, SPHParameters* param) {
#if DIM != 1
        std::cerr << "ERROR: Shock tube template requires DIM=1" << std::endl;
        exit(1);
#endif
        
        // Get derived class reference for CRTP
        Derived& derived = static_cast<Derived&>(*this);
        
        // Get initial states from derived class
        real dens_left, pres_left, vel_left;
        real dens_right, pres_right, vel_right;
        
        derived.set_left_state(dens_left, pres_left, vel_left);
        derived.set_right_state(dens_right, pres_right, vel_right);
        
        // Compute particle properties
        real pmass = derived.compute_particle_mass(dens_left, dens_right);
        
        // Determine particle distribution
        // Equal mass particles means more particles in dense region
        real mass_left = dens_left * (x_discontinuity_ - x_min_);
        real mass_right = dens_right * (x_max_ - x_discontinuity_);
        real total_mass = mass_left + mass_right;
        
        int n_left = static_cast<int>(n_particles_ * mass_left / total_mass);
        int n_right = n_particles_ - n_left;
        
        real dx_left = (x_discontinuity_ - x_min_) / n_left;
        real dx_right = (x_max_ - x_discontinuity_) / n_right;
        
        // Compute smoothing length (use average spacing)
        real dx_avg = (x_max_ - x_min_) / n_particles_;
        real h = derived.compute_smoothing_length(dx_avg);
        
        // Create particles
        std::vector<Particle> particles;
        particles.reserve(n_particles_);
        
        // Left state
        for (int i = 0; i < n_left; ++i) {
            Particle p;
            
            // Position
            p.pos[0] = x_min_ + (i + 0.5) * dx_left;
            for (int d = 1; d < DIM; ++d) {
                p.pos[d] = 0.0;
            }
            
            // Velocity
            p.vel[0] = vel_left;
            for (int d = 1; d < DIM; ++d) {
                p.vel[d] = 0.0;
            }
            
            // Thermodynamics
            p.dens = dens_left;
            p.pres = pres_left;
            p.ene = pres_left / ((gamma_ - 1.0) * dens_left);
            
            // Properties
            p.mass = pmass;
            p.h = h;
            
            particles.push_back(p);
        }
        
        // Right state
        for (int i = 0; i < n_right; ++i) {
            Particle p;
            
            // Position
            p.pos[0] = x_discontinuity_ + (i + 0.5) * dx_right;
            for (int d = 1; d < DIM; ++d) {
                p.pos[d] = 0.0;
            }
            
            // Velocity
            p.vel[0] = vel_right;
            for (int d = 1; d < DIM; ++d) {
                p.vel[d] = 0.0;
            }
            
            // Thermodynamics
            p.dens = dens_right;
            p.pres = pres_right;
            p.ene = pres_right / ((gamma_ - 1.0) * dens_right);
            
            // Properties
            p.mass = pmass;
            p.h = h;
            
            particles.push_back(p);
        }
        
        // Initialize simulation
        sim->particles = std::move(particles);
        
        // Set parameters
        param->gamma = gamma_;
        
        // Set domain bounds (with some padding for periodic boundaries if used)
        for (int d = 0; d < DIM; ++d) {
            if (d == 0) {
                param->rangeMin[d] = x_min_ - 0.1;
                param->rangeMax[d] = x_max_ + 0.1;
            } else {
                param->rangeMin[d] = -0.1;
                param->rangeMax[d] = 0.1;
            }
        }
    }
    
    // Virtual destructor for proper cleanup
    virtual ~ShockTubeTemplate() = default;
};

#endif // SHOCK_TUBE_TEMPLATE_HPP
