#include "core/sample_registry.hpp"
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "utilities/exception.hpp"
#include "utilities/defines.hpp"
#include <cmath>
#include <vector>

namespace sph
{
    void load_sedov_taylor(std::shared_ptr<sph::Simulation> sim,
                           std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("DIM != 3 for sedov_taylor (requires 3D)");
#endif

        // Sedov-Taylor blast wave parameters
        const real E0 = 1.0;      // Total energy of explosion
        const real rho0 = 1.0;    // Background density
        const real gamma = 5.0/3.0;  // Adiabatic index for ideal gas
        
        // Domain: cube [-0.5, 0.5]^3
        const real L = 1.0;
        const int N_1D = 50;  // Particles per dimension
        const int N = N_1D * N_1D * N_1D;  // Total particles
        
        const real dx = L / N_1D;
        const real mass = (L*L*L) * rho0 / N;  // Uniform mass
        
        // Energy injection region (central particle)
        const real r_inject = 2.0 * dx;  // Injection radius
        
        std::vector<SPHParticle> p(N);
        int idx = 0;
        
        // Create uniform lattice
        for (int i = 0; i < N_1D; ++i) {
            for (int j = 0; j < N_1D; ++j) {
                for (int k = 0; k < N_1D; ++k) {
                    auto &p_i = p[idx];
                    
                    // Position: uniform grid
                    p_i.pos[0] = -0.5*L + (i + 0.5) * dx;
                    p_i.pos[1] = -0.5*L + (j + 0.5) * dx;
                    p_i.pos[2] = -0.5*L + (k + 0.5) * dx;
                    
                    // Distance from center
                    real r = std::sqrt(p_i.pos[0]*p_i.pos[0] + 
                                      p_i.pos[1]*p_i.pos[1] + 
                                      p_i.pos[2]*p_i.pos[2]);
                    
                    // Velocity: initially zero
                    p_i.vel[0] = 0.0;
                    p_i.vel[1] = 0.0;
                    p_i.vel[2] = 0.0;
                    
                    // Density and mass: uniform
                    p_i.dens = rho0;
                    p_i.mass = mass;
                    
                    // Pressure and energy: inject energy in central region
                    if (r < r_inject) {
                        // Number of particles in injection region (approx)
                        real V_inject = (4.0/3.0) * M_PI * r_inject * r_inject * r_inject;
                        int N_inject = static_cast<int>(V_inject / (dx*dx*dx));
                        if (N_inject < 1) N_inject = 1;
                        
                        // Distribute energy among central particles
                        real e_inject = E0 / (N_inject * mass);
                        p_i.ene = e_inject;
                        p_i.pres = (gamma - 1.0) * p_i.dens * p_i.ene;
                    } else {
                        // Background: low energy
                        p_i.ene = 1.0e-6;
                        p_i.pres = (gamma - 1.0) * p_i.dens * p_i.ene;
                    }
                    
                    p_i.id = idx;
                    p_i.volume = p_i.mass / p_i.dens;
                    
                    ++idx;
                }
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(N);
        
        // Set simulation parameters
        param->type = SPHType::GSPH;  // Use GSPH for smooth blast wave
        param->time.end = 0.06;  // End time (before shock reaches boundary)
        param->time.output = 0.01;  // Output every 0.01 time units
        param->cfl.sound = 0.3;  // CFL number
        param->physics.neighbor_number = 50;  // Target neighbors
        param->physics.gamma = gamma;
        param->av.alpha = 1.0;  // Artificial viscosity
        param->av.use_balsara_switch = true;
        param->av.use_time_dependent_av = true;
    }
    
    REGISTER_SAMPLE("sedov_taylor", load_sedov_taylor);

} // namespace sph
