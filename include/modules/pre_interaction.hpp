#pragma once

#include <vector>

#include "modules/module.hpp"
#include "core/particle.hpp"

namespace sph
{
    class Periodic;
    class KernelFunction;

    class PreInteraction : public Module
    {
    protected:
        bool m_use_balsara_switch;
        bool m_use_time_dependent_av;
        real m_alpha_max;
        real m_alpha_min;
        real m_epsilon; // tau = h / (epsilon * c)
        real m_gamma;
        int m_neighbor_number;
        real m_kernel_ratio;
        bool m_iteration;
        bool m_first;
        bool m_twoAndHalf;
        bool m_anisotropic = false;
        real m_hz;

        virtual real newton_raphson(
            const SPHParticle &p_i,
            const std::vector<SPHParticle> &particles,
            const std::vector<int> &neighbor_list,
            const int n_neighbor,
            const Periodic *periodic,
            const KernelFunction *kernel);

    public:
        virtual void initialize(std::shared_ptr<SPHParameters> param) override;
        void perform_initial_smoothing(std::shared_ptr<Simulation> sim, bool twoAndHalf, real kernel_ratio, int neighbor_number, real gamma);
        virtual void calculation(std::shared_ptr<Simulation> sim) override;
    };
}
