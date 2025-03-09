#include <cmath>
#include <vector>
#include <iostream>
#include <random>

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "lane_emden.hpp"
#include "exception.hpp"

namespace sph
{
    void load_thin_slice_poly_2_5d(std::shared_ptr<Simulation> sim,
                                   std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("thin_slice_poly_2_5d requires DIM == 3.");
#endif

        // Fluid parameters
        const real gamma = 5.0 / 3.0; // Polytrope with γ = 5/3
        const real n_poly = 1.5;      // Polytropic index
        const real R_fluid = 3.0;     // Fluid disk radius
        const real z_max = 0.2;       // Half-thickness (unused since z=0)
        const real M_total = 1000.0;  // Total fluid mass
        const real K = 1.0;           // Polytropic constant
        const real rho_c = 1.0;       // Central surface density

        // Load Lane-Emden solution
        loadLaneEmdenTableFromCSV("./sample/thin_slice_poly_2_5d/lane_emden_2d_3d_gravity.csv");
        const real xi_max = laneEmden_x.back();
        const real theta_max = laneEmden_theta.back();
        if (theta_max > 1e-3)
        {
            std::cerr << "Warning: θ(xi_max=" << xi_max << ") = " << theta_max
                      << " is not near zero; consider adjusting R_fluid or extending the table.\n";
        }
        const real alpha = R_fluid / xi_max; // Radial scaling factor
        param->alpha_scaling = alpha;
        param->R_fluid = R_fluid;
        param->z_max = z_max;

        // Compute cumulative mass M(<xi) using trapezoidal rule
        std::vector<real> M_cum(laneEmden_x.size(), 0.0);
        for (size_t i = 1; i < laneEmden_x.size(); ++i)
        {
            real xi_prev = laneEmden_x[i - 1];
            real xi_curr = laneEmden_x[i];
            real theta_prev = laneEmden_theta[i - 1];
            real theta_curr = laneEmden_theta[i];
            real sigma_prev = std::pow(theta_prev, n_poly);
            real sigma_curr = std::pow(theta_curr, n_poly);
            real dM = 0.5 * (xi_prev * sigma_prev + xi_curr * sigma_curr) * (xi_curr - xi_prev) * 2.0 * M_PI;
            M_cum[i] = M_cum[i - 1] + dM;
        }
        real M_total_lane_emden = M_cum.back();
        // Adjust rho_c to match M_total (optional; here we assume rho_c=1.0 is normalized)
        // real rho_c_adjusted = M_total / (alpha * alpha * M_total_lane_emden);

        // Particle placement parameters
        const int N_total = 2500;                              // Target number of particles (approx 50x50 equivalent)
        const real mpp = M_total / static_cast<real>(N_total); // Mass per particle

        // Initialize particles
        auto &particles = sim->get_particles();
        particles.clear();
        particles.reserve(N_total);

        std::mt19937 gen(42); // Seed for reproducibility
        std::uniform_real_distribution<real> dist(0.0, 1.0);

        int pid = 0;
        for (int i = 0; i < N_total; ++i)
        {
            // Sample cumulative mass fraction
            real u = (i + 0.5) / static_cast<real>(N_total);
            real M_i = u * M_total_lane_emden;

            // Interpolate xi for M_i
            auto it = std::lower_bound(M_cum.begin(), M_cum.end(), M_i);
            size_t idx = std::distance(M_cum.begin(), it);
            real xi;
            if (idx == 0)
                xi = laneEmden_x[0];
            else if (idx == M_cum.size())
                xi = laneEmden_x.back();
            else
            {
                real M0 = M_cum[idx - 1], M1 = M_cum[idx];
                real xi0 = laneEmden_x[idx - 1], xi1 = laneEmden_x[idx];
                real frac = (M_i - M0) / (M1 - M0);
                xi = xi0 + frac * (xi1 - xi0);
            }

            // Convert to physical radius and position
            real r = alpha * xi;
            real theta = 2.0 * M_PI * dist(gen);
            real x = r * std::cos(theta);
            real y = r * std::sin(theta);
            real z = 0.0; // Razor-thin disk

            // Compute polytropic properties
            real thetaVal = getTheta(xi);
            if (thetaVal < 0.0)
                thetaVal = 0.0;
            real dens = rho_c * std::pow(thetaVal, n_poly); // Surface density
            real pres = K * std::pow(dens, 1.0 + 1.0 / n_poly);
            real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;

            // Set particle properties
            SPHParticle pp;
            pp.pos[0] = x;
            pp.pos[1] = y;
            pp.pos[2] = z;
            pp.vel[0] = 0.0;
            pp.vel[1] = 0.0;
            pp.vel[2] = 0.0;
            pp.mass = mpp;
            pp.dens = dens;
            pp.pres = pres;
            pp.ene = ene;
            pp.id = pid++;
            pp.is_wall = false; // All particles are fluid
            particles.push_back(pp);
        }

        // Output diagnostics
        std::cout << "Total: " << N_total << " fluid particles.\n";
        std::cout << "NOTE: Integration should clamp z and vz to 0 for all particles.\n";

        sim->set_particle_num(N_total);
    }

    REGISTER_SAMPLE("thin_slice_poly_2_5d", load_thin_slice_poly_2_5d);
} // namespace sph