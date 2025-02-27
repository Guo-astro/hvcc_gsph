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
    void load_khi(std::shared_ptr<sph::Simulation> sim,
                  std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM != 2
        THROW_ERROR("DIM != 2 for KHI");
#endif

        int N = 50; // or some default
        const int num = N * N * 3 / 4;
        const real dx = 1.0 / N;
        const real mass = 1.5 / num;
        const real gamma = param->physics.gamma;

        std::vector<SPHParticle> p(num);

        real x = dx * 0.5;
        real y = dx * 0.5;
        int region = 1;
        bool odd = true;

        auto vy = [](real xx, real yy)
        {
            constexpr real sigma2_inv = 2 / (0.05 * 0.05);
            return (real)(0.1 * std::sin(4.0 * M_PI * xx) * (std::exp(-sqr(yy - 0.25) * 0.5 * sigma2_inv) + std::exp(-sqr(yy - 0.75) * 0.5 * sigma2_inv)));
        };

        for (int i = 0; i < num; ++i)
        {
            auto &p_i = p[i];
            p_i.pos[0] = x;
            p_i.pos[1] = y;
            p_i.vel[0] = (region == 1 ? -0.5 : 0.5);
            p_i.vel[1] = vy(x, y);
            p_i.mass = mass;
            p_i.dens = (real)region;
            p_i.pres = 2.5;
            p_i.ene = p_i.pres / ((gamma - 1.0) * p_i.dens);
            p_i.id = i;

            // Step x depending on region
            x += (region == 1 ? 2.0 * dx : dx);

            if (x > 1.0)
            {
                y += dx;
                // switch region if inside 0.25 < y < 0.75
                if (y > 0.25 && y < 0.75)
                    region = 2;
                else
                    region = 1;

                // shift x pattern
                if (region == 1)
                {
                    if (odd)
                    {
                        odd = false;
                        x = dx * 1.5;
                    }
                    else
                    {
                        odd = true;
                        x = dx * 0.5;
                    }
                }
                else
                {
                    x = dx * 0.5;
                }
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(num);
    }
    REGISTER_SAMPLE("khi", load_khi);

} // end anon
