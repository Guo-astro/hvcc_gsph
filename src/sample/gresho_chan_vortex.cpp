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
    inline real vortex_velocity(real r)
    {
        if (r < 0.2)
            return 5.0 * r;
        else if (r < 0.4)
            return 2.0 - 5.0 * r;
        else
            return 0.0;
    }

    inline real vortex_pressure(real r)
    {
        if (r < 0.2)
            return 5.0 + 12.5 * r * r;
        else if (r < 0.4)
            return 9.0 + 12.5 * r * r - 20.0 * r + 4.0 * std::log(5.0 * r);
        else
            return 3.0 + 4.0 * std::log(2.0);
    }

    void load_gresho(std::shared_ptr<sph::Simulation> sim,
                     std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM == 2

        // e.g. default N=64
        int N = 64;

        auto &p = sim->get_particles();
        p.clear();
        p.resize(N * N);

        const real dx = 1.0 / N;
        real x = -0.5 + dx * 0.5;
        real y = -0.5 + dx * 0.5;

        const real mass = 1.0 / (N * N);
        const real gamma = param->physics.gamma;

        for (int i = 0; i < (N * N); ++i)
        {
            auto &pp = p[i];
            pp.pos[0] = x;
            pp.pos[1] = y;
            const real r = std::abs(pp.pos);
            const real vel = vortex_velocity(r);

            // direction
            vec_t dir(-y, x);
            real rr = (r == 0.0 ? 1.0 : r);
            dir /= rr;

            pp.vel = dir * vel;
            pp.dens = 1.0;
            pp.pres = vortex_pressure(r);
            pp.mass = mass;
            pp.ene = pp.pres / ((gamma - 1.0) * pp.dens);
            pp.id = i;

            // shift x,y in row-major
            x += dx;
            if (x > 0.5)
            {
                x = -0.5 + dx * 0.5;
                y += dx;
            }
        }

        sim->set_particle_num(N * N);
#endif
    }
    REGISTER_SAMPLE("gresho_chan_vortex", load_gresho);

} // end anonymous
