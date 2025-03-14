#pragma once

#include "vector_type.hpp"

namespace sph
{

    class SPHParticle
    {
    public:
        vec_t pos;   // position
        vec_t vel;   // velocity
        vec_t vel_p; // velocity at t + dt/2
        vec_t acc;   // acceleration
        real mass;   // mass
        real dens;   // mass density
        real pres;   // pressure
        real ene;    // internal energy
        real ene_p;  // internal energy at t + dt/2
        real dene;   // du/dt
        real sml;    // smoothing length
        real sound;  // sound speed

        real balsara; // balsara switch
        real alpha;   // AV coefficient

        real gradh; // grad-h term

        real phi = 0.0;             // potential
        bool is_point_mass = false; // Flag to indicate if particle is fixed

        int id;
        int neighbor;
        SPHParticle *next = nullptr;
        bool is_wall = false; // <<-- flag indicating a wall particle

        real shockSensor; // dimensionless measure of compression
        int shockMode;    // 1 = currently in shock mode, 0 = not
        int oldShockMode;
        bool switch_to_no_shock_region = false; // True if DISPH will be used
        real target_rho;
    };

}