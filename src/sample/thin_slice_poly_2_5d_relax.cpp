#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <random>
#include <algorithm>

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "exception.hpp"
#include "relaxation/density_relaxation.hpp"

namespace sph
{
    void load_thin_slice_poly_2_5d_relax(std::shared_ptr<Simulation> sim,
                                         std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("thin_slice_poly_2_5d_relax requires DIM == 3.");
#endif

        const real gamma = 5.0 / 3.0;
        const real K = 1.0;

        loadLaneEmdenTableFromCSV("./sample/thin_slice_poly_2_5d/adiabatic_razor_thin_disk_sigma.csv");
        const real R_fluid = laneEmden_x.back() * 0.89;
        param->R_fluid = R_fluid;
        param->h_z = 0.01 * R_fluid; // small finite thickness, e.g., 5% of disk radius
        WRITE_LOG << "R_fluid = " << R_fluid << " ,h_z = " << param->h_z;
        param->alpha_scaling = 1.0;

        std::vector<real> M_cum(laneEmden_x.size(), 0.0);
        for (size_t i = 1; i < laneEmden_x.size(); i++)
        {
            real R0 = laneEmden_x[i - 1];
            real R1 = laneEmden_x[i];
            real Sigma0_ = laneEmden_theta[i - 1];
            real Sigma1_ = laneEmden_theta[i];
            real dM = 0.5 * (R0 * Sigma0_ + R1 * Sigma1_) * (R1 - R0) * 2.0 * M_PI;
            M_cum[i] = M_cum[i - 1] + dM;
        }
        real M_total = M_cum.back() * 0.8;

        const int N_total = 10000;
        const real mpp = M_total / static_cast<real>(N_total);

        auto &particles = sim->get_particles();
        particles.clear();
        particles.reserve(N_total);

        std::mt19937 gen(42);
        std::uniform_real_distribution<real> dist(0.0, 1.0);
        std::normal_distribution<real> gaussian_z(0.0, param->h_z);

        int pid = 0;
        for (int i = 0; i < N_total; i++)
        {
            real u = (i + 0.5) / static_cast<real>(N_total);
            real M_target = u * M_total;

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

            real phi = 2.0 * M_PI * dist(gen);
            real x = R_star * std::cos(phi);
            real y = R_star * std::sin(phi);
            real z = gaussian_z(gen);

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

            real dens = Sigma_val / (sqrt(2.0 * M_PI) * param->h_z) * std::exp(-0.5 * (z * z) / (param->h_z * param->h_z));
            real pres = K * std::pow(dens, gamma);
            real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;

            SPHParticle pp;
            pp.pos = {x, y, z};
            pp.vel = vec_t{0.0, 0.0, 0.0};
            pp.mass = mpp;
            pp.dens = dens;
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
