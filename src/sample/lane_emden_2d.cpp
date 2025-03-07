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
#include "lane_emden_2d.hpp"

namespace sph
{
    // Function to compute I_n = ∫ θ^n ξ dξ from 0 to ξ1 using the trapezoidal rule
    real compute_In(const std::vector<real> &xi, const std::vector<real> &theta, real n)
    {
        real integral = 0.0;
        for (size_t i = 1; i < xi.size(); ++i)
        {
            real xi_prev = xi[i - 1];
            real xi_curr = xi[i];
            real theta_prev = theta[i - 1];
            real theta_curr = theta[i];
            real integrand_prev = std::pow(theta_prev, n) * xi_prev;
            real integrand_curr = std::pow(theta_curr, n) * xi_curr;
            integral += (integrand_prev + integrand_curr) * (xi_curr - xi_prev) / 2.0;
        }
        return integral;
    }

    //-------------------------------------------------------------------
    // load_lane_emden_2d: Sets up particle properties using the 2D Lane–Emden solution.
    //-------------------------------------------------------------------
    void load_lane_emden_2d(std::shared_ptr<sph::Simulation> sim,
                            std::shared_ptr<sph::SPHParameters> param)
    {
#if DIM == 2
        // 1) Physical and Lane-Emden Parameters for 2D
        const real R_phys = 3.0;                // Disk radius in parsecs
        const real M_total = 1000.0;            // Total mass in solar masses
        const real xi1 = 2.752;                 // First zero of θ(ξ) for n = 3/2 in 2D (approximate)
        const real alpha = R_phys / xi1;        // Scale factor
        const real n_poly = 1.5;                // Polytropic index
        const real gamma = 5.0 / 3.0;           // Adiabatic index (gamma)
        const real G = param->gravity.constant; // Gravitational constant

        // Load the 2D Lane–Emden CSV table
        loadLaneEmdenTableFromCSV_2d("./sample/lane_emden_2d/lane_emden_2d.csv");

        // Retrieve xi and theta vectors from the loaded table
        std::vector<real> xi = laneEmden_x_2d;        // 2D-specific xi values
        std::vector<real> theta = laneEmden_theta_2d; // 2D-specific theta values

        // Compute I_n for n = 1.5
        const real I_n = compute_In(xi, theta, n_poly);
        WRITE_LOG << "Computed I_n = " << I_n;

        const real omega_n = I_n; // Replace hardcoded omega_n with computed I_n
        WRITE_LOG << "Using omega_n = " << omega_n;

        // Central surface density using the computed omega_n
        const real sigma_c = M_total / (2.0 * M_PI * std::pow(alpha, 2) * omega_n);
        WRITE_LOG << "Computed central surface density sigma_c = " << sigma_c;

        // Polytropic constant K for 2D
        const real K = (2.0 * M_PI * G * std::pow(alpha, 2) * std::pow(sigma_c, 2.0 / 3.0)) / (n_poly + 1.0);
        WRITE_LOG << "Computed polytropic constant K = " << K;

        param->boundary_radius = R_phys; // Set simulation boundary

        // 2) Create Particle Positions
        int N = 40; // Grid resolution (N x N in 2D)
        auto &p = sim->get_particles();
        p.clear();
        p.reserve(N * N);         // Reserve space for 2D grid
        const real dxi = 2.0 / N; // Step size in normalized units
        WRITE_LOG << "Initializing particles on a " << N << "x" << N << " grid.";
        int totalIterations = N * N;
        int counter = 0;

        for (int i = 0; i < N; i++)
        {
            for (int j = 0; j < N; j++)
            {
                counter++;
                if (counter % 100 == 0 || counter == totalIterations)
                {
                    int progress = static_cast<int>(100.0 * counter / totalIterations);
                    std::cout << "\rProgress: " << progress << "% complete" << std::flush;
                }
                // Uniform grid position in 2D (normalized coordinates)
                vec_t r_uniform = {
                    (i + 0.5) * dxi - 1.0, // x-coordinate
                    (j + 0.5) * dxi - 1.0, // y-coordinate
                };
                // Radial distance in normalized units
                const real ru_mag = std::sqrt(r_uniform[0] * r_uniform[0] + r_uniform[1] * r_uniform[1]);
                if (ru_mag > 1.0)
                    continue; // Skip particles outside the unit disk

                // Use r^2 distribution for a uniform 2D disk
                real u = ru_mag * ru_mag;
                real xi_val = invertCumulativeMass_2d(u, n_poly); // Map to 2D Lane-Emden coordinate
                real r_phys = alpha * xi_val;                     // Physical radius
                vec_t pos = r_uniform;
                if (ru_mag > 0.0)
                    pos *= (r_phys / ru_mag); // Scale to physical position
                else
                    pos = {0.0, 0.0}; // Center case

                SPHParticle p_i;
                p_i.pos = pos;
                p.emplace_back(p_i);
            }
        }
        std::cout << "\rProgress: 100% complete" << std::endl;
        WRITE_LOG << "Total number of particles created: " << p.size();

        // 3) Fill in Particle Properties
        const size_t total = p.size();
        const real particle_mass = M_total / total; // Mass per particle
        WRITE_LOG << "Particle mass per particle: " << particle_mass;
        for (size_t idx = 0; idx < total; ++idx)
        {
            auto &pp = p[idx];
            pp.vel = 0.0; // Initial velocity is zero
            pp.mass = particle_mass;

            // Compute ξ from physical position
            real xi_val = std::sqrt(pp.pos[0] * pp.pos[0] + pp.pos[1] * pp.pos[1]) / alpha;
            real theta_val = getTheta_2d(xi_val);                  // Use the 2D Lane–Emden function value
            pp.dens = sigma_c * std::pow(theta_val, n_poly);       // Surface density
            real pres = K * std::pow(pp.dens, 1.0 + 1.0 / n_poly); // Pressure
            pp.pres = pres;
            pp.ene = pres / ((gamma - 1.0) * pp.dens); // Internal energy
            pp.id = static_cast<int>(idx);
            pp.sml = 1.2 * std::sqrt(pp.mass / pp.dens); // Adjusted smoothing length

            if (idx == total / 2)
            {
                WRITE_LOG << "Center particle: dens=" << pp.dens << ", pres=" << pp.pres;
            }
        }
        int centerIndex = -1;
        real minRadius = 1e30;
        for (int i = 0; i < (int)p.size(); ++i)
        {
            real r = std::sqrt(p[i].pos[0] * p[i].pos[0] + p[i].pos[1] * p[i].pos[1]);
            if (r < minRadius)
            {
                minRadius = r;
                centerIndex = i;
            }
        }
        if (centerIndex >= 0)
        {
            WRITE_LOG << "Near-true center (idx=" << centerIndex << ", r=" << minRadius
                      << ") dens=" << p[centerIndex].dens
                      << ", pres=" << p[centerIndex].pres;
        }
        sim->set_particle_num(static_cast<int>(p.size()));
        outputSimulationCSV_2d(sim->get_particles()); // Write out debug CSV data for inspection
#endif
    }

    REGISTER_SAMPLE("lane_emden_2d", load_lane_emden_2d);

} // namespace sph