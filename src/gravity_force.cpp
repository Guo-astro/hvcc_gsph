#include "defines.hpp"
#include "gravity_force.hpp"
#include "particle.hpp"
#include "periodic.hpp"
#include "simulation.hpp"
#include "bhtree.hpp"

namespace sph
{

    // Softening functions (assuming these are already defined in your code)
    inline real f(const real r, const real h)
    {
        const real e = h * 0.5;
        const real u = r / e;
        if (u < 1.0)
        {
            return (-0.5 * u * u * (1.0 / 3.0 - 3.0 / 20 * u * u + u * u * u / 20) + 1.4) / e;
        }
        else if (u < 2.0)
        {
            return -1.0 / (15 * r) + (-u * u * (4.0 / 3.0 - u + 0.3 * u * u - u * u * u / 30) + 1.6) / e;
        }
        else
        {
            return 1 / r;
        }
    }

    inline real g(const real r, const real h)
    {
        const real e = h * 0.5;
        const real u = r / e;
        if (u < 1.0)
        {
            return (4.0 / 3.0 - 1.2 * u * u + 0.5 * u * u * u) / (e * e * e);
        }
        else if (u < 2.0)
        {
            return (-1.0 / 15 + 8.0 / 3 * u * u * u - 3 * u * u * u * u + 1.2 * u * u * u * u * u - u * u * u * u * u * u / 6.0) / (r * r * r);
        }
        else
        {
            return 1 / (r * r * r);
        }
    }

    void GravityForce::initialize(std::shared_ptr<SPHParameters> param)
    {
        m_is_valid = param->gravity.is_valid;
        if (m_is_valid)
        {
            m_constant = param->gravity.constant;
        }
    }

    void GravityForce::calculation(std::shared_ptr<Simulation> sim)
    {
        WRITE_LOG << "Calculating gravity...";

        if (!m_is_valid)
        {
            return;
        }
        apply_self_gravity(sim);     // Compute self-gravity with BHTree
        apply_external_gravity(sim); // Add external gravity from point masses
    }

    void GravityForce::apply_self_gravity(std::shared_ptr<Simulation> sim)
    {
        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        auto *tree = sim->get_tree().get();

// Parallel computation of self-gravity using the Barnes-Hut tree
#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            auto &p_i = particles[i];
            // Reset acceleration and potential for self-gravity
            p_i.acc = vec_t(0.0);
            p_i.phi = 0.0;
            tree->tree_force(p_i); // Compute self-gravity force using BHTree
        }
    }

    void GravityForce::apply_external_gravity(std::shared_ptr<Simulation> sim)
    {
        auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        auto *periodic = sim->get_periodic().get();

// Direct summation for external gravity from point masses
#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            auto &p_i = particles[i];
            if (p_i.is_point_mass)
                continue; // Point masses don’t feel external gravity

            vec_t force(0.0);
            const vec_t &r_i = p_i.pos;

            for (int j = 0; j < num; ++j)
            {
                const auto &p_j = particles[j];
                if (!p_j.is_point_mass)
                    continue; // Only point masses contribute to external gravity

                const vec_t r_ij = periodic->calc_r_ij(r_i, p_j.pos);
                const real r = std::abs(r_ij);
                if (r < 1e-12)
                    continue; // Avoid division by zero

                // Add gravitational force from point mass p_j to p_i
                force -= r_ij * (m_constant * p_j.mass * (g(r, p_i.sml) + g(r, p_j.sml)) * 0.5);
                // WRITE_LOG << "Calculating force..." << force[0];
            }

            p_i.acc += force; // Add external gravity contribution to acceleration
        }
    }

} // namespace sph