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
    void load_hydrostatic(std::shared_ptr<sph::Simulation> sim,
                          std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM != 2
        THROW_ERROR("DIM != 2 for hydrostatic test");
#endif

        int N = 32; // example default
        const real dx1 = 0.5 / N;
        const real dx2 = dx1 * 2.0;
        const real mass = 1.0 / (N * N);
        const real gamma = param->physics.gamma;

        std::vector<SPHParticle> p;
        p.reserve(N * N);
        int id = 0;

        // dense region
        real x = -0.25 + dx1 * 0.5;
        real y = -0.25 + dx1 * 0.5;
        while (y < 0.25)
        {
            SPHParticle p_i;
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            p_i.mass = mass;
            p_i.dens = 4.0;
            p_i.pres = 2.5;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = id++;

            p.push_back(p_i);

            x += dx1;
            if (x > 0.25)
            {
                x = -0.25 + dx1 * 0.5;
                y += dx1;
            }
        }

        // ambient region
        x = -0.5 + dx2 * 0.5;
        y = -0.5 + dx2 * 0.5;
        while (y < 0.5)
        {
            SPHParticle p_i;
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            p_i.mass = mass;
            p_i.dens = 1.0;
            p_i.pres = 2.5;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = id++;
            p.push_back(p_i);

            do
            {
                x += dx2;
                if (x > 0.5)
                {
                    x = -0.5 + dx2 * 0.5;
                    y += dx2;
                }
            } while (x > -0.25 && x < 0.25 && y > -0.25 && y < 0.25);
        }

        sim->get_particles() = p;
        sim->set_particle_num((int)p.size());
    }
    REGISTER_SAMPLE("hydrostatic", load_hydrostatic);

} // end anon
