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
    void load_evrard(std::shared_ptr<sph::Simulation> sim,
                     std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM == 3

        // 1) Decide on N. For example, you can default it or read a field from param-> ...
        int N = 20; // example default
        // If you wanted to store an integer inside param->someField, you could do so.

        auto &p = sim->get_particles();
        p.clear();
        p.reserve(N * N * N);

        const real dx = 2.0 / N;

        // 2) Create positions inside a sphere of radius 1
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                for (int k = 0; k < N; k++)
                {
                    vec_t r = {
                        (i + 0.5) * dx - 1.0,
                        (j + 0.5) * dx - 1.0,
                        (k + 0.5) * dx - 1.0};
                    const real r_0 = std::abs(r);
                    if (r_0 > 1.0)
                        continue;

                    // rescale
                    if (r_0 > 0.0)
                    {
                        real r_abs = std::pow(r_0, 1.5);
                        r *= (r_abs / r_0);
                    }

                    SPHParticle p_i;
                    p_i.pos = r;
                    p.emplace_back(p_i);
                }
            }
        }

        // 3) Fill mass, density, etc.
        const size_t total = p.size();
        const real mass = 1.0 / total;
        const real gamma = param->physics.gamma;
        const real G = param->gravity.constant;
        const real u = 0.05 * G;

        for (size_t idx = 0; idx < total; ++idx)
        {
            auto &pp = p[idx];
            pp.vel = 0.0;
            pp.mass = mass;
            // approximate dens
            real rr = std::abs(pp.pos);
            pp.dens = (rr == 0.0 ? 1.0 : 1.0 / (2.0 * M_PI * rr));
            pp.ene = u;
            pp.pres = (gamma - 1.0) * pp.dens * pp.ene;
            pp.id = (int)idx;
        }

        // 4) Store into sim
        sim->set_particle_num((int)p.size());
#endif
    }

    REGISTER_SAMPLE("evrard", load_evrard);

} // end anonymous namespace
