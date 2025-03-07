#include <cmath>
#include <random>
#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "exception.hpp"
#include "defines.hpp"

namespace sph
{

    // Example: 3D thin disk with a uniform vertical thickness and Keplerian rotation.
    // The disk is assumed to have constant surface density (Sigma0) over 0 <= r <= R_disk.
    // The enclosed mass is approximated as: M_enc(r)= M_total * (r^2/R_disk^2)
    // Then, the Keplerian speed is: v_phi = sqrt(G * M_enc / r)
    void load_thin_disk_3d(std::shared_ptr<Simulation> sim,
                           std::shared_ptr<SPHParameters> param)
    {
#if DIM == 3
        // User-chosen disk parameters:
        const real R_disk = 3.0;     // Disk outer radius (in chosen units, e.g., parsecs)
        const real M_total = 1000.0; // Total disk mass (e.g., solar masses)
        const real H = 0.01;         // Vertical thickness (pc) (assumed uniform: from -H/2 to +H/2)
        const int Nr = 20;           // Number of radial steps
        const int Nphi = 80;         // Number of azimuthal steps
        const int Nz = 5;            // Number of vertical steps
        const real G = param->gravity.constant;

        // For a disk with constant surface density:
        // Total mass = pi * R_disk^2 * Sigma0  => Sigma0 = M_total/(pi*R_disk^2)
        const real Sigma0 = M_total / (M_PI * R_disk * R_disk);

        auto &p = sim->get_particles();
        p.clear();

        // Loop over radial, azimuthal, and vertical directions.
        // We use a uniform grid in radius (r), azimuth (phi) and vertical coordinate (z).
        // The radial coordinate is sampled uniformly in the normalized variable u in [0,1],
        // with r = u * R_disk.
        for (int ir = 0; ir < Nr; ++ir)
        {
            // Use the mid-point in the radial bin.
            real r_frac = (ir + 0.5) / Nr;
            real r = r_frac * R_disk; // actual radius

            // For a uniform surface density disk, the mass in an annulus (of width dr)
            // is given by: dM = 2*pi*r*dr*Sigma0.
            // We assume dr = R_disk/Nr.
            real dr = R_disk / Nr;
            real annulusMass = 2 * M_PI * r * dr * Sigma0;
            // The number of particles in this annulus is Nphi*Nz.
            real m_particle = annulusMass / (Nphi * Nz);

            for (int iphi = 0; iphi < Nphi; ++iphi)
            {
                // Azimuthal coordinate (phi) in [0, 2π).
                real phi = 2 * M_PI * (iphi + 0.5) / Nphi;

                // For each ring, create Nz layers in z between -H/2 and H/2.
                for (int iz = 0; iz < Nz; ++iz)
                {
                    // Uniform vertical placement:
                    real z_frac = (iz + 0.5) / Nz;           // in [0,1]
                    real z = (z_frac * 2.0 - 1.0) * H * 0.5; // z from -H/2 to +H/2

                    SPHParticle p_i;
                    // Convert polar (r, phi) into Cartesian (x,y).
                    p_i.pos[0] = r * std::cos(phi);
                    p_i.pos[1] = r * std::sin(phi);
                    p_i.pos[2] = z;

                    // Set the mass per particle.
                    p_i.mass = m_particle;

                    // Estimate local density.
                    // In a vertically uniform disk, the local (3D) density is roughly:
                    //   rho ~ Sigma0/(H)  (if H is the full thickness, or use 2H if defined as half-thickness)
                    // Here we assume H is the full thickness.
                    real rho = Sigma0 / H;
                    p_i.dens = rho;

                    // For testing a hydrostatic disk in 3D we choose a polytropic EOS.
                    // Let’s choose an isothermal-like sound speed for simplicity.
                    real c_s = 0.2; // sound speed (choose units consistently)
                    p_i.pres = c_s * c_s * p_i.dens;
                    // Alternatively, for a polytropic EOS P= K*rho^gamma with gamma=5/3,
                    // you could set p_i.pres = K * std::pow(p_i.dens, 5.0/3.0) and then compute internal energy.
                    real gamma_val = param->physics.gamma; // should be 5/3
                    p_i.ene = p_i.pres / ((gamma_val - 1.0) * p_i.dens);

                    // --- Set the velocity field: Keplerian rotation ---
                    // Compute the enclosed disk mass at radius r.
                    // For a disk with uniform surface density, M_enc = M_total*(r^2/R_disk^2).
                    // (This is valid for r <= R_disk.)
                    real M_enc = M_total * (r * r) / (R_disk * R_disk);
                    // The Keplerian speed is:
                    //   v_phi = sqrt(G * M_enc / r)
                    // At very small r (r ~ 0), care must be taken (set v=0).
                    real v_phi = (r > 1e-8) ? std::sqrt(G * M_enc / r) : 0.0;
                    // Set the azimuthal velocity in the x-y plane.
                    p_i.vel[0] = -v_phi * std::sin(phi);
                    p_i.vel[1] = v_phi * std::cos(phi);
                    p_i.vel[2] = 0.0; // no vertical motion

                    // Set particle ID
                    p_i.id = (int)p.size();

                    // Set smoothing length (3D: h ~ (mass/density)^(1/3))
                    p_i.sml = std::cbrt(p_i.mass / p_i.dens);

                    p.push_back(p_i);
                }
            }
        }

        sim->set_particle_num((int)p.size());
        WRITE_LOG << "Constructed 3D disk with " << p.size() << " particles.";
#endif
    }

    REGISTER_SAMPLE("thin_disk_3d", load_thin_disk_3d);

} // end namespace sph
