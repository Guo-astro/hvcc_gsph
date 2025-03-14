#include <ctime>
#include <boost/format.hpp>
#include <fstream>
#include "exception.hpp"
#include "output.hpp"
#include "logger.hpp"
#include "defines.hpp"
#include "particle.hpp"
#include "simulation.hpp"
#include "unit_system.hpp"

namespace sph
{
    // Helper function: outputs one particle's data in CSV format.
    void output_particle_data_csv(const SPHParticle &p, std::ostringstream &line, const UnitSystem &units)
    {
#if DIM == 1
        line << p.pos[0] * units.length_factor << ","
             << p.vel[0] * (units.length_factor / units.time_factor) << ","
             << p.acc[0] * (units.length_factor / (units.time_factor * units.time_factor)) << ",";
#elif DIM == 2
        line << p.pos[0] * units.length_factor << ","
             << p.pos[1] * units.length_factor << ","
             << p.vel[0] * (units.length_factor / units.time_factor) << ","
             << p.vel[1] * (units.length_factor / units.time_factor) << ","
             << p.acc[0] * (units.length_factor / (units.time_factor * units.time_factor)) << ","
             << p.acc[1] * (units.length_factor / (units.time_factor * units.time_factor)) << ",";
#elif DIM == 3
        line << p.pos[0] * units.length_factor << ","
             << p.pos[1] * units.length_factor << ","
             << p.pos[2] * units.length_factor << ","
             << p.vel[0] * (units.length_factor / units.time_factor) << ","
             << p.vel[1] * (units.length_factor / units.time_factor) << ","
             << p.vel[2] * (units.length_factor / units.time_factor) << ","
             << p.acc[0] * (units.length_factor / (units.time_factor * units.time_factor)) << ","
             << p.acc[1] * (units.length_factor / (units.time_factor * units.time_factor)) << ","
             << p.acc[2] * (units.length_factor / (units.time_factor * units.time_factor)) << ",";
#endif
        line << p.mass * units.mass_factor << ","
             << p.dens * units.density_factor << ","
             << p.pres * units.pressure_factor << ","
             << p.ene * units.energy_factor << ","
             << p.sml * units.length_factor << ","
             << p.id << ","
             << p.neighbor << ","
             << p.alpha << ","
             << p.gradh << ","
             << p.shockSensor; // Added shock detection result (e.g., Mach number)
    }

    // Modified output_particle function: writes CSV with a header line showing units.
    void Output::output_particle(std::shared_ptr<Simulation> sim)
    {
        const auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        // Use .csv extension and create file name using m_count.
        const std::string file_name = m_dir + (boost::format("/%05d.csv") % m_count).str();
        std::ofstream out(file_name);

        // Build CSV header with explicit units
        std::ostringstream header;
        header << "time [" << m_unit.time_unit << "],";
#if DIM == 1
        header << "pos_x [" << m_unit.length_unit << "],"
               << "vel_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "],"
               << "acc_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "^2],";
#elif DIM == 2
        header << "pos_x [" << m_unit.length_unit << "],"
               << "pos_y [" << m_unit.length_unit << "],"
               << "vel_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "],"
               << "vel_y [" << m_unit.length_unit << "/" << m_unit.time_unit << "],"
               << "acc_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "^2],"
               << "acc_y [" << m_unit.length_unit << "/" << m_unit.time_unit << "^2],";
#elif DIM == 3
        header << "pos_x [" << m_unit.length_unit << "],"
               << "pos_y [" << m_unit.length_unit << "],"
               << "pos_z [" << m_unit.length_unit << "],"
               << "vel_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "],"
               << "vel_y [" << m_unit.length_unit << "/" << m_unit.time_unit << "],"
               << "vel_z [" << m_unit.length_unit << "/" << m_unit.time_unit << "],"
               << "acc_x [" << m_unit.length_unit << "/" << m_unit.time_unit << "^2],"
               << "acc_y [" << m_unit.length_unit << "/" << m_unit.time_unit << "^2],"
               << "acc_z [" << m_unit.length_unit << "/" << m_unit.time_unit << "^2],";
