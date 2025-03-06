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
             << p.gradh;
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
               << "id,neighbor,alpha,gradh";

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

    // Modified checkpoint reader remains the same.
    void Output::read_checkpoint(const std::string &file_name, std::shared_ptr<Simulation> sim)
    {
        std::ifstream in(file_name);
        if (!in.is_open())
        {
            THROW_ERROR("Cannot open checkpoint file: ", file_name);
        }
        std::string headerLine;
        std::getline(in, headerLine);
        // (Optionally, you can parse headerLine to verify the column order.)

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
            // Create a particle from the fields. The order is assumed to be:
            // time, pos (DIM), vel (DIM), acc (DIM), mass, dens, pres, ene, sml, id, neighbor, alpha, gradh, [additional...]
            int idx = 0;
            double timeVal = std::stod(fields[idx++]);
            // For now we set the simulation time to that in the first row.
            if (particles.empty())
            {
                checkpointTime = timeVal / m_unit.time_factor;
            }
            SPHParticle p;
#if DIM == 1
            p.pos[0] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.vel[0] = std::stod(fields[idx++]) * m_unit.time_factor / m_unit.length_factor;
            p.acc[0] = std::stod(fields[idx++]) * (m_unit.time_factor * m_unit.time_factor) / m_unit.length_factor;
#elif DIM == 2
            p.pos[0] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.pos[1] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.vel[0] = std::stod(fields[idx++]) * m_unit.time_factor / m_unit.length_factor;
            p.vel[1] = std::stod(fields[idx++]) * m_unit.time_factor / m_unit.length_factor;
            p.acc[0] = std::stod(fields[idx++]) * (m_unit.time_factor * m_unit.time_factor) / m_unit.length_factor;
            p.acc[1] = std::stod(fields[idx++]) * (m_unit.time_factor * m_unit.time_factor) / m_unit.length_factor;
#elif DIM == 3
            p.pos[0] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.pos[1] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.pos[2] = std::stod(fields[idx++]) / m_unit.length_factor;
            p.vel[0] = std::stod(fields[idx++]) * m_unit.time_factor / m_unit.length_factor;
            p.vel[1] = std::stod(fields[idx++]) * m_unit.time_factor / m_unit.length_factor;
            p.vel[2] = std::stod(fields[idx++]) * m_unit.time_factor / m_unit.length_factor;
            p.acc[0] = std::stod(fields[idx++]) * (m_unit.time_factor * m_unit.time_factor) / m_unit.length_factor;
            p.acc[1] = std::stod(fields[idx++]) * (m_unit.time_factor * m_unit.time_factor) / m_unit.length_factor;
            p.acc[2] = std::stod(fields[idx++]) * (m_unit.time_factor * m_unit.time_factor) / m_unit.length_factor;
#endif
            p.mass = std::stod(fields[idx++]) / m_unit.mass_factor;
            p.dens = std::stod(fields[idx++]) / m_unit.density_factor;
            p.pres = std::stod(fields[idx++]) / m_unit.pressure_factor;
            p.ene = std::stod(fields[idx++]) / m_unit.energy_factor;
            p.sml = std::stod(fields[idx++]) / m_unit.length_factor;
            p.id = std::stoi(fields[idx++]);
            p.neighbor = std::stoi(fields[idx++]);
            p.alpha = std::stod(fields[idx++]);
            p.gradh = std::stod(fields[idx++]);

            // (For now we ignore any additional arrays; add similar parsing here if needed.)
            particles.push_back(p);
        }
        // Now update the simulation state with the loaded particles and time.
        sim->set_particles(particles);
        sim->set_time(checkpointTime);
        WRITE_LOG << "Loaded checkpoint from " << file_name << " with " << particles.size() << " particles and time " << checkpointTime;
    }

    Output::Output(const std::string &dir, int count, const UnitSystem &unit)
        : m_dir(dir), m_count(count), m_unit(unit)
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
