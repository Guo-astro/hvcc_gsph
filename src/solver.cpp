#include <cassert>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <omp.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/any.hpp>

#include "solver.hpp"
#include "parameters.hpp"
#include "particle.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "output.hpp"
#include "simulation.hpp"
#include "periodic.hpp"
#include "bhtree.hpp"
#include "timestep.hpp"
#include "pre_interaction.hpp"
#include "fluid_force.hpp"
#include "gravity_force.hpp"
#include "sample_registry.hpp"

// disph
#include "disph/d_pre_interaction.hpp"
#include "disph/d_fluid_force.hpp"

// gsph
#include "gsph/g_pre_interaction.hpp"
#include "gsph/g_fluid_force.hpp"

// ADDED: our new HeatingCooling module
#include "heating_cooling.hpp"

// for unit system
#include "unit_system.hpp"

namespace sph
{
    std::string sphTypeToString(SPHType type)
    {
        switch (type)
        {
        case SPHType::GSPH:
            return "GSPH";
        case SPHType::SSPH:
            return "SSPH";
        case SPHType::DISPH:
            return "DISPH";
        default:
            return "UNKNOWN";
        }
    }

    Solver::Solver(int argc, char *argv[])
        : m_unit(),
          m_param(std::make_shared<SPHParameters>()),
          m_output_dir("results"), // Default output directory
          m_num_threads(1),
          m_sample_recognized(false)
    {
        std::cout << "--------------SPH simulation-------------\n\n";

        // 1) Parse command-line arguments
        // We expect:
        //   argv[1] = sampleName (e.g. "shock_tube")
        //   argv[2] = optional .json or threads
        //   argv[3] = optional threads
        if (argc < 2)
        {
            std::cerr << "Usage: " << argv[0] << " <sampleName> [jsonFile] [numThreads]\n";
            std::exit(EXIT_FAILURE);
        }

        // The first argument is the sample name:
        m_sample_name = argv[1];
        // The second argument, if present, might be a .json or an integer:
        if (argc >= 3)
        {
            std::string arg2 = argv[2];
            if (arg2.size() > 5 && arg2.substr(arg2.size() - 5) == ".json")
            {
                // This is a JSON file
                m_json_file = arg2;
            }
            else
            {
                // interpret as an integer for threads
                m_num_threads = std::atoi(arg2.c_str());
            }
        }

        // The third argument, if present, is threads:
        if (argc >= 4)
        {
            m_num_threads = std::atoi(argv[3]);
        }
        // safety clamp:
        if (m_num_threads < 1)
            m_num_threads = 1;

        // 2) Start up the logger
        Logger::open(m_output_dir);

#ifdef _OPENMP
        WRITE_LOG << "OpenMP is enabled.";
        omp_set_num_threads(m_num_threads);
        WRITE_LOG << "Number of threads = " << m_num_threads;
#else
        WRITE_LOG << "OpenMP is disabled.";
#endif

        //    so the sample can fill in param defaults (like param->physics.gamma).
        WRITE_LOG << "app_name: " << m_sample_name;
    }

