// src/sample/shock_tube_strong_shock.cpp
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include <cmath>
#include <vector>

namespace sph
{

    void load_vacuum_test(std::shared_ptr<Simulation> sim,
                          std::shared_ptr<SPHParameters> param)
    {
#if DIM != 1
        THROW_ERROR("DIM != 1 for vacuum_test");
#endif

        // Define the domain boundaries for the fluid region.
        const real L = 4; // fluid region from -L to L
        const real R = L; // fluid region from -L to L

        // Define resolutions for fluid particles on the left and right.
        const real dx_fluid_left = 0.01;  // left state (high pressure)
        const real dx_fluid_right = 0.01; // right state (low pressure)

        // Container for all particles.
        std::vector<SPHParticle> particles;

        // --- Create fluid particles in the domain ---
        // Left fluid: x in [-L, 0)
        for (real x = -L + 0.5 * dx_fluid_left; x < 0.0; x += dx_fluid_left)
        {
            SPHParticle p_i;
            p_i.pos[0] = x;
            p_i.vel[0] = -2.0;
            p_i.dens = 1.0;                 // left state density
            p_i.pres = 0.4;                 // left state pressure (strong shock)
            p_i.mass = 1.0 * dx_fluid_left; // mass = density * dx (in 1D)
            const real gamma = param->physics.gamma;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = particles.size();
            p_i.is_wall = false; // fluid particle
            particles.push_back(p_i);
        }
        // Right fluid: x in [0, L]
        for (real x = 0.0 + 0.5 * dx_fluid_right; x < R; x += dx_fluid_right)
        {
            SPHParticle p_i;
            p_i.pos[0] = x;
            p_i.vel[0] = 2.0;
            p_i.dens = 1.0; // right state density
            p_i.pres = 0.4; // right state pressure
            p_i.mass = 1.0 * dx_fluid_right;
            const real gamma = param->physics.gamma;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = particles.size();
            p_i.is_wall = false;
            particles.push_back(p_i);
        }

        // --- Create ghost (wall) particles outside the domain ---
        // Choose the number of ghost layers and spacing
        // const int num_wall_layers = 128; // e.g., three layers of ghost particles per side

        // Left wall ghost particles: place them to the left of -L.
        // const real dx_wall_left = dx_fluid_left; // use similar spacing as the left fluid
        // for (int i = 0; i < num_wall_layers; ++i)
        // {
        //     SPHParticle wall;
        //     wall.pos[0] = -L - (i + 0.5) * dx_wall_left;
        //     wall.vel[0] = -2.0; // fixed wall
        //     wall.dens = 1.0;    // match left state density
        //     wall.pres = 0.4;    // match left state pressure
        //     wall.mass = 1.0 * dx_wall_left;
        //     const real gamma = param->physics.gamma;
        //     wall.ene = wall.pres / ((gamma - 1.0) * wall.dens);
        //     wall.id = particles.size();
        //     wall.is_wall = true;
        //     particles.push_back(wall);
        // }

        // // Right wall ghost particles: place them to the right of L.
        // const real dx_wall_right = dx_fluid_right; // use similar spacing as right fluid
        // for (int i = 0; i < num_wall_layers; ++i)
        // {
        //     SPHParticle wall;
        //     wall.pos[0] = R + (i + 0.5) * dx_wall_right;
        //     wall.vel[0] = 2.0;
        //     wall.dens = 1.0; // match right state density
        //     wall.pres = 0.4; // match right state pressure
        //     wall.mass = 1.0 * dx_wall_right;
        //     const real gamma = param->physics.gamma;
        //     wall.ene = wall.pres / ((gamma - 1.0) * wall.dens);
        //     wall.id = particles.size();
        //     wall.is_wall = true;
        //     particles.push_back(wall);
        // }

        // Set the combined particle list and count in the simulation.
        sim->set_particles(particles);
        sim->set_particle_num(static_cast<int>(particles.size()));
    }

    REGISTER_SAMPLE("vacuum_test", load_vacuum_test);

} // namespace sph
