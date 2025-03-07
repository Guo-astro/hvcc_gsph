#pragma once

#include "defines.hpp"

namespace sph
{

    enum struct SPHType
    {
        SSPH,
        DISPH,
        GSPH,
        GDISPH,
    };

    enum struct KernelType
    {
        CUBIC_SPLINE,
        WENDLAND,
        UNKNOWN,
    };

    // ADDED: HeatingCooling
    struct HeatingCooling
    {
        bool is_valid = false;
        real heating_rate = 0.0;
        real cooling_rate = 0.0;
    };

    struct SPHParameters
    {

        struct Time
        {
            real start;
            real end;
            real output;
            real energy;
        } time;

        SPHType type;

        struct CFL
        {
            real sound;
            real force;
        } cfl;

        struct ArtificialViscosity
        {
            real alpha;
            bool use_balsara_switch;
            bool use_time_dependent_av;
            real alpha_max;
            real alpha_min;
            real epsilon; // tau = h / (epsilon * c)
        } av;

        struct ArtificialConductivity
        {
            real alpha;
            bool is_valid;
        } ac;

        struct Tree
        {
            int max_level;
            int leaf_particle_num;
        } tree;

        struct Physics
        {
            int neighbor_number;
            real gamma;
        } physics;

        KernelType kernel;

        bool iterative_sml;

        struct Periodic
        {
            bool is_valid;
            real range_max[DIM];
            real range_min[DIM];
        } periodic;

        struct Gravity
        {
            bool is_valid;
            real constant;
            real theta;
        } gravity;

        struct GSPH
        {
            bool is_2nd_order;
            bool force_correction; // new field; when true, symmetric (pairwise) force correction is applied

        } gsph;

        // ADDED: For Heating & Cooling
        HeatingCooling heating_cooling;
        real boundary_radius;
        bool two_and_half_sim = false; // Set to true in JSON to force 2.5D integration.
    };

} // namespace sph