    void Solver::parseJsonOverrides()
    {
        // If we have no JSON file, skip
        if (m_json_file.empty())
        {
            return;
        }

        namespace pt = boost::property_tree;
        pt::ptree root;
        try
        {
            pt::read_json(m_json_file, root);
        }
        catch (std::exception &e)
        {
            THROW_ERROR("Cannot read JSON file: ", m_json_file, " => ", e.what());
        }

        // Now override fields in m_param as needed:
        // Example:
        m_output_dir = root.get<std::string>("outputDirectory", m_output_dir);

        m_param->time.start = root.get<real>("startTime", real(0));
        m_param->time.end = root.get<real>("endTime");
        if (m_param->time.end < m_param->time.start)
        {
            THROW_ERROR("endTime < startTime");
        }
        m_param->time.output = root.get<real>("outputTime", (m_param->time.end - m_param->time.start) / 1000);
        m_param->time.energy = root.get<real>("energyTime", m_param->time.output);

        // cfl
        m_param->cfl.sound = root.get<real>("cflSound", 0.3);
        m_param->cfl.force = root.get<real>("cflForce", 0.125);

        // Artificial Viscosity
        m_param->av.alpha = root.get<real>("avAlpha", 1.0);
        m_param->av.use_balsara_switch = root.get<bool>("useBalsaraSwitch", true);
        m_param->av.use_time_dependent_av = root.get<bool>("useTimeDependentAV", false);
        if (m_param->av.use_time_dependent_av)
        {
            m_param->av.alpha_max = root.get<real>("alphaMax", 2.0);
            m_param->av.alpha_min = root.get<real>("alphaMin", 0.1);
            if (m_param->av.alpha_max < m_param->av.alpha_min)
            {
                THROW_ERROR("alphaMax < alphaMin");
            }
            m_param->av.epsilon = root.get<real>("epsilonAV", 0.2);
        }
        // Artificial Conductivity
        m_param->ac.is_valid = root.get<bool>("useArtificialConductivity", false);
        if (m_param->ac.is_valid)
        {
            m_param->ac.alpha = root.get<real>("alphaAC", 1.0);
        }

        // Tree
        m_param->tree.max_level = root.get<int>("maxTreeLevel", 20);
        m_param->tree.leaf_particle_num = root.get<int>("leafParticleNumber", 1);

        // Physics
        m_param->physics.neighbor_number = root.get<int>("neighborNumber", 32);
        m_param->physics.gamma = root.get<real>("gamma");
        // kernel
        std::string kernel_name = root.get<std::string>("kernel", "");
        if (!kernel_name.empty())
        {
            if (kernel_name == "cubic_spline")
                m_param->kernel = KernelType::CUBIC_SPLINE;
            else if (kernel_name == "wendland")
                m_param->kernel = KernelType::WENDLAND;
            else
                THROW_ERROR("kernel is unknown: ", kernel_name);
        }

        // iterative smoothing length
        m_param->iterative_sml = root.get<bool>("iterativeSmoothingLength", true);

        // periodic
        m_param->periodic.is_valid = root.get<bool>("periodic", false);
        if (m_param->periodic.is_valid)
        {
            {
                auto &range_max = root.get_child("rangeMax");
                if (range_max.size() != DIM)
                {
                    THROW_ERROR("rangeMax != DIM");
                }
                int i = 0;
                for (auto &v : range_max)
                {
                    m_param->periodic.range_max[i] = std::stod(v.second.data());
                    ++i;
                }
            }

            {
                auto &range_min = root.get_child("rangeMin");
                if (range_min.size() != DIM)
                {
                    THROW_ERROR("rangeMax != DIM");
                }
                int i = 0;
                for (auto &v : range_min)
                {
                    m_param->periodic.range_min[i] = std::stod(v.second.data());
                    ++i;
                }
            }
        }
        // gravity
        m_param->gravity.is_valid = root.get<bool>("useGravity", false);
        if (m_param->gravity.is_valid)
        {
            m_param->gravity.constant = root.get<real>("G", 1.0);
            WRITE_LOG << "G = " << m_param->gravity.constant;
            m_param->gravity.theta = root.get<real>("theta", 0.5);
        }
        std::string sph_type = root.get<std::string>("SPHType", "ssph");

        if (sph_type == "ssph")
        {
            m_param->type = SPHType::SSPH;
        }
        else if (sph_type == "disph")
        {
            m_param->type = SPHType::DISPH;
        }
        else if (sph_type == "gsph")
        {
            m_param->type = SPHType::GSPH;
        }
        else
        {
            THROW_ERROR("Unknown SPH type");
        }
        WRITE_LOG << "Using algorithm: " << sph::sphTypeToString(m_param->type);
        // GSPH
        if (m_param->type == SPHType::GSPH)
        {
            m_param->gsph.is_2nd_order = root.get<bool>("use2ndOrderGSPH", false);
        }
        // heating/cooling
        m_param->heating_cooling.is_valid = root.get<bool>("useHeatingCooling", false);
        m_param->heating_cooling.heating_rate = root.get<real>("heatingRate", m_param->heating_cooling.heating_rate);
        m_param->heating_cooling.cooling_rate = root.get<real>("coolingRate", m_param->heating_cooling.cooling_rate);

        // unitConfig
        std::string unitFile = root.get<std::string>("unitConfig", "");
        if (!unitFile.empty())
        {
            namespace pt2 = boost::property_tree;
            pt2::ptree uroot;
            try
            {
                pt2::read_json(unitFile, uroot);
                m_unit.time_factor = uroot.get<double>("time_factor", 1.0);
                m_unit.length_factor = uroot.get<double>("length_factor", 1.0);
                m_unit.mass_factor = uroot.get<double>("mass_factor", 1.0);
                m_unit.density_factor = uroot.get<double>("density_factor", 1.0);
                m_unit.pressure_factor = uroot.get<double>("pressure_factor", 1.0);
                m_unit.energy_factor = uroot.get<double>("energy_factor", 1.0);

                m_unit.time_unit = uroot.get<std::string>("time_unit", "s");
                m_unit.length_unit = uroot.get<std::string>("length_unit", "m");
                m_unit.mass_unit = uroot.get<std::string>("mass_unit", "kg");
                m_unit.density_unit = uroot.get<std::string>("density_unit", "kg/m^3");
                m_unit.pressure_unit = uroot.get<std::string>("pressure_unit", "Pa");
                m_unit.energy_unit = uroot.get<std::string>("energy_unit", "J/kg");
            }
            catch (...)
            {
                WRITE_LOG << "Warning: cannot read unitConfig \"" << unitFile << "\", using defaults.";
            }
        }
    }

