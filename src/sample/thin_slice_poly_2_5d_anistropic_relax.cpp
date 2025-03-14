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
#include "relaxation/density_relaxation.hpp"

namespace sph
{
    void thin_slice_poly_2_5d_anistropic_relax(std::shared_ptr<Simulation> sim,
                                               std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("thin_slice_poly_2_5d_relax requires DIM == 3.");
#endif

        // **Polytropic and disk parameters**
        const real gamma = 5.0 / 3.0; // Polytropic index (γ = 1 + 1/n, n = 1.5)
        const real K = 1.0;           // Polytropic constant
        const real Sigma0 = 1.0;      // Central surface density (reference value)
        const real h_z_frac = 0.01;   // Vertical scale height as fraction of R_fluid (1%)

        // **Step 1: Load surface density profile from CSV**
        // Loads laneEmden_x (R) and laneEmden_theta (Σ(R)) in physical units
        loadLaneEmdenTableFromCSV("./sample/thin_slice_poly_2_5d/adiabatic_razor_thin_disk_sigma.csv");

        // **Step 2: Set disk dimensions**
        const real R_fluid = laneEmden_x.back(); // Maximum radius of the disk
        const real h_z = h_z_frac * R_fluid;     // Actual vertical scale height
        param->R_fluid = R_fluid;
        param->h_z = 0.0;           // Kept as 0.0; adjust if used elsewhere
        param->alpha_scaling = 1.0; // Assumes CSV uses physical units

        // **Step 3: Compute cumulative mass**
        // Total mass via 2π ∫₀^R Σ(R') R' dR' using trapezoidal rule
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
        // Use 80% of total mass to avoid sparse outer regions (configurable?)
        real M_total = M_cum.back() * 0.9;

        // **Step 4: Particle sampling**
        const int N_total = 10000;                             // Number of particles
        const real mpp = M_total / static_cast<real>(N_total); // Mass per particle

        auto &particles = sim->get_particles();
        particles.clear();
        particles.reserve(N_total);

        // Random number generators
        std::mt19937 gen(42);                                // Fixed seed for reproducibility
        std::uniform_real_distribution<real> dist(0.0, 1.0); // For azimuthal angle
        std::normal_distribution<real> gaussian_z(0.0, h_z); // For vertical position

        int pid = 0;
        for (int i = 0; i < N_total; i++)
        {
            // **Radial position via mass sampling**
            real u = (i + 0.5) / static_cast<real>(N_total); // Uniform mass fraction
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

            // **Azimuthal position**
            real phi = 2.0 * M_PI * dist(gen);
            real x = R_star * std::cos(phi);
            real y = R_star * std::sin(phi);

            // **Vertical position**
            real z_star = gaussian_z(gen); // Sample z from Gaussian

            // **Interpolate surface density Σ(R_star)**
            real Sigma_val;
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

            // **Compute volume density at particle position**
            real dens = Sigma_val / (std::sqrt(2.0 * M_PI) * h_z) *
                        std::exp(-0.5 * (z_star * z_star) / (h_z * h_z));

            // **Polytropic pressure and internal energy**
            real pres = K * std::pow(dens, gamma);
            real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;

            // **Set particle properties**
            SPHParticle pp;
            pp.pos[0] = x;
            pp.pos[1] = y;
            pp.pos[2] = z_star; // Use sampled z position
            pp.vel[0] = 0.0;
            pp.vel[1] = 0.0;
            pp.vel[2] = 0.0;
            pp.mass = mpp;
            pp.dens = dens;
            pp.pres = pres;
            pp.ene = ene;
            pp.id = pid++;
            pp.is_wall = false;
            particles.push_back(pp);
        }

        sim->set_particle_num(N_total);
        std::cout << "[thin_slice_poly_2_5d_anistropic_relax] Placed " << N_total
                  << " particles, total mass=" << M_total << "\n";
    }

    REGISTER_SAMPLE("thin_slice_poly_2_5d_anistropic_relax", thin_slice_poly_2_5d_anistropic_relax);
} // namespace sph