#include <cmath>
#include <vector>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{
    void load_purtabation_damping(std::shared_ptr<sph::Simulation> sim,
                                  std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM != 2
        THROW_ERROR("DIM != 2 for KHI");
#endif

        // --- Simulation Parameters ---
        const real Lx = M_PI / 2.0; // Domain half-width: [-Lx, Lx]
        const real Ly = Lx;         // Square domain
        const real A = 0.01 * M_PI; // Perturbation amplitude
        const real kappa = 2.0;     // Wavenumber for perturbation
        const real gamma = param->physics.gamma;
        const real pres = 2.0 / gamma; // Pressure for sound speed c=1 in upper layer

        // --- Particle Spacing ---
        // Lower half (y < 0, density = 1): larger spacing
        const real xstep_lower = 0.2;         // Base step size (adjust as needed)
        const real ystep_lower = xstep_lower; // Same in lower domain
        // Upper half (y >= 0, density = 2): smaller spacing for 2x density
        const real xstep_upper = xstep_lower / std::sqrt(2.0);
        const real ystep_upper = xstep_upper; // Same in upper domain

        std::vector<SPHParticle> particles;

        // --- Helper Function: Create Fluid Particle ---
        auto create_fluid_particle = [&](real x0, real y0, real density)
        {
            SPHParticle p;
            // Perturbation calculations
            real xi_x = 0.0, xi_y = 0.0;
            real exp_ky = std::exp(kappa * y0);
            real exp_mky = std::exp(-kappa * y0);
            real exp_ky_m2kLy = std::exp(kappa * y0 - 2.0 * kappa * Ly);
            real exp_mky_m2kLy = std::exp(-kappa * y0 - 2.0 * kappa * Ly);

            if (y0 < 0.0)
            { // Lower layer
                xi_x = -A * kappa * std::sin(kappa * x0) * (exp_mky + exp_ky_m2kLy);
                xi_y = -A * kappa * std::cos(kappa * x0) * (exp_mky - exp_ky_m2kLy);
            }
            else
            { // Upper layer
                xi_x = -A * kappa * std::sin(kappa * x0) * (-exp_ky - exp_mky_m2kLy);
                xi_y = A * kappa * std::cos(kappa * x0) * (-exp_ky + exp_mky_m2kLy);
            }

            // Set particle properties
            p.pos[0] = x0 + xi_x;
            p.pos[1] = y0 + xi_y;
            p.vel[0] = 0.0;
            p.vel[1] = 0.0;
            p.mass = density * xstep_lower * ystep_lower; // Consistent mass base
            p.dens = density;
            p.pres = pres;
            p.ene = pres / ((gamma - 1.0) * density);
            p.id = particles.size();
            p.is_wall = false;
            particles.push_back(p);
        };

        // --- Fluid Particles: Lower Half (y < 0) ---
        for (real y = -Ly + ystep_lower / 2.0; y < 0.0; y += ystep_lower)
        {
            for (real x = -Lx + xstep_lower / 2.0; x < Lx; x += xstep_lower)
            {
                create_fluid_particle(x, y, 2.0);
            }
        }

        // --- Fluid Particles: Upper Half (y >= 0) ---
        for (real y = 0.0 + ystep_upper / 2.0; y < Ly; y += ystep_upper)
        {
            for (real x = -Lx + xstep_upper / 2.0; x < Lx; x += xstep_upper)
            {
                create_fluid_particle(x, y, 1.0);
            }
        }

        // // --- Wall Particles ---
        // const int num_wall_layers = 3;
        // const real dx_wall = xstep_lower; // Wall spacing based on lower domain

        // // Helper function to create a wall particle
        // auto create_wall = [&](real pos_x, real pos_y, real dens)
        // {
        //     SPHParticle wall;
        //     wall.pos[0] = pos_x;
        //     wall.pos[1] = pos_y;
        //     wall.vel[0] = 0.0;
        //     wall.vel[1] = 0.0;
        //     wall.dens = dens;
        //     wall.pres = pres;
        //     wall.mass = dens * dx_wall * dx_wall;
        //     wall.ene = pres / ((gamma - 1.0) * dens);
        //     wall.id = particles.size();
        //     wall.is_wall = true;
        //     particles.push_back(wall);
        // };

        // // Left wall (x < -Lx)
        // for (int i = 0; i < num_wall_layers; ++i)
        // {
        //     real wall_x = -Lx - (i + 0.5) * dx_wall;
        //     for (real y = -Ly; y < Ly; y += dx_wall)
        //     {
        //         real dens = (y < 0.0) ? 1.0 : 2.0;
        //         create_wall(wall_x, y, dens);
        //     }
        // }

        // // Right wall (x > Lx)
        // for (int i = 0; i < num_wall_layers; ++i)
        // {
        //     real wall_x = Lx + (i + 0.5) * dx_wall;
        //     for (real y = -Ly; y < Ly; y += dx_wall)
        //     {
        //         real dens = (y < 0.0) ? 1.0 : 2.0;
        //         create_wall(wall_x, y, dens);
        //     }
        // }

        // // Bottom wall (y < -Ly)
        // for (int i = 0; i < num_wall_layers; ++i)
        // {
        //     real wall_y = -Ly - (i + 0.5) * dx_wall;
        //     for (real x = -Lx - num_wall_layers * dx_wall; x < Lx + num_wall_layers * dx_wall; x += dx_wall)
        //     {
        //         create_wall(x, wall_y, 1.0); // Lower half density
        //     }
        // }

        // // Top wall (y > Ly)
        // for (int i = 0; i < num_wall_layers; ++i)
        // {
        //     real wall_y = Ly + (i + 0.5) * dx_wall;
        //     for (real x = -Lx - num_wall_layers * dx_wall; x < Lx + num_wall_layers * dx_wall; x += dx_wall)
        //     {
        //         create_wall(x, wall_y, 2.0); // Upper half density
        //     }
        // }

        // --- Finalize Simulation ---
        sim->set_particles(particles);
        sim->set_particle_num(static_cast<int>(particles.size()));
    }
    REGISTER_SAMPLE("purtabation_damping", load_purtabation_damping);
} // namespace sph