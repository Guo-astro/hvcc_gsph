#include <ctime>
#include <boost/format.hpp>
#include <fstream>
#include "output.hpp"
#include "logger.hpp"
#include "defines.hpp"
#include "particle.hpp"
#include "simulation.hpp"
#include "unit_system.hpp" // For UnitSystem

namespace sph
{

    // Helper function: outputs one particle's data using conversion factors from the UnitSystem.
    void output_particle_data(const SPHParticle &p, std::ofstream &out, const UnitSystem &units)
    {
#if DIM == 1
        out << p.pos[0] * units.length_factor << " ";
        out << p.vel[0] * (units.length_factor / units.time_factor) << " ";
        out << p.acc[0] * (units.length_factor / (units.time_factor * units.time_factor)) << " ";
#elif DIM == 2
        out << p.pos[0] * units.length_factor << " "
            << p.pos[1] * units.length_factor << " ";
        out << p.vel[0] * (units.length_factor / units.time_factor) << " "
            << p.vel[1] * (units.length_factor / units.time_factor) << " ";
        out << p.acc[0] * (units.length_factor / (units.time_factor * units.time_factor)) << " "
            << p.acc[1] * (units.length_factor / (units.time_factor * units.time_factor)) << " ";
#elif DIM == 3
        out << p.pos[0] * units.length_factor << " "
            << p.pos[1] * units.length_factor << " "
            << p.pos[2] * units.length_factor << " ";
        out << p.vel[0] * (units.length_factor / units.time_factor) << " "
            << p.vel[1] * (units.length_factor / units.time_factor) << " "
            << p.vel[2] * (units.length_factor / units.time_factor) << " ";
        out << p.acc[0] * (units.length_factor / (units.time_factor * units.time_factor)) << " "
            << p.acc[1] * (units.length_factor / (units.time_factor * units.time_factor)) << " "
            << p.acc[2] * (units.length_factor / (units.time_factor * units.time_factor)) << " ";
#endif
        out << p.mass * units.mass_factor << " ";
        out << p.dens * units.density_factor << " ";
        out << p.pres * units.pressure_factor << " ";
        out << p.ene * units.energy_factor << " ";
        out << p.sml * units.length_factor << " ";
        out << p.id << " " << p.neighbor << " " << p.alpha << " " << p.gradh << "\n";
    }

    //
    // Output constructor now accepts a UnitSystem instance.
    // It opens the energy file and writes a header line using the unit labels.
    //
    Output::Output(int count, const UnitSystem &unit)
        : m_count(count), m_unit(unit)
    {
        const std::string dir_name = Logger::get_dir_name();
        const std::string file_name = dir_name + "/energy.dat";
        m_out_energy.open(file_name);
        m_out_energy << "# time [" << m_unit.time_unit << "] "
                     << "kinetic [" << m_unit.energy_unit << "] "
                     << "thermal [" << m_unit.energy_unit << "] "
                     << "potential [" << m_unit.energy_unit << "] "
                     << "total [" << m_unit.energy_unit << "]\n";
    }

    Output::~Output()
    {
        m_out_energy.close();
    }

    //
    // output_particle() writes a header with current simulation time and column names (with units),
    // then writes the particle data using output_particle_data().
    void Output::output_particle(std::shared_ptr<Simulation> sim)
    {
        const auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        const std::string dir_name = Logger::get_dir_name();
        const std::string file_name = dir_name + (boost::format("/%05d.dat") % m_count).str();
        std::ofstream out(file_name);

        // Write a header line with the current simulation time (converted to desired units)
        out << "# Time [" << m_unit.time_unit << "]: " << time * m_unit.time_factor << "\n";

#if DIM == 1
        out << "# Columns: pos [" << m_unit.length_unit << "], vel ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "], acc ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "^2], mass ["
            << m_unit.mass_unit << "], dens [" << m_unit.density_unit << "], pres ["
            << m_unit.pressure_unit << "], ene [" << m_unit.energy_unit << "], sml ["
            << m_unit.length_unit << "], id, neighbor, alpha, gradh\n";
#elif DIM == 2
        out << "# Columns: pos_x [" << m_unit.length_unit << "], pos_y [" << m_unit.length_unit
            << "], vel_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "], vel_y ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "], acc_x ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "^2], acc_y ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "^2], mass ["
            << m_unit.mass_unit << "], dens [" << m_unit.density_unit << "], pres ["
            << m_unit.pressure_unit << "], ene [" << m_unit.energy_unit << "], sml ["
            << m_unit.length_unit << "], id, neighbor, alpha, gradh\n";
#elif DIM == 3
        out << "# Columns: pos_x [" << m_unit.length_unit << "], pos_y [" << m_unit.length_unit << "], pos_z ["
            << m_unit.length_unit << "], vel_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "], vel_y ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "], vel_z ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "], acc_x ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "^2], acc_y ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "^2], acc_z ["
            << m_unit.length_unit << "/" << m_unit.time_unit << "^2], mass ["
            << m_unit.mass_unit << "], dens [" << m_unit.density_unit << "], pres ["
            << m_unit.pressure_unit << "], ene [" << m_unit.energy_unit << "], sml ["
            << m_unit.length_unit << "], id, neighbor, alpha, gradh\n";
#endif

        for (int i = 0; i < num; ++i)
        {
            output_particle_data(particles[i], out, m_unit);
        }
        WRITE_LOG << "write " << file_name;
        ++m_count;
    }

    //
    // output_energy() computes the kinetic, thermal, and potential energies, converts them using unit factors,
    // and writes a header line (with units) to the energy file.
    void Output::output_energy(std::shared_ptr<Simulation> sim)
    {
        const auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        real kinetic = 0.0;
        real thermal = 0.0;
        real potential = 0.0;

#pragma omp parallel for reduction(+ : kinetic, thermal, potential)
        for (int i = 0; i < num; ++i)
        {
            const auto &p = particles[i];
            kinetic += 0.5 * p.mass * abs2(p.vel);
            thermal += p.mass * p.ene;
            potential += 0.5 * p.mass * p.phi;
        }
        const real total = kinetic + thermal + potential;

        m_out_energy << (time * m_unit.time_factor) << " "
                     << (kinetic * m_unit.energy_factor) << " "
                     << (thermal * m_unit.energy_factor) << " "
                     << (potential * m_unit.energy_factor) << " "
                     << (total * m_unit.energy_factor) << std::endl;
    }

} // namespace sph
