// gsph/g_pre_interaction.hpp
#pragma once
#include "pre_interaction.hpp"

namespace sph
{
    namespace gsph
    {

        class PreInteraction : public sph::PreInteraction
        {
            bool m_is_2nd_order;

        public:
            void initialize(std::shared_ptr<SPHParameters> param) override;
            void calculation(std::shared_ptr<Simulation> sim) override;
            real newton_raphson(
                const SPHParticle &p_i,
                const std::vector<SPHParticle> &particles,
                const std::vector<int> &neighbor_list,
                const int n_neighbor,
                const Periodic *periodic,
                const KernelFunction *kernel) override;
        };

    } // namespace gsph
} // namespace sph