#ifndef DISK_TEMPLATE_HPP
#define DISK_TEMPLATE_HPP

/**
 * @file disk_template.hpp
 * @brief Template for creating disk simulations (2D/3D)
 * 
 * This file provides a CRTP base class for disk simulations with customizable
 * surface density profiles, scale heights, and rotation curves.
 * 
 * Example usage (2D razor-thin disk):
 * 
 * class RazorThinDisk : public DiskTemplate<RazorThinDisk> {
 * protected:
 *     real surface_density(real r) override {
 *         return sigma0_ * std::pow(r / r0_, -1.5);  // Power-law profile
 *     }
 *     
 *     real rotation_velocity(real r) override {
 *         return std::sqrt(G_ * M_central_ / r);     // Keplerian
 *     }
 * };
 * 
 * REGISTER_SAMPLE("razor_thin_disk", RazorThinDisk);
 */

#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/defines.hpp"

#include <cmath>
#include <vector>
#include <random>

/**
 * @brief CRTP base class for disk simulations
 * 
 * @tparam Derived The derived class implementing specific disk profile
 */
template<typename Derived>
class DiskTemplate {
protected:
    // Disk geometry
    real r_inner_ = 0.1;      ///< Inner disk radius
    real r_outer_ = 1.0;      ///< Outer disk radius
    real aspect_ratio_ = 0.05; ///< Scale height / radius (for 3D)
    
    // Physical parameters
    real total_mass_ = 1.0;   ///< Total disk mass
    real temperature_ = 1.0;  ///< Temperature (for pressure calculation)
    real gamma_ = 1.4;        ///< Adiabatic index
    
    // Numerical parameters
    int n_particles_ = 10000; ///< Number of particles
    bool use_3d_ = false;     ///< True for 3D, false for 2D
    
    /**
     * @brief Surface density profile Σ(r)
     * @param r Cylindrical radius
     * @return Surface density
     */
    virtual real surface_density(real r) = 0;
    
    /**
     * @brief Rotation velocity profile v_φ(r)
     * @param r Cylindrical radius
     * @return Azimuthal velocity
     */
    virtual real rotation_velocity(real r) = 0;
    
    /**
     * @brief Optional: Vertical density profile (3D only)
     * @param r Cylindrical radius
     * @param z Height above midplane
     * @return Density ρ(r, z)
     */
    virtual real vertical_density(real r, real z) {
        // Gaussian vertical profile by default
        real H = aspect_ratio_ * r;  // Scale height
        real sigma = surface_density(r);
        return sigma / (std::sqrt(2.0 * M_PI) * H) * 
               std::exp(-0.5 * (z / H) * (z / H));
    }
    
    /**
     * @brief Optional: Radial velocity perturbation
     * @param r Radius
     * @param phi Azimuthal angle
     * @return Radial velocity
     */
    virtual real radial_velocity(real r, real phi) {
        return 0.0;  // No radial flow by default
    }
    
    /**
     * @brief Optional: Vertical velocity perturbation
     * @param r Radius
     * @param z Height
     * @return Vertical velocity
     */
    virtual real vertical_velocity(real r, real z) {
        return 0.0;  // No vertical flow by default
    }
    
    /**
     * @brief Compute pressure from temperature
     * @param dens Density
     * @return Pressure
     */
    virtual real compute_pressure(real dens) {
        // Ideal gas: P = ρ * k_B * T / (μ * m_H)
        // Simplified: P = ρ * c_s^2 where c_s^2 ∝ T
        real c_s = std::sqrt(temperature_);
        return dens * c_s * c_s;
    }

public:
    /**
     * @brief Load the disk simulation
     * @param sim Simulation object
     * @param param Parameters object
     */
    void load(Simulation* sim, SPHParameters* param) {
#if DIM < 2
        std::cerr << "ERROR: Disk template requires DIM >= 2" << std::endl;
        exit(1);
#endif
        
        // Get derived class reference
        Derived& derived = static_cast<Derived&>(*this);
        
        // Random number generator for particle placement
        std::mt19937 rng(12345);
        std::uniform_real_distribution<real> uniform(0.0, 1.0);
        
        // Estimate particle mass
        real pmass = total_mass_ / n_particles_;
        
        // Estimate smoothing length
        real area = M_PI * (r_outer_ * r_outer_ - r_inner_ * r_inner_);
        real particle_area = area / n_particles_;
        real h = 2.0 * std::sqrt(particle_area / M_PI);
        
        // Create particles
        std::vector<Particle> particles;
        particles.reserve(n_particles_);
        
        // Generate particles with density following surface density profile
        int particles_created = 0;
        
        while (particles_created < n_particles_) {
            Particle p;
            
            // Generate random position
            // Sample radius from cumulative mass distribution
            real u = uniform(rng);
            real r = r_inner_ + (r_outer_ - r_inner_) * std::sqrt(u);
            real phi = 2.0 * M_PI * uniform(rng);
            
            // Cartesian coordinates
            p.pos[0] = r * std::cos(phi);
            p.pos[1] = r * std::sin(phi);
            
            real z = 0.0;
            if (use_3d_ && DIM == 3) {
                // Sample vertical position from Gaussian
                real H = aspect_ratio_ * r;
                z = H * std::sqrt(-2.0 * std::log(uniform(rng))) * 
                    std::cos(2.0 * M_PI * uniform(rng));
                p.pos[2] = z;
            }
            
            // Velocities
            real v_phi = derived.rotation_velocity(r);
            real v_r = derived.radial_velocity(r, phi);
            
            p.vel[0] = -v_phi * std::sin(phi) + v_r * std::cos(phi);
            p.vel[1] = v_phi * std::cos(phi) + v_r * std::sin(phi);
            
            if (use_3d_ && DIM == 3) {
                p.vel[2] = derived.vertical_velocity(r, z);
            }
            
            // Thermodynamics
            if (use_3d_ && DIM == 3) {
                p.dens = derived.vertical_density(r, z);
            } else {
                // 2D: column density
                p.dens = derived.surface_density(r);
            }
            
            p.pres = derived.compute_pressure(p.dens);
            p.ene = p.pres / ((gamma_ - 1.0) * p.dens);
            
            // Properties
            p.mass = pmass;
            p.h = h;
            
            particles.push_back(p);
            particles_created++;
        }
        
        // Initialize simulation
        sim->particles = std::move(particles);
        
        // Set parameters
        param->gamma = gamma_;
        
        // Set domain bounds
        real pad = 0.1;
        for (int d = 0; d < DIM; ++d) {
            if (d < 2) {
                // x, y: cover disk
                param->rangeMin[d] = -(r_outer_ + pad);
                param->rangeMax[d] = r_outer_ + pad;
            } else {
                // z: vertical extent
                real H_max = aspect_ratio_ * r_outer_;
                param->rangeMin[d] = -5.0 * H_max;
                param->rangeMax[d] = 5.0 * H_max;
            }
        }
    }
    
    // Virtual destructor
    virtual ~DiskTemplate() = default;
};

#endif // DISK_TEMPLATE_HPP
