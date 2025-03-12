#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm> // for std::lower_bound

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
        const real z_max = 0.01;      // Half-thickness (unused since z=0)
        const real K = 1.0;           // Polytropic constant
        const real rho_c = 1.0;       // Central surface density

        // Load Lane-Emden solution
        loadLaneEmdenTableFromCSV("./sample/thin_slice_poly_2_5d/adiabatic_razor_thin_disk_sigma.csv");
        const real xi_max = laneEmden_x.back();
        const real R_fluid = xi_max; // Fluid disk radius

        const real theta_max = laneEmden_theta.back();
        if (theta_max > 1e-3)
        {
            std::cerr << "Warning: θ(xi_max=" << xi_max << ") = " << theta_max
                      << " is not near zero; consider adjusting R_fluid or extending the table.\n";
        }
        const real alpha = R_fluid / xi_max; // Radial scaling factor
        param->alpha_scaling = alpha;
        param->R_fluid = R_fluid;
        param->h_z = z_max;

        // Compute cumulative mass M(<xi) using the trapezoidal rule
        std::vector<real> M_cum(laneEmden_x.size(), 0.0);
        for (size_t i = 1; i < laneEmden_x.size(); ++i)
        {
            real xi_prev = laneEmden_x[i - 1];
            real xi_curr = laneEmden_x[i];
            real theta_prev = laneEmden_theta[i - 1];
            real theta_curr = laneEmden_theta[i];
            real sigma_prev = std::pow(theta_prev, n_poly);
            real sigma_curr = std::pow(theta_curr, n_poly);
            real dM = 0.5 * (xi_prev * sigma_prev + xi_curr * sigma_curr) *
                      (xi_curr - xi_prev) * 2.0 * M_PI;
            M_cum[i] = M_cum[i - 1] + dM;
        }
        real M_total_lane_emden = M_cum.back();
        const real M_total = M_total_lane_emden; // Full integrated mass

        // Truncate the disk to avoid placing particles near the very outer edge.
        // Here we use only the inner 98% of the mass.
        const real f = 0.98;
        const real M_trunc = f * M_total;

        // Mesh parameters: choose the number of radial bins and angular subdivisions.
        // For example, 100 rings and 100 particles per ring gives 10,000 particles.
        const int N_r = 100;     // number of radial bins (rings)
        const int N_theta = 100; // number of angular divisions per ring
        int total_particles = N_r * N_theta;
        // Each particle gets the same mass so that the total mass is M_trunc.
        real mpp = M_trunc / static_cast<real>(total_particles);

        // Initialize particles container
        auto &particles = sim->get_particles();
        particles.clear();
        particles.reserve(total_particles);

        int pid = 0;
        // Loop over radial bins
        for (int i = 0; i < N_r; ++i)
        {
            // Compute the cumulative mass at the center of the i-th radial bin.
            real u = (i + 0.5) / static_cast<real>(N_r);
            real M_i = u * M_trunc;

            // Invert the cumulative mass function to find xi for the current radial bin.
            real xi = laneEmden_x.front();
            auto it = std::lower_bound(M_cum.begin(), M_cum.end(), M_i);
            size_t idx = std::distance(M_cum.begin(), it);
            if (idx == 0)
            {
                xi = laneEmden_x[0];
            }
            else if (idx >= M_cum.size())
            {
                xi = xi_max;
            }
            else
            {
                real M0 = M_cum[idx - 1], M1 = M_cum[idx];
                real xi0 = laneEmden_x[idx - 1], xi1 = laneEmden_x[idx];
                real frac = (M_i - M0) / (M1 - M0);
                xi = xi0 + frac * (xi1 - xi0);
                if (xi > xi_max)
                    xi = xi_max;
            }
            // Convert xi to physical radius.
            real r = alpha * xi;

            // Loop over angular subdivisions in this ring.
            for (int j = 0; j < N_theta; ++j)
            {
                real theta_angle = 2.0 * M_PI * (j + 0.5) / N_theta;
                real x = r * std::cos(theta_angle);
                real y = r * std::sin(theta_angle);
                real z = 0.0; // Razor-thin disk

                // Compute local polytropic properties using the Lane-Emden value.
                real thetaVal = getTheta(xi);
                if (thetaVal < 0.0)
                    thetaVal = 0.0;
                real dens = rho_c * std::pow(thetaVal, n_poly); // Surface density
                real pres = K * std::pow(dens, 1.0 + 1.0 / n_poly);
                real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;

                // Create the particle.
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
                pp.is_wall = false;
                particles.push_back(pp);
            }
        }

        // Output diagnostics.
        std::cout << "Mesh method: Placed " << total_particles << " fluid particles using a polar grid mesh.\n";
        std::cout << "Each particle has mass " << mpp << " and covers " << (100.0 * f)
                  << "% of the total mass.\n";
        std::cout << "NOTE: Integration should clamp z and vz to 0 for all particles.\n";

        sim->set_particle_num(total_particles);
    }

    REGISTER_SAMPLE("thin_slice_poly_2_5d", load_thin_slice_poly_2_5d);
} // namespace sph
