#pragma once

#include "density_relaxation_base.hpp"
#include "lane_emden_data.hpp"
#include "core/particle.hpp"   // For SPHParticle
#include "core/parameters.hpp" // For SPHParameters

namespace sph
{
    class LaneEmdenRelaxation : public DensityRelaxationBase
    {
    public:
        LaneEmdenData m_data;

        void load_table(const std::string &filename)
        {
            m_data.load_csv(filename);
        }

        void add_relaxation_force(std::shared_ptr<Simulation> sim,
                                  const SPHParameters &params) override;

    private:
        vec_t compute_relaxation_force(const SPHParticle &p, const SPHParameters &params);
    };
}