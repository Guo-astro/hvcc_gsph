#pragma once

#include "density_relaxation_base.hpp"
#include "lane_emden_data.hpp"

namespace sph
{
    /// Concrete implementation of density relaxation using Lane–Emden approach.
    class LaneEmdenRelaxation : public DensityRelaxationBase
    {
    public:
        // We hold a LaneEmdenData to load 2D or 3D table, etc.
        LaneEmdenData m_data;

        // Must call load_csv before usage:
        void load_table(const std::string &filename)
        {
            m_data.load_csv(filename);
        }

        // The main function that adds the Lane–Emden-based radial "relaxation" force.
        void add_relaxation_force(std::shared_ptr<Simulation> sim,
                                  const SPHParameters &params) override;
    };
}
