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
    void load_shock_tube(std::shared_ptr<sph::Simulation> sim,
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
            // Initialize volume element for DISPH: V = m/ρ
            p_i.volume = p_i.mass / p_i.dens;

            x += dx;
            if (x > 0.0 && left)
            {
                x = 0.0 + dx_r * 0.5;
                dx = dx_r;
                dens = 0.125;  // Standard Sod problem: ρ_R = 0.125
                pres = 0.1;    // Standard Sod problem: P_R = 0.1
                left = false;
            }
        }

        sim->set_particles(p);
        sim->set_particle_num(num);
        
        // Set simulation parameters
        param->type = SPHType::DISPH;  // Use DISPH algorithm
        param->time.end = 0.2;  // End time for shock tube evolution
        param->time.output = 0.02;  // Output every 0.02 time units
        param->cfl.sound = 0.3;  // CFL number for timestep
        param->physics.neighbor_number = 50;  // Target number of neighbors
    }
    
    REGISTER_SAMPLE("shock_tube", load_shock_tube);

} // end anonymous
