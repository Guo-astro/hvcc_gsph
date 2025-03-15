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
             << p.shockSensor << "," // Add comma after shockSensor
             << p.ene_floored;       // Add ene_floored
    }

    // Modified output_particle function: writes CSV with a header line showing units.
    void Output::output_particle(std::shared_ptr<Simulation> sim)
    {
        const auto &particles = sim->get_particles();
        const int num = sim->get_particle_num();
        const real time = sim->get_time();

        const std::string file_name = m_dir + (boost::format("/%05d.csv") % m_count).str();
        std::ofstream out(file_name);

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
               << "id,neighbor,alpha,gradh,shockSensor,ene_floored"; // Add ene_floored

        // Rest of the function remains unchanged
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

        for (int i = 0; i < num; ++i)
        {
            std::ostringstream line;
            line << time * m_unit.time_factor << ",";
            output_particle_data_csv(particles[i], line, m_unit);
            for (auto &[name, arr] : scalar_map)
            {
                line << "," << arr[i];
            }
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
        std::ifstream file(file_name);
        if (!file.is_open())
        {
            WRITE_LOG << "Cannot open checkpoint file: " << file_name << "\n";
            throw std::runtime_error("Cannot open checkpoint file");
        }
        std::vector<SPHParticle> particles;

        std::string line;
        int lineNum = 1;
        double checkpointTime = 0.0;

        // Read and parse the header
        if (!std::getline(file, line))
        {
            WRITE_LOG << "Checkpoint file is empty\n";
            throw std::runtime_error("Empty checkpoint file");
        }
        std::vector<std::string> header_fields;
        std::stringstream header_ss(line);
        std::string field;
        while (std::getline(header_ss, field, ','))
        {
            // Remove units in brackets, e.g., "time [s]" -> "time"
            size_t bracket = field.find('[');
            if (bracket != std::string::npos)
            {
                field = field.substr(0, bracket);
            }
            field.erase(0, field.find_first_not_of(" \t")); // Trim leading whitespace
            field.erase(field.find_last_not_of(" \t") + 1); // Trim trailing whitespace
            header_fields.push_back(field);
        }

        // Create a map from field names to indices
        std::unordered_map<std::string, int> field_map;
        for (int i = 0; i < header_fields.size(); ++i)
        {
            field_map[header_fields[i]] = i;
        }

        // Define required fields based on DIM
        std::vector<std::string> required_fields;
#if DIM == 1
        required_fields = {"time", "pos_x", "vel_x", "acc_x", "mass", "dens", "pres", "ene", "sml",
                           "id", "neighbor", "alpha", "gradh", "shockSensor"};
#elif DIM == 2
        required_fields = {"time", "pos_x", "pos_y", "vel_x", "vel_y", "acc_x", "acc_y", "mass",
                           "dens", "pres", "ene", "sml", "id", "neighbor", "alpha", "gradh", "shockSensor"};
#elif DIM == 3
        required_fields = {"time", "pos_x", "pos_y", "pos_z", "vel_x", "vel_y", "vel_z",
                           "acc_x", "acc_y", "acc_z", "mass", "dens", "pres", "ene", "sml",
                           "id", "neighbor", "alpha", "gradh", "shockSensor"};
#endif

        // Check for missing required fields
        for (const auto &req_field : required_fields)
        {
            if (field_map.find(req_field) == field_map.end())
            {
                WRITE_LOG << "Checkpoint file missing required field: " << req_field << "\n";
                throw std::runtime_error("Checkpoint file missing required field: " + req_field);
            }
        }

        // Read particle data
        particles.clear();
        while (std::getline(file, line))
        {
            std::vector<std::string> fields;
            std::stringstream ss(line);
            std::string value;
            while (std::getline(ss, value, ','))
            {
                fields.push_back(value);
            }

            if (fields.empty())
            {
                WRITE_LOG << "Line " << lineNum << ": Empty line skipped\n";
                ++lineNum;
                continue;
            }

            // Ensure the line has at least as many fields as in the header
            if (fields.size() < header_fields.size())
            {
                WRITE_LOG << "Line " << lineNum << ": Insufficient fields (" << fields.size()
                          << " found, " << header_fields.size() << " expected)\n";
                ++lineNum;
                continue;
            }

            SPHParticle p{};
            // Read time (as in original code, used only once for checkpointTime)
            double timeVal = safe_stod(fields[field_map["time"]], "time");
            if (particles.empty())
            {
                checkpointTime = timeVal / m_unit.time_factor;
            }

            // Read position
#if DIM >= 1
            p.pos[0] = safe_stod(fields[field_map["pos_x"]], "pos_x") / m_unit.length_factor;
#endif
#if DIM >= 2
            p.pos[1] = safe_stod(fields[field_map["pos_y"]], "pos_y") / m_unit.length_factor;
#endif
#if DIM == 3
            p.pos[2] = safe_stod(fields[field_map["pos_z"]], "pos_z") / m_unit.length_factor;
#endif

            // Read velocity
#if DIM >= 1
            p.vel[0] = safe_stod(fields[field_map["vel_x"]], "vel_x") / (m_unit.length_factor / m_unit.time_factor);
#endif
#if DIM >= 2
            p.vel[1] = safe_stod(fields[field_map["vel_y"]], "vel_y") / (m_unit.length_factor / m_unit.time_factor);
#endif
#if DIM == 3
            p.vel[2] = safe_stod(fields[field_map["vel_z"]], "vel_z") / (m_unit.length_factor / m_unit.time_factor);
#endif

            // Read acceleration
#if DIM >= 1
            p.acc[0] = safe_stod(fields[field_map["acc_x"]], "acc_x") / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
#endif
#if DIM >= 2
            p.acc[1] = safe_stod(fields[field_map["acc_y"]], "acc_y") / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
#endif
#if DIM == 3
            p.acc[2] = safe_stod(fields[field_map["acc_z"]], "acc_z") / (m_unit.length_factor / (m_unit.time_factor * m_unit.time_factor));
#endif

            // Read scalar properties with unit conversions
            p.mass = safe_stod(fields[field_map["mass"]], "mass") / m_unit.mass_factor;
            p.dens = safe_stod(fields[field_map["dens"]], "dens") / m_unit.density_factor;
            p.pres = safe_stod(fields[field_map["pres"]], "pres") / m_unit.pressure_factor;
            p.ene = safe_stod(fields[field_map["ene"]], "ene") / m_unit.energy_factor;
            p.sml = safe_stod(fields[field_map["sml"]], "sml") / m_unit.length_factor;

            // Read integer and additional properties
            p.id = safe_stoi(fields[field_map["id"]], "id");
            p.neighbor = safe_stoi(fields[field_map["neighbor"]], "neighbor");
            p.alpha = safe_stod(fields[field_map["alpha"]], "alpha");
            p.gradh = safe_stod(fields[field_map["gradh"]], "gradh");
            p.shockSensor = safe_stod(fields[field_map["shockSensor"]], "shockSensor");

            // Read optional field ene_floored
            if (field_map.count("ene_floored"))
            {
                p.ene_floored = safe_stoi(fields[field_map["ene_floored"]], "ene_floored");
            }
            else
            {
                p.ene_floored = 0; // Default for older files
            }

            particles.push_back(p);
            ++lineNum;
        }

        file.close();
        // Optional recentering
        if (m_recenterParticles)
        {
            recenterParticles(particles);
            WRITE_LOG << "Checkpoint particles recentered.";
        }

        // NEW: Use the Simulation's checkpoint modifier if set.
        auto modifier = sim->get_checkpoint_modifier();
        if (modifier)
        {
            modifier->modifyParticles(particles, sim);
            WRITE_LOG << "CheckpointModifier applied custom modifications.";
        }

        sim->set_particles(particles);
        sim->set_time(checkpointTime);
        sim->set_particle_num((int)particles.size());
        WRITE_LOG << "Loaded " << particles.size() << " particles from " << file_name << "\n";
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
