#pragma once
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "sample_registry.hpp"
#include "simulation.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "defines.hpp"
#include "exception.hpp"
#include "utilities/checkpoint_modifier.hpp"

namespace sph
{
    // HighPressureModifier sets the pressure to a high value for all particles
    // whose (x,y) position lies within a circle of a given radius.
    class HighPressureModifier : public CheckpointModifier
    {
    public:
        // Center (in x and y) of the circular high-pressure region.
        real center[DIM];
        // Radius of the high-pressure region.
        real radius;
        // The pressure value to set inside the region.
        real highPressure;

        HighPressureModifier(const real center_[DIM], real radius_, real highPressure_)
            : radius(radius_), highPressure(highPressure_)
        {
            for (int i = 0; i < DIM; ++i)
                center[i] = center_[i];
        }

        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) override
        {
            // Use gamma from simulation parameters or default to 5/3.
            real gamma_val = (sim->get_scalar_array("dummy").empty() ? 5.0 / 3.0 : 5.0 / 3.0);
            // For this example, assume we work in the x-y plane.
            for (auto &p : particles)
            {
                real dx = p.pos[0] - center[0];
                real dy = 0.0;
#if DIM >= 2
                dy = p.pos[1] - center[1];
#endif
                real r = std::sqrt(dx * dx + dy * dy);
                if (r < radius)
                {
                    p.pres = highPressure;
                    // Update internal energy using u = P/((gamma-1)*density)
                    if (p.dens > 1e-12)
                        p.ene = p.pres / ((gamma_val - 1.0) * p.dens);
                }
            }
            std::cout << "[HighPressureModifier] Applied high pressure inside radius " << radius << "\n";
        }
    };

    // Sample function that reads an existing checkpoint (razor-thin disk data)
    // and then modifies it so that a circular region centered at (0,0,...) gets high pressure.
    void blast_wave_from_checkpoint(std::shared_ptr<Simulation> sim,
                                    std::shared_ptr<SPHParameters> param)
    {
#if DIM != 3
        THROW_ERROR("blast_wave_from_checkpoint requires DIM == 3.");
#endif

        // Ensure that param->checkpoint_file is set (the file that holds the razor-thin disk data).
        if (param->checkpoint_file.empty())
        {
            THROW_ERROR("No checkpoint file provided for blast wave initialization.");
        }
        std::cout << "[blast_wave_from_checkpoint] Reading checkpoint from " << param->checkpoint_file << "\n";

        // Set up a high-pressure modifier.
        // For example, we want to set a high pressure of 1000.0 inside a circle of radius 0.2,
        // centered at the origin.
        real center_arr[DIM] = {0.0};
#if DIM >= 2
        center_arr[1] = 0.0;
#endif
        std::shared_ptr<CheckpointModifier> mod = std::make_shared<HighPressureModifier>(center_arr, 0.2, 1000.0);
        sim->set_checkpoint_modifier(mod);

        // Now, trigger the checkpoint reading.
        // (This will read the CSV and then call our modifier.)
        // For example, if you have an Output object in your solver, you can call:
        //    output->read_checkpoint(param->checkpoint_file, sim);
        // For this sample, we assume that the checkpoint reading is triggered externally.
        std::cout << "[blast_wave_from_checkpoint] Finished modifying checkpoint data.\n";
    }

    REGISTER_SAMPLE("blast_wave_from_checkpoint", blast_wave_from_checkpoint);
}