#endif
        header << "mass [" << m_unit.mass_unit << "],"
               << "dens [" << m_unit.density_unit << "],"
               << "pres [" << m_unit.pressure_unit << "],"
               << "ene [" << m_unit.energy_unit << "],"
               << "sml [" << m_unit.length_unit << "],"
               << "id,neighbor,alpha,gradh,shockSensor";

        // Append additional arrays (if any)
        auto &scalar_map = sim->get_scalar_map();
        auto &vector_map = sim->get_vector_map();
        for (auto &kv : scalar_map)
        {
            header << "," << kv.first;
        }
        for (auto &kv : vector_map)
        {
#if DIM == 1
            header << "," << kv.first;
#elif DIM == 2
            header << "," << kv.first << "_x," << kv.first << "_y";
#elif DIM == 3
            header << "," << kv.first << "_x," << kv.first << "_y," << kv.first << "_z";
#endif
        }
        out << header.str() << "\n";

        // Write one line per particle (each line includes simulation time)
        for (int i = 0; i < num; ++i)
        {
            std::ostringstream line;
            line << time * m_unit.time_factor << ",";
            output_particle_data_csv(particles[i], line, m_unit);

            // Write additional scalar arrays
            for (auto &[name, arr] : scalar_map)
            {
                line << "," << arr[i];
            }
            // Write additional vector arrays
            for (auto &[name, arrv] : vector_map)
            {
#if DIM == 1
                line << "," << arrv[i][0];
#elif DIM == 2
                line << "," << arrv[i][0] << "," << arrv[i][1];
#elif DIM == 3
                line << "," << arrv[i][0] << "," << arrv[i][1] << "," << arrv[i][2];
#endif
            }
            out << line.str() << "\n";
        }
        WRITE_LOG << "write " << file_name;
        ++m_count;
    }

    // Recenter the particle system so that its center-of-mass is at the origin.
    void Output::recenterParticles(std::vector<SPHParticle> &particles)
    {
        double totalMass = 0.0;
        std::array<double, DIM> com = {0.0};
        std::array<double, DIM> comVel = {0.0};

        // Sum over particles.
        for (const auto &p : particles)
        {
            totalMass += p.mass;
            for (int i = 0; i < DIM; ++i)
            {
                com[i] += p.mass * p.pos[i];
                comVel[i] += p.mass * p.vel[i];
            }
        }
        // Compute the averages.
        for (int i = 0; i < DIM; ++i)
        {
            com[i] /= totalMass;
            comVel[i] /= totalMass;
        }
        // Shift each particle.
        for (auto &p : particles)
        {
            for (int i = 0; i < DIM; ++i)
            {
                p.pos[i] -= com[i];
                p.vel[i] -= comVel[i];
            }
        }
    }

    void Output::read_checkpoint(const std::string &file_name, std::shared_ptr<Simulation> sim)
    {
        WRITE_LOG << "Output::read_checkpoint called with simulation pointer: " << sim.get();

        std::ifstream in(file_name);
        if (!in.is_open())
        {
            THROW_ERROR("Cannot open checkpoint file: ", file_name);
        }
        std::string headerLine;
        std::getline(in, headerLine);

        std::vector<SPHParticle> particles;
        std::string line;
        double checkpointTime = 0.0;
        while (std::getline(in, line))
        {
            std::istringstream ss(line);
            std::string field;
            std::vector<std::string> fields;
            while (std::getline(ss, field, ','))
            {
                fields.push_back(field);
            }
            int idx = 0;
            double timeVal = std::stod(fields[idx++]);
            if (particles.empty())
            {
                checkpointTime = timeVal / m_unit.time_factor;
            }
            SPHParticle p;

#if DIM == 3
            p.pos[0] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.pos[1] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.pos[2] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.vel[0] = std::stod(fields[idx++]) / (m_unit.length_factor / m_unit.time_factor);
            p.vel[1] = std::stod(fields[idx++]) / (m_unit.length_factor / m_unit.time_factor);
            p.vel[2] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
            p.acc[0] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
            p.acc[1] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
            p.acc[2] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
#elif DIM == 2
            p.pos[0] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.pos[1] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.vel[0] = std::stod(fields[idx++]) / (m_unit.length_factor / m_unit.time_factor);
            p.vel[1] = std::stod(fields[idx++]) / (m_unit.length_factor / m_unit.time_factor);
            p.acc[0] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
            p.acc[1] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
#elif DIM == 1
            p.pos[0] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.vel[0] = std::stod(fields[idx++]) / (m_unit.length_factor / m_unit.time_factor);
            p.acc[0] = std::stod(fields[idx++]) / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
#endif

            p.mass = std::stod(fields[idx++]) / m_unit.mass_factor;

            p.dens = std::stod(fields[idx++]) / m_unit.density_factor;
            p.pres = std::stod(fields[idx++]) / m_unit.pressure_factor;
            p.ene = std::stod(fields[idx++]) / m_unit.energy_factor;
            p.sml = std::stod(fields[idx++]) / m_unit.length_factor;
            p.id = std::stoi(fields[idx++]);

            p.neighbor = std::stoi(fields[idx++]);

            p.alpha = std::stod(fields[idx++]);
            try
            {
                p.gradh = std::stod(fields[idx++]);
            }
            catch (const std::out_of_range &e)
            {
                // WRITE_LOG << "stod: out of range error for  p.gradh : " << p.gradh;
            }
            catch (const std::invalid_argument &e)
            {
                WRITE_LOG << "stod: invalid argument error for  p.gradh : " << p.gradh;
            }
            p.shockSensor = std::stod(fields[idx++]);

            // Note: The shockSensor value was not saved to the checkpoint file.
            // You may choose to output it later from the simulation state.
            particles.push_back(p);
        }
        in.close();

        // Optional recentering
        if (m_recenterParticles)
        {
            recenterParticles(particles);
            WRITE_LOG << "Checkpoint particles recentered.";
        }

        // NEW: Use the Simulation's checkpoint modifier if set.
        auto modifier = sim->get_checkpoint_modifier();
        WRITE_LOG << "CheckpointModifier in read_checkpoint: " << (modifier ? "found" : "not found")
                  << " (simulation pointer: " << sim.get() << ")";
        if (modifier)
        {
            modifier->modifyParticles(particles, sim);
            WRITE_LOG << "CheckpointModifier applied custom modifications.";
        }

        sim->set_particles(particles);
        sim->set_time(checkpointTime);
        sim->set_particle_num((int)particles.size());

        WRITE_LOG << "Loaded checkpoint from " << file_name << " with "
                  << particles.size() << " particles and time " << checkpointTime;
    }

    // Updated constructor: now accepts a flag to recenter particles.
    Output::Output(const std::string &dir, int count, const UnitSystem &unit, bool recenterParticles)
        : m_dir(dir), m_count(count), m_unit(unit), m_recenterParticles(recenterParticles)
    {
        // energy output code omitted...
    }

    Output::~Output()
    {
        m_out_energy.close();
    }

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
