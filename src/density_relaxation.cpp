#include "density_relaxation.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "logger.hpp"
#include "lane_emden.hpp" // to use getTheta() and the laneEmden_x/table

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cmath>
#include <omp.h>

namespace sph
{
    // Helper: compute dθ/dξ at a given ξ using the Lane–Emden lookup table.
    // This is an example interpolation function for dimensionless data {xi, theta}.
    real dTheta_dXi(real xi)
    {
        // Ensure the table is loaded.
        if (laneEmden_x.empty())
            loadLaneEmdenTableFromCSV(); // uses default filename if not loaded

        // If xi is below or above the table range, do a simple slope extrapolation:
        if (xi <= laneEmden_x.front())
        {
            if (laneEmden_x.size() >= 2)
            {
                return (laneEmden_theta[1] - laneEmden_theta[0]) /
                       (laneEmden_x[1] - laneEmden_x[0]);
            }
            else
                return 0.0;
        }
        if (xi >= laneEmden_x.back())
        {
            size_t n = laneEmden_x.size();
            if (n >= 2)
            {
                return (laneEmden_theta[n - 1] - laneEmden_theta[n - 2]) /
                       (laneEmden_x[n - 1] - laneEmden_x[n - 2]);
            }
            else
                return 0.0;
        }

        // Otherwise, find interval [i, i+1] such that laneEmden_x[i] <= xi < laneEmden_x[i+1].
        size_t i = 0;
        for (; i < laneEmden_x.size() - 1; ++i)
        {
            if (xi < laneEmden_x[i + 1])
                break;
        }
        real dx = laneEmden_x[i + 1] - laneEmden_x[i];
        if (dx < 1e-12)
            return 0.0;
        real dtheta = laneEmden_theta[i + 1] - laneEmden_theta[i];
        return dtheta / dx;
    }

    // Example function that integrates theta^n in the vertical direction from z=0 to z_max.
    // For a truly razor-thin disk, we might keep z=0, but if you want a finite thickness,
    // this function can be used. This code is purely illustrative.
    real integrate_theta_power(real z_max, real alpha, real n, int N = 1000)
    {
        // Use Simpson's rule (N must be even)
        if (N % 2 == 1)
            N++; // ensure even

        real dz = z_max / static_cast<real>(N);
        real sum = 0.0;

        for (int i = 0; i <= N; i++)
        {
            real z = i * dz;
            real xi = z / alpha;
            real theta_val = getTheta(xi);
            real f_val = std::pow(theta_val, n);
            if (i == 0 || i == N)
                sum += f_val;
            else if (i % 2 == 1)
                sum += 4.0 * f_val;
            else
                sum += 2.0 * f_val;
        }
        return sum * dz / 3.0;
    }

    // compute_relaxation_force: an example function that tries to push each particle
    // toward the polytropic Lane–Emden solution by adding an artificial acceleration
    // based on the difference between current and target states. This is highly
    // application-specific and can be changed as needed.
    vec_t compute_relaxation_force(const SPHParticle &p, const SPHParameters &params)
    {
        vec_t force{};
        // Polytropic parameters
        const real n = 1.5;                   // polytropic index => gamma=5/3
        const real gamma_val = 1.0 + 1.0 / n; // = 5/3
        const real rho_c = 1.0;               // central "density" or surface density
        const real K = 1.0;                   // polytropic constant

        // radial coordinate
        real r_phys = std::sqrt(inner_product(p.pos, p.pos));
        if (r_phys < 1e-12)
            return force;

        // dimensionless coordinate
        real xi = r_phys / params.alpha_scaling;
        // get θ(ξ)
        real theta_val = getTheta(xi);

        // optionally compute a gradient or a mismatch factor
        // For demonstration, we define a simple coefficient that tries to
        // push the system to θ=1 near the center. You might do a real dP/dr instead.
        real mismatch = (1.0 - theta_val);
        real factor = 0.01; // tweak as needed
        real a_r = factor * mismatch;

        // radial unit vector
        vec_t e_r;
        for (int i = 0; i < DIM; i++)
        {
            e_r[i] = p.pos[i] / r_phys;
        }
        // define the relaxation force in the radial direction
        for (int i = 0; i < DIM; i++)
        {
            force[i] = -a_r * e_r[i]; // negative sign to push outward if theta<1
        }
        return force;
    }

    // add_relaxation_force: loop over all fluid particles, compute the relaxation acceleration,
    // and add it to their accelerations. This is typically done each timestep or substep.
    void add_relaxation_force(std::shared_ptr<Simulation> sim, const SPHParameters &params)
    {
        auto &particles = sim->get_particles();
        int num_p = sim->get_particle_num();

#pragma omp parallel for
        for (int i = 0; i < num_p; i++)
        {
            if (particles[i].is_wall)
                continue;
            // compute the artificial "relaxation" force
            vec_t relax_force = compute_relaxation_force(particles[i], params);
            // add it to the acceleration
            particles[i].acc -= relax_force;
            // optionally zero out velocity if you want a pure relaxation with no dynamics
            particles[i].vel = 0.0;
        }
        WRITE_LOG << "[density_relaxation] Added Lane–Emden-based relaxation force to particles.";
    }
} // namespace sph
