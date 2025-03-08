#include <cmath>
#include <vector>
#include <iostream>

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "lane_emden.hpp" // Provides loadLaneEmdenTableFromCSV() and getTheta()
#include "exception.hpp"

namespace sph
{
    void load_thin_slice_poly_2_5d_relax(std::shared_ptr<Simulation> sim,
                                         std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("thin_slice_poly_2_5d requires DIM == 3.");
#endif

        // Fluid parameters
        const real gamma = 5.0 / 3.0; // Polytrope with γ = 5/3
        const real n_poly = 1.5;      // Polytropic index
        const real R_fluid = 3.0;     // Fluid disk radius in x-y
        const real z_max = 0.2;       // Half-thickness in z
        const real M_total = 1000.0;  // Total fluid mass
        const real K = 1.0;           // Polytropic constant
        const real rho_c = 1.0;       // Central density

        // Lane-Emden setup
        loadLaneEmdenTableFromCSV("./sample/lane_emden/lane_emden_data_5_3.csv");
        const real xi1 = laneEmden_x.back(); // First zero of θ
        const real alpha = R_fluid / xi1;    // Radial scaling factor
        param->alpha_scaling = alpha;
        param->R_fluid = R_fluid;
        param->z_max = z_max;

        // Grid setup
        int Nx = 20, Ny = 20, Nz = 5;
        const real dx = (2.0 * R_fluid) / Nx;
        const real dy = (2.0 * R_fluid) / Ny;
        const real dz = (2.0 * z_max) / Nz;

        // Generate fluid particle positions
        std::vector<vec_t> fluid_positions;
        fluid_positions.reserve(Nx * Ny * Nz);
        for (int ix = 0; ix < Nx; ++ix)
        {
            for (int iy = 0; iy < Ny; ++iy)
            {
                real x = -R_fluid + (ix + 0.5) * dx;
                real y = -R_fluid + (iy + 0.5) * dy;
                if (x * x + y * y > R_fluid * R_fluid)
                    continue; // Only within disk radius
                for (int iz = 0; iz < Nz; ++iz)
                {
                    real z = -z_max + (iz + 0.5) * dz;
                    fluid_positions.push_back({x, y, z});
                }
            }
        }
        size_t fluid_count = fluid_positions.size();
        const real mpp = M_total / static_cast<real>(fluid_count); // Mass per particle

        // Initialize particles
        auto &particles = sim->get_particles();
        particles.clear();
        particles.reserve(fluid_count);

        int pid = 0;
        int wall_count = 0;
        for (const auto &pos : fluid_positions)
        {
            real x = pos[0];
            real y = pos[1];
            real z = pos[2];

            // Use the full 3D radial coordinate for hydrostatic calculations.
            real r = std::sqrt(x * x + y * y + z * z);
            real xi = r / alpha;
            real thetaVal = getTheta(xi);
            if (thetaVal < 0.0)
                thetaVal = 0.0; // Ensure non-negative

            real dens = rho_c * std::pow(thetaVal, n_poly);
            real pres = K * std::pow(dens, 1.0 + 1.0 / n_poly);
            real ene = (dens > 0.0) ? pres / ((gamma - 1.0) * dens) : 0.0;

            // Flag edge (wall) particles in x or y.
            bool on_edge = (std::abs(x) > (R_fluid - dx)) || (std::abs(y) > (R_fluid - dy));

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
            pp.is_wall = on_edge;
            if (on_edge)
                wall_count++;
            particles.push_back(pp);
        }

        // Output diagnostics
        std::cout << "Set " << wall_count << " edge (wall) particles.\n";
        std::cout << "Total: " << (fluid_count - wall_count) << " fluid particles, "
                  << wall_count << " wall particles, " << particles.size() << " total.\n";
        std::cout << "NOTE: Ensure integration clamps z and vz to 0 for non-wall particles.\n";

        sim->set_particle_num(static_cast<int>(particles.size()));
    }

    REGISTER_SAMPLE("thin_slice_poly_2_5d_relax", load_thin_slice_poly_2_5d_relax);
} // namespace sph
