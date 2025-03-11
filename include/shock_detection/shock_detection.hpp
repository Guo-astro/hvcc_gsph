#pragma once
#include <memory>

namespace sph
{

    class Simulation;
    class Periodic;

    /// \brief Detect shocks in the SPH simulation.
    ///
    /// This function computes the pressure gradient at each fluid particle,
    /// determines a shock normal, and uses Wendland‐based one‐ and two–dimensional
    /// kernel weightings to compute weighted upstream and downstream averages.
    /// It then estimates the Mach number based on the pressure jump and stores
    /// the result in the particle’s `shockSensor` field.
    ///
    /// \param sim         Shared pointer to the Simulation object.
    /// \param periodic    Raw pointer to the Periodic object from the simulation.
    /// \param gamma_val   The adiabatic index for the fluid.
    /// \param h_factor    A multiplier on the smoothing length (default is 1.0).
    void detect_shocks(std::shared_ptr<Simulation> sim,
                       const Periodic *periodic,
                       double gamma_val,
                       double h_factor = 1.0);

} // namespace sph