    void Solver::run()
    {
        initialize();
        assert(m_sim->get_particles().size() == m_sim->get_particle_num());

        m_output->output_particle(m_sim);
        m_output->output_energy(m_sim);
        const real t_end = m_param->time.end;
        real t_out = m_param->time.output;
        real t_ene = m_param->time.energy;

        const auto start = std::chrono::system_clock::now();
        auto t_cout_i = start;
        int loop = 0;

        real t = m_sim->get_time();
        while (t < t_end)
        {
            integrate();
            const real dt = m_sim->get_dt();
            ++loop;

            m_sim->update_time();
            t = m_sim->get_time();

            // Output to console every ~1 second
            const auto t_cout_f = std::chrono::system_clock::now();
            const real t_cout_s = std::chrono::duration_cast<std::chrono::seconds>(t_cout_f - t_cout_i).count();
            if (t_cout_s >= 1.0)
            {
                WRITE_LOG << "loop: " << loop << ", time: " << t << ", dt: " << dt
                          << ", num: " << m_sim->get_particle_num();
                t_cout_i = std::chrono::system_clock::now();
            }
            else
            {
                WRITE_LOG_ONLY << "loop: " << loop << ", time: " << t
                               << ", dt: " << dt << ", num: " << m_sim->get_particle_num();
            }

            if (t > t_out)
            {
                m_output->output_particle(m_sim);
                t_out += m_param->time.output;
            }
            if (t > t_ene)
            {
                m_output->output_energy(m_sim);
                t_ene += m_param->time.energy;
            }
        }
        const auto end = std::chrono::system_clock::now();
        const real calctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        WRITE_LOG << "\ncalculation is finished";
        WRITE_LOG << "calculation time: " << calctime << " ms";
    }

