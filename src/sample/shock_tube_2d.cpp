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

    void load_shock_tube_2d(std::shared_ptr<Simulation> sim,
                            std::shared_ptr<SPHParameters> param)
    {
#if DIM != 2
        THROW_ERROR("DIM != 2 for 2d shock_tube");
#endif

        // Retrieve gamma from simulation parameters
        const double gamma = param->physics.gamma;

        // Domain limits (example values)
        const double x_min = -1.0;
        const double x_split = 0.0; // vertical discontinuity
        const double x_max = 1.0;
        const double y_min = 0.0;
        const double y_max = 0.5;

        // Resolution parameters:
        // Left region (x < x_split): higher resolution
        const double dx_left = 0.005;
        // Right region (x >= x_split): coarser resolution
        const double dx_right = 0.01;
        // Uniform spacing in y-direction
        const double dy = 0.01;

        std::vector<SPHParticle> particles;
        int id = 0;

        // Loop over y for the entire domain
        for (double y = y_min + 0.5 * dy; y < y_max; y += dy)
        {
            // Left region particles (x from x_min to x_split)
            for (double x = x_min + 0.5 * dx_left; x < x_split; x += dx_left)
            {
                SPHParticle p_i;
                // Set particle positions in 2D
                p_i.pos[0] = x;
                p_i.pos[1] = y;
                // Set velocities to zero
                p_i.vel[0] = 0.0;
                p_i.vel[1] = 0.0;
                // Left state: density and pressure
                double dens = 1.0;
                double pres = 1.0;
                p_i.dens = dens;
                p_i.pres = pres;
                // Compute particle mass as density * cell area
                p_i.mass = dens * dx_left * dy;
                // Compute energy per particle (assuming ideal gas)
                p_i.ene = pres / ((gamma - 1.0) * dens);
                p_i.id = id++;
                particles.push_back(p_i);
            }

            // Right region particles (x from x_split to x_max)
            for (double x = x_split + 0.5 * dx_right; x < x_max; x += dx_right)
            {
                SPHParticle p_i;
                p_i.pos[0] = x;
                p_i.pos[1] = y;
                p_i.vel[0] = 0.0;
                p_i.vel[1] = 0.0;
                // Right state: density and pressure from your simulation initial condition
                double dens = 0.25;
                double pres = 0.1795;
                p_i.dens = dens;
                p_i.pres = pres;
                p_i.mass = dens * dx_right * dy;
                p_i.ene = pres / ((gamma - 1.0) * dens);
                p_i.id = id++;
                particles.push_back(p_i);
            }
        }

        // Set the particles in the simulation object
        sim->set_particles(particles);
        sim->set_particle_num(static_cast<int>(particles.size()));
    }

    REGISTER_SAMPLE("shock_tube_2d", load_shock_tube_2d);

} // namespace sph
