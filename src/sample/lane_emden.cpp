#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include "lane_emden.hpp"
namespace sph
{

    //-------------------------------------------------------------------
    // load_lane_emden: Sets up particle properties using the Lane–Emden solution.
    //-------------------------------------------------------------------
    void load_lane_emden(std::shared_ptr<sph::Simulation> sim,
                         std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM == 3
        // 1) Physical and Lane–Emden parameters.
        const real R_phys = 3.0;     // Cloud radius in pc.
        const real M_total = 1000.0; // Total mass in solar masses.
        const real xi1 = 3.65375;    // First zero of θ(ξ) for n = 3/2.
        const real alpha = R_phys / xi1;
        const real omega_n = 2.714; // Precomputed.
        const real rho_c = M_total / (4.0 * M_PI * std::pow(alpha, 3) * omega_n);
        const real n_poly = 1.5;
        const real gamma = 5.0 / 3.0; // Use the configurable gamma.
        const real G = param->gravity.constant;

        const real K = (4.0 * M_PI * G * std::pow(alpha, 2) * std::pow(rho_c, 1.0 / 3.0)) / (n_poly + 1.0);
        param->boundary_radius = R_phys;

        // 2) Create Particle Positions.
        int N = 20; // Grid resolution.
        auto &p = sim->get_particles();
        p.clear();
        p.reserve(N * N * N);
        const real dxi = 2.0 / N;
        std::cout << "Initializing particles..." << std::endl;
        int totalIterations = N * N * N;
        int counter = 0;
        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                for (int k = 0; k < N; k++)
                {
                    counter++;
                    if (counter % 100 == 0 || counter == totalIterations)
                    {
                        int progress = static_cast<int>(100.0 * counter / totalIterations);
                        std::cout << "\rProgress: " << progress << "% complete" << std::flush;
                    }
                    vec_t r_uniform = {
                        (i + 0.5) * dxi - 1.0,
                        (j + 0.5) * dxi - 1.0,
                        (k + 0.5) * dxi - 1.0};
                    const real ru_mag = std::abs(r_uniform);
                    if (ru_mag > 1.0)
                        continue;
                    // Use r^3 distribution for a uniform sphere.
                    real u = ru_mag * ru_mag * ru_mag;
                    real xi = invertCumulativeMass(u);
                    real r_phys = alpha * xi;
                    vec_t pos = r_uniform;
                    if (ru_mag > 0.0)
                        pos *= (r_phys / ru_mag);
                    else
                        pos = {0.0, 0.0, 0.0};
                    SPHParticle p_i;
                    p_i.pos = pos;
                    p.emplace_back(p_i);
                }
            }
        }
        std::cout << "\rProgress: 100% complete" << std::endl;
        std::cout << "Total number of particles: " << p.size() << std::endl;

        // 3) Fill in Particle Properties.
        const size_t total = p.size();
        const real particle_mass = M_total / total;
        for (size_t idx = 0; idx < total; ++idx)
        {
            auto &pp = p[idx];
            pp.vel = 0.0;
            pp.mass = particle_mass;
            real xi = std::abs(pp.pos) / alpha;
            real theta_val = getTheta(xi);
            pp.dens = rho_c * std::pow(theta_val, n_poly);
            real pres = K * std::pow(pp.dens, 1.0 + 1.0 / n_poly);
            pp.pres = pres;
            pp.ene = pres / ((gamma - 1.0) * pp.dens);
            pp.id = static_cast<int>(idx);
            pp.sml = std::cbrt(pp.mass / pp.dens);
        }
        sim->set_particle_num(static_cast<int>(p.size()));
        outputSimulationCSV(sim->get_particles());

#endif
    }

    REGISTER_SAMPLE("lane_emden", load_lane_emden);

} // end namespace sph