    void Solver::initialize()
    {
        parseJsonOverrides();
        m_output = std::make_shared<Output>(0, m_unit);

        m_sim = std::make_shared<Simulation>(m_param);
        // 3) If the sample was recognized, fill actual particles now
        bool recognized = SampleRegistry::instance().create_sample(m_sample_name, m_sim, m_param);
        if (!recognized)
        {
            THROW_ERROR("No recognized sample named ", m_sample_name,
                        " (and no code to fill from JSON-based ICs).");
        }

        m_timestep = std::make_shared<TimeStep>();
        if (m_param->type == SPHType::SSPH)
        {
            m_pre = std::make_shared<PreInteraction>();
            m_fforce = std::make_shared<FluidForce>();
        }
        else if (m_param->type == SPHType::DISPH)
        {
            m_pre = std::make_shared<disph::PreInteraction>();
            m_fforce = std::make_shared<disph::FluidForce>();
        }
        else if (m_param->type == SPHType::GSPH)
        {
            m_pre = std::make_shared<gsph::PreInteraction>();
            m_fforce = std::make_shared<gsph::FluidForce>();
        }
        m_gforce = std::make_shared<GravityForce>();

        // ADDED: If heating_cooling is valid, create that module
        if (m_param->heating_cooling.is_valid)
        {
            auto hc = std::make_shared<HeatingCoolingModule>();
            hc->initialize(m_param);
            m_hcool = hc;
        }

        // For GSPH additional arrays
        if (m_param->type == SPHType::GSPH)
        {
            std::vector<std::string> names;
            names.push_back("grad_density");
            names.push_back("grad_pressure");
            names.push_back("grad_velocity_0");
#if DIM == 2
            names.push_back("grad_velocity_1");
#elif DIM == 3
            names.push_back("grad_velocity_1");
            names.push_back("grad_velocity_2");
#endif
            m_sim->add_vector_array(names);
        }

        m_timestep->initialize(m_param);
        m_pre->initialize(m_param);
        m_fforce->initialize(m_param);
        m_gforce->initialize(m_param);

        auto &p = m_sim->get_particles();
        const int num = m_sim->get_particle_num();
        const real gamma = m_param->physics.gamma;
        const real c_sound = gamma * (gamma - 1.0);

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            p[i].alpha = m_param->av.alpha;
            p[i].balsara = 1.0;
            p[i].sound = std::sqrt(c_sound * p[i].ene);
        }

#ifndef EXHAUSTIVE_SEARCH
        auto tree = m_sim->get_tree();
        tree->resize(num);
        tree->make(p, num);
#endif

        m_pre->calculation(m_sim);
        m_fforce->calculation(m_sim);
        m_gforce->calculation(m_sim);
        if (m_hcool)
        {
            m_hcool->calculation(m_sim);
        }
        WRITE_LOG << "Initialization complete. Particle count="
                  << m_sim->get_particle_num();
    }

    void Solver::integrate()
    {
        m_timestep->calculation(m_sim);

        predict();
#ifndef EXHAUSTIVE_SEARCH
        m_sim->make_tree();
#endif
        m_pre->calculation(m_sim);
        m_fforce->calculation(m_sim);
        m_gforce->calculation(m_sim);

        // ADDED: call heating/cooling if present
        if (m_hcool)
            m_hcool->calculation(m_sim);

        correct();
    }

    void Solver::predict()
    {
        auto &p = m_sim->get_particles();
        const int num = m_sim->get_particle_num();
        auto *periodic = m_sim->get_periodic().get();
        const real dt = m_sim->get_dt();
        const real gamma = m_param->physics.gamma;
        const real c_sound = gamma * (gamma - 1.0);

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            p[i].vel_p = p[i].vel + p[i].acc * (0.5 * dt);
            p[i].ene_p = p[i].ene + p[i].dene * (0.5 * dt);

            p[i].pos += p[i].vel_p * dt;
            p[i].vel += p[i].acc * dt;
            p[i].ene += p[i].dene * dt;
            p[i].sound = std::sqrt(c_sound * p[i].ene);

            periodic->apply(p[i].pos);
        }
    }

    void Solver::correct()
    {
        auto &p = m_sim->get_particles();
        const int num = m_sim->get_particle_num();
        const real dt = m_sim->get_dt();
        const real gamma = m_param->physics.gamma;
        const real c_sound = gamma * (gamma - 1.0);

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            p[i].vel = p[i].vel_p + p[i].acc * (0.5 * dt);
            p[i].ene = p[i].ene_p + p[i].dene * (0.5 * dt);
            p[i].sound = std::sqrt(c_sound * p[i].ene);
        }
    }

} // namespace sph
