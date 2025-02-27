#include <random>
#include <vector>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{
    void load_pairing(std::shared_ptr<sph::Simulation> sim,
                      std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM != 2
        THROW_ERROR("DIM != 2 for pairing instability");
#endif

        int N = 50; // default
        const real dx = 1.0 / N;
        const int num = N * N;

        std::vector<SPHParticle> p(num);

        real x = -0.5 + dx * 0.5;
        real y = -0.5 + dx * 0.5;
        const real mass = 1.0 / num;
        const real gamma = param->physics.gamma;

        std::mt19937 engine(1);
        std::uniform_real_distribution<real> dist(-dx * 0.05, dx * 0.05);

        for (int i = 0; i < num; ++i)
        {
            auto &p_i = p[i];
            p_i.pos[0] = x + dist(engine);
            p_i.pos[1] = y + dist(engine);
            p_i.vel = 0.0;
            p_i.dens = 1.0;
            p_i.pres = 1.0;
            p_i.mass = mass;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;

            x += dx;
            if (x > 0.5)
            {
                x = -0.5 + dx * 0.5;
                y += dx;
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(num);
    }
    REGISTER_SAMPLE("pairing_instability", load_pairing);

}
