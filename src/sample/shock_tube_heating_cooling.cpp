#include "sample_registry.hpp"
#include "solver.hpp"
#include "simulation.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "parameters.hpp"

namespace sph
{
    void shock_tube_heating_cooling(std::shared_ptr<sph::Simulation> sim,
                                    std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM != 1
        THROW_ERROR("DIM != 1 for shock_tube");
#endif

        // default N
        int N = 50;
        const real dx_r = 0.5 / N;
        const real dx_l = dx_r * 0.25;

        const int num = N * 10;
        std::vector<SPHParticle> p(num);

        real x = -0.5 + dx_l * 0.5;
        real dx = dx_l;
        real dens = 1.0;
        real pres = 1.0;
        const real mass = 0.5 / N * 0.25;
        const real gamma = param->physics.gamma;
        bool left = true;

        for (int i = 0; i < num; ++i)
        {
            auto &p_i = p[i];
            p_i.pos[0] = x;
            p_i.vel[0] = 0.0;
            p_i.dens = dens;
            p_i.pres = pres;
            p_i.mass = mass;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;

            x += dx;
            if (x > 0.5 && left)
            {
                x = 0.5 + dx_r * 0.5;
                dx = dx_r;
                dens = 0.25;
                pres = 0.1795;
                left = false;
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(num);
    }
    REGISTER_SAMPLE("shock_tube_heating_cooling", shock_tube_heating_cooling);

} // end anonymous
