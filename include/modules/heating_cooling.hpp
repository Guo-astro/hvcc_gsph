// heating_cooling.hpp
#pragma once

#include "utilities/defines.hpp"
#include <memory>
#include "modules/module.hpp"

namespace sph
{
    // Renamed from 'HeatingCooling' -> 'HeatingCoolingModule'
    class HeatingCoolingModule : public Module
    {
    public:
        void initialize(std::shared_ptr<SPHParameters> param) override;
        void calculation(std::shared_ptr<Simulation> sim) override;

    private:
        bool m_is_valid = false;
        real m_heating_rate = 0.0;
        real m_cooling_rate = 0.0;
    };
}
