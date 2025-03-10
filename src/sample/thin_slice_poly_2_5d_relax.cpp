// load_thin_slice_poly_2_5d_relax.cpp
#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <random>
#include <algorithm> // for std::lower_bound

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "exception.hpp"
#include "density_relaxation.hpp"
namespace sph
{
    void load_thin_slice_poly_2_5d_relax(std::shared_ptr<Simulation> sim,
                                         std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("thin_slice_poly_2_5d_relax requires DIM == 3.");
#endif

        // Polytropic + disk parameters
        const real gamma = 5.0 / 3.0;
        const real n_poly = 1.5; // gamma=1+1/n => n=1.5 => gamma=5/3
        const real K = 1.0;      // polytropic constant
        const real Sigma0 = 1.0; // central surface density
        const real z_max = 0.0;  // strictly razor-thin => z=0

        // 1) Load the CSV from File #1
        loadLaneEmdenTableFromCSV("./sample/thin_slice_poly_2_5d/adiabatic_razor_thin_disk_sigma.csv");
        // Now laneEmden_x and laneEmden_theta store R, Σ(R).
        // We rename them to avoid confusion with the spherical Lane–Emden usage:
        //   laneEmden_x[i] = R_i,
        //   laneEmden_theta[i] = Σ(R_i).
        // Let's interpret them as "physical" R, not dimensionless.

        // 2) The maximum radius is the last entry
        const real R_fluid = laneEmden_x.back();
        param->R_fluid = R_fluid;
        param->z_max = z_max;
        param->alpha_scaling = 1.0; // If the CSV is already in physical units, alpha=1

        // 3) Compute total mass by integrating 2π ∫0^R Σ(R') R' dR'
        std::vector<real> M_cum(laneEmden_x.size(), 0.0);
        for (size_t i = 1; i < laneEmden_x.size(); i++)
        {
            real R0 = laneEmden_x[i - 1];
            real R1 = laneEmden_x[i];
            real Sigma0_ = laneEmden_theta[i - 1];
            real Sigma1_ = laneEmden_theta[i];
            // trapezoid
            real dM = 0.5 * (R0 * Sigma0_ + R1 * Sigma1_) * (R1 - R0) * 2.0 * M_PI;
            M_cum[i] = M_cum[i - 1] + dM;
        }
        real M_total = M_cum.back();

        // 4) Particle sampling
        const int N_total = 10000; // choose resolution
        const real mpp = M_total / static_cast<real>(N_total);

        auto &particles = sim->get_particles();
        particles.clear();
        particles.reserve(N_total);

        std::mt19937 gen(42);
        std::uniform_real_distribution<real> dist(0.0, 1.0);

        int pid = 0;
        for (int i = 0; i < N_total; i++)
        {
            real u = (i + 0.5) / static_cast<real>(N_total);
            real M_target = u * M_total;

            // find R s.t. M_cum(R) ~ M_target
            auto it = std::lower_bound(M_cum.begin(), M_cum.end(), M_target);
            size_t idx = std::distance(M_cum.begin(), it);
            real R_star;
            if (idx == 0)
                R_star = laneEmden_x[0];
            else if (idx == M_cum.size())
                R_star = laneEmden_x.back();
            else
            {
                real M0 = M_cum[idx - 1], M1 = M_cum[idx];
                real R0 = laneEmden_x[idx - 1], R1 = laneEmden_x[idx];
                real frac = (M_target - M0) / (M1 - M0);
                R_star = R0 + frac * (R1 - R0);
            }
            // random azimuth
            real phi = 2.0 * M_PI * dist(gen);
            real x = R_star * std::cos(phi);
            real y = R_star * std::sin(phi);
            real z = 0.0; // razor-thin

            // find Σ(R_star) by interpolation or table lookup
            // for simplicity, just do nearest
            // better: do linear interpolation
            // Here let's do a quick linear interpolation:
            real Sigma_val = 0.0;
            if (idx == 0)
                Sigma_val = laneEmden_theta[0];
            else if (idx == laneEmden_x.size())
                Sigma_val = laneEmden_theta.back();
            else
            {
                real R0 = laneEmden_x[idx - 1], R1 = laneEmden_x[idx];
                real S0 = laneEmden_theta[idx - 1], S1 = laneEmden_theta[idx];
                real frac = (R_star - R0) / (R1 - R0);
                Sigma_val = S0 + frac * (S1 - S0);
            }

            // polytropic P = K * Σ^(5/3)
            real pres = K * std::pow(Sigma_val, gamma);
            // volumetric "dens" in the code => we are storing Σ as "dens"
            // if your code truly wants 3D density, you must handle thickness or bridging
            // For demonstration, we just store Σ in p.dens
            real dens = Sigma_val;

            // internal energy
            real ene = 0.0;
            if (dens > 0.0)
                ene = pres / ((gamma - 1.0) * dens);

            SPHParticle pp;
            pp.pos[0] = x;
            pp.pos[1] = y;
            pp.pos[2] = z;
            pp.vel[0] = 0.0;
            pp.vel[1] = 0.0;
            pp.vel[2] = 0.0;
            pp.mass = mpp;
            pp.dens = dens; // storing Σ as "dens" for convenience
            pp.pres = pres;
            pp.ene = ene;
            pp.id = pid++;
            pp.is_wall = false;
            particles.push_back(pp);
        }

        sim->set_particle_num(N_total);
        std::cout << "[load_thin_slice_poly_2_5d_relax] Placed " << N_total
                  << " particles, total mass=" << M_total << "\n";
    }

    REGISTER_SAMPLE("thin_slice_poly_2_5d_relax", load_thin_slice_poly_2_5d_relax);
} // namespace sph
