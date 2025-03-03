#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"
#include <cmath>
#include <vector>
#include <iostream> // for std::cout

namespace sph
{

    void load_shock_tube_2p5d(std::shared_ptr<Simulation> sim,
                              std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("DIM != 3 for 2.5D shock_tube");
#endif

        // Retrieve gamma from simulation parameters.
        const double gamma = param->physics.gamma;

        // Domain limits.
        const double x_min = -1.0;
        const double x_split = 0.0; // vertical discontinuity (shock tube interface)
        const double x_max = 1.0;
        const double y_min = 0.0;
        const double y_max = 0.5;
        // For 2.5D we have one layer in z.
        const double z = 0.0;
        // Use a unit thickness in z.
        const double dz = 1.0;

        // Resolution parameters.
        // Left region (x < x_split): finer resolution.
        const double dx_left = 0.0025;
        // Right region (x >= x_split): coarser resolution.
        const double dx_right = 0.01;
        // Uniform spacing in y.
        const double dy = 0.01;

        // Physical initial states.
        // Left: ρ_phys = 1.0, Right: ρ_phys = 0.25.
        const double rho_phys_left = 1.0;
        const double rho_phys_right = 0.25;

        // In a 2.5D DISPH simulation the density is computed as:
        //    ρ_sim = ρ_phys * dz * (integral of W over xy)
        // With the 3D cubic spline kernel integrated in z at z = 0, one obtains approximately:
        //    ∫W(x,y,0,h) dxdy ≈ 0.70/h.
        // Thus, ideally:
        //    ρ_sim ≈ ρ_phys * (0.70 / h).
        //
        // Smoothing lengths are typically chosen proportional to the local particle spacing.
        // Let h_left = k_left * dx_left and h_right = k_right * dx_right.
        double k_left = 1.3;  // example factor for left region.
        double k_right = 0.7; // example factor for right region.

        double h_left = k_left * dx_left;    // h_left ≈ 1.3 * 0.0025 = 0.00325
        double h_right = k_right * dx_right; // h_right ≈ 0.7 * 0.01 = 0.007

        // Theoretical initial densities computed from the SPH summation in the continuum limit:
        double rho_sim_left = rho_phys_left * (0.70 / h_left);    // ≈ 0.70 / 0.00325 ≈ 215
        double rho_sim_right = rho_phys_right * (0.70 / h_right); // ≈ 0.25 * (0.70 / 0.007) ≈ 25

        // Optionally, print these theoretical densities.
        std::cout << "Theoretical initial density (left): " << rho_sim_left << "\n";
        std::cout << "Theoretical initial density (right): " << rho_sim_right << "\n";

        // Create particle list.
        std::vector<SPHParticle> particles;
        int id = 0;

        // Loop over y for the entire domain.
        for (double y = y_min + 0.5 * dy; y < y_max; y += dy)
        {
            // Left region particles.
            for (double x = x_min + 0.5 * dx_left; x < x_split; x += dx_left)
            {
                SPHParticle p_i;
                // Set positions (2.5D: one layer in z).
                p_i.pos[0] = x;
                p_i.pos[1] = y;
                p_i.pos[2] = z;
                // Initialize velocities to zero.
                p_i.vel[0] = 0.0;
                p_i.vel[1] = 0.0;
                p_i.vel[2] = 0.0;
                // Left state.
                double pres = 1.0 * (0.70 / h_left);
                // Use simulation density computed from physical state and kernel integration.
                p_i.dens = rho_sim_left;
                p_i.pres = pres;
                // Particle mass is computed as m = ρ_phys * (dx_left * dy * dz).
                p_i.mass = rho_phys_left * dx_left * dy * dz;
                // Internal energy based on simulation density.
                p_i.ene = pres / ((gamma - 1.0) * rho_sim_left);
                p_i.id = id++;
                particles.push_back(p_i);
            }
            // Right region particles.
            for (double x = x_split + 0.5 * dx_right; x < x_max; x += dx_right)
            {
                SPHParticle p_i;
                p_i.pos[0] = x;
                p_i.pos[1] = y;
                p_i.pos[2] = z;
                p_i.vel[0] = 0.0;
                p_i.vel[1] = 0.0;
                p_i.vel[2] = 0.0;
                double pres = 0.1795 * (0.70 / h_right);
                // Use simulation density for the right region.
                p_i.dens = rho_sim_right;
                p_i.pres = pres;
                p_i.mass = rho_phys_right * dx_right * dy * dz;
                p_i.ene = pres / ((gamma - 1.0) * rho_sim_right);
                p_i.id = id++;
                particles.push_back(p_i);
            }
        }

        // Set particles in the simulation.
        sim->set_particles(particles);
        sim->set_particle_num(static_cast<int>(particles.size()));
    }

    REGISTER_SAMPLE("shock_tube_2p5d", load_shock_tube_2p5d);

} // namespace sph
