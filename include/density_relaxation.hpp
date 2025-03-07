#ifndef DENSITY_RELAXATION_HPP
#define DENSITY_RELAXATION_HPP

#include "simulation.hpp" // Contains Simulation class
#include "parameters.hpp" // Contains SPHParameters struct

namespace sph
{
    void perform_density_relaxation(std::shared_ptr<Simulation> sim, const SPHParameters &params);
}

#endif // DENSITY_RELAXATION_HPP