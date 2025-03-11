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
    // NullModifier sets the pressure to a high value for all particles
    // whose (x,y) position lies within a circle of a given radius.
    class NullModifier : public CheckpointModifier
    {
    public:
        // Center (in x and y) of the circular high-pressure region.
        real center[DIM];
        // Radius of the high-pressure region.
        real radius;
        // The pressure value to set inside the region.
        real highPressure;

        NullModifier(const real center_[DIM], real radius_, real highPressure_)
            : radius(radius_), highPressure(highPressure_)
        {
            for (int i = 0; i < DIM; ++i)
                center[i] = center_[i];
        }

        virtual void modifyParticles(std::vector<SPHParticle> &particles,
                                     std::shared_ptr<Simulation> sim) override
        {
        }
    };

    // Sample function that reads an existing checkpoint (razor-thin disk data)
    // and then modifies it so that a circular region centered at (0,0,...) gets high pressure.
    void razor_thin_sg_relaxation(std::shared_ptr<Simulation> sim,
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
        std::shared_ptr<CheckpointModifier> mod = std::make_shared<NullModifier>(center_arr, 0.2, 1000.0);
        sim->set_checkpoint_modifier(mod);

        // Now, trigger the checkpoint reading.
        // (This will read the CSV and then call our modifier.)
        // For example, if you have an Output object in your solver, you can call:
        //    output->read_checkpoint(param->checkpoint_file, sim);
        // For this sample, we assume that the checkpoint reading is triggered externally.
        std::cout << "[razor_thin_sg_relaxation] Finished modifying checkpoint data.\n";
    }

    REGISTER_SAMPLE("razor_thin_sg_relaxation", razor_thin_sg_relaxation);
}
