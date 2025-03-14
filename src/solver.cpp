/* ================================
 * solver.cpp
 * ================================ */
#include <cassert>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <omp.h>
#include <filesystem>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>

#include "module_factory.hpp"
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

// DISPH
#include "disph/d_pre_interaction.hpp"
#include "disph/d_fluid_force.hpp"

// GSPH
#include "gsph/g_pre_interaction.hpp"
#include "gsph/g_fluid_force.hpp"

// GDISPH
#include "gdisph/gdi_pre_interaction.hpp"
#include "gdisph/gdi_fluid_force.hpp"

// Heating and Cooling module
#include "heating_cooling.hpp"

// Unit system and density relaxation helper
#include "unit_system.hpp"
#include "relaxation/density_relaxation.hpp"
#include "shock_detection/shock_detection.hpp"
namespace sph
{
    // **Module Registrations**
    namespace
    {
        struct ModuleRegistrations
        {
            ModuleRegistrations()
            {
                auto &factory = ModuleFactory::instance();
                // SSPH modules
                factory.register_module(SPHType::SSPH, "PreInteraction", []()
                                        { return std::make_shared<sph::PreInteraction>(); });
                factory.register_module(SPHType::SSPH, "FluidForce", []()
                                        { return std::make_shared<sph::FluidForce>(); });
                // DISPH modules
                factory.register_module(SPHType::DISPH, "PreInteraction", []()
                                        { return std::make_shared<sph::disph::PreInteraction>(); });
                factory.register_module(SPHType::DISPH, "FluidForce", []()
                                        { return std::make_shared<sph::disph::FluidForce>(); });
                // GSPH modules
                factory.register_module(SPHType::GSPH, "PreInteraction", []()
                                        { return std::make_shared<sph::gsph::PreInteraction>(); });
                factory.register_module(SPHType::GSPH, "FluidForce", []()
                                        { return std::make_shared<sph::gsph::FluidForce>(); });
                // GDISPH modules
                factory.register_module(SPHType::GDISPH, "PreInteraction", []()
                                        { return std::make_shared<sph::gdisph::PreInteraction>(); });
                factory.register_module(SPHType::GDISPH, "FluidForce", []()
                                        { return std::make_shared<sph::gdisph::FluidForce>(); });
            }
        } g_moduleRegistrations;
    }

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
        case SPHType::GDISPH:
            return "GDISPH";
        default:
            return "UNKNOWN";
        }
    }

    Solver::Solver(int argc, char *argv[])
        : m_unit(),
          m_param(std::make_shared<SPHParameters>()),
          m_output_dir("results"),
          m_num_threads(1),
          m_sample_recognized(false)
    {
        std::cout << "--------------SPH simulation-------------\n\n";

        if (argc < 2)
        {
            std::cerr << "Usage: " << argv[0] << " <sampleName> [jsonFile] [numThreads]\n";
            std::exit(EXIT_FAILURE);
        }

        m_sample_name = argv[1];
        if (argc >= 3)
        {
            std::string arg2 = argv[2];
            if (arg2.size() > 5 && arg2.substr(arg2.size() - 5) == ".json")
            {
                m_json_file = arg2;
            }
            else
            {
                m_num_threads = std::atoi(arg2.c_str());
            }
        }

        if (argc >= 4)
        {
            m_num_threads = std::atoi(argv[3]);
        }
        if (m_num_threads < 1)
            m_num_threads = 1;

        Logger::open(m_output_dir);

#ifdef _OPENMP
        WRITE_LOG << "OpenMP is enabled.";
        omp_set_num_threads(m_num_threads);
        WRITE_LOG << "Number of threads = " << m_num_threads;
#else
        WRITE_LOG << "OpenMP is disabled.";
#endif

        WRITE_LOG << "app_name: " << m_sample_name;
    }

    // **Type-Specific Parser Functions**
    namespace
    {
        using ParserFunc = std::function<void(const boost::property_tree::ptree &, SPHParameters &)>;
        void parseSSPH(const boost::property_tree::ptree &root, SPHParameters &param)
        {
            // No specific parameters for SSPH in this example
        }
        void parseDISPH(const boost::property_tree::ptree &root, SPHParameters &param)
        {
            // No specific parameters for DISPH in this example
        }
        void parseGSPH(const boost::property_tree::ptree &root, SPHParameters &param)
        {
            param.gsph.is_2nd_order = root.get<bool>("use2ndOrderGSPH", false);
            param.gsph.force_correction = root.get<bool>("forceCorrection", false);
            WRITE_LOG << "use2ndOrderGSPH: " << param.gsph.is_2nd_order;
            WRITE_LOG << "forceCorrection: " << param.gsph.force_correction;
        }
        void parseGDISPH(const boost::property_tree::ptree &root, SPHParameters &param)
        {
            param.gsph.is_2nd_order = root.get<bool>("use2ndOrderGSPH", false);
            param.gsph.force_correction = root.get<bool>("forceCorrection", false);
            WRITE_LOG << "use2ndOrderGSPH: " << param.gsph.is_2nd_order;
            WRITE_LOG << "forceCorrection: " << param.gsph.force_correction;
        }
        const std::unordered_map<SPHType, ParserFunc> type_specific_parsers = {
            {SPHType::SSPH, parseSSPH},
            {SPHType::DISPH, parseDISPH},
            {SPHType::GDISPH, parseGDISPH},
            {SPHType::GSPH, parseGSPH}};
    }

    void Solver::parseJsonOverrides()
    {
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

        m_param->checkpoint_file = root.get<std::string>("checkpointFile", "");
        if (!m_param->checkpoint_file.empty())
        {
            WRITE_LOG << "Checkpoint file specified: " << m_param->checkpoint_file;
        }
        m_param->recenterParticles = root.get<bool>("recenterParticles", false);
        if (m_param->recenterParticles)
        {
            WRITE_LOG << "Recenter particles enabled.";
        }

        m_param->two_and_half_sim = root.get<bool>("two_and_half_sim", false);

        m_output_dir = root.get<std::string>("outputDirectory", m_output_dir);
        m_param->time.start = root.get<real>("startTime", real(0));
        WRITE_LOG << "startTime: " << m_param->time.start;

        m_param->time.end = root.get<real>("endTime");
        WRITE_LOG << "endTimeOutput directory: " << m_param->time.start;

        if (m_param->time.end < m_param->time.start)
        {
            THROW_ERROR("endTime < startTime");
        }
        m_param->time.output = root.get<real>("outputTime", (m_param->time.end - m_param->time.start) / 1000);
        m_param->time.energy = root.get<real>("energyTime", m_param->time.output);

        m_param->cfl.sound = root.get<real>("cflSound", 0.3);
        m_param->cfl.force = root.get<real>("cflForce", 0.125);

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

        m_param->ac.is_valid = root.get<bool>("useArtificialConductivity", false);
        if (m_param->ac.is_valid)
        {
            m_param->ac.alpha = root.get<real>("alphaAC", 1.0);
        }

        m_param->tree.max_level = root.get<int>("maxTreeLevel", 20);
        m_param->tree.leaf_particle_num = root.get<int>("leafParticleNumber", 1);

        m_param->physics.neighbor_number = root.get<int>("neighborNumber", 32);
        m_param->physics.gamma = root.get<real>("gamma");

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

        m_param->iterative_sml = root.get<bool>("iterativeSmoothingLength", true);

        m_param->periodic.is_valid = root.get<bool>("periodic", false);
        if (m_param->periodic.is_valid)
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

            auto &range_min = root.get_child("rangeMin");
            if (range_min.size() != DIM)
            {
                THROW_ERROR("rangeMin != DIM");
            }
            i = 0;
            for (auto &v : range_min)
            {
                m_param->periodic.range_min[i] = std::stod(v.second.data());
                ++i;
            }
        }
        m_param->density_relaxation.is_valid = root.get<bool>("useDensityRelaxation", false);
        if (m_param->density_relaxation.is_valid)
        {
            m_param->density_relaxation.max_iterations = root.get<int>("densityRelaxationMaxIter", 100);
            m_param->density_relaxation.damping_factor = root.get<real>("densityRelaxationDamping", 0.1);
            m_param->density_relaxation.velocity_threshold = root.get<real>("velocityThreshold", 1e-3);
            m_param->density_relaxation.table_file = root.get<std::string>("laneEmdenTable", "xi_theta.csv");
        }

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
        else if (sph_type == "gdisph")
        {
            m_param->type = SPHType::GDISPH;
        }
        else
        {
            THROW_ERROR("Unknown SPH type");
        }
        WRITE_LOG << "Using algorithm: " << sph::sphTypeToString(m_param->type);

        auto it = type_specific_parsers.find(m_param->type);
        if (it != type_specific_parsers.end())
        {
            it->second(root, *m_param);
        }

        m_param->heating_cooling.is_valid = root.get<bool>("useHeatingCooling", false);
        m_param->heating_cooling.heating_rate = root.get<real>("heatingRate", m_param->heating_cooling.heating_rate);
        m_param->heating_cooling.cooling_rate = root.get<real>("coolingRate", m_param->heating_cooling.cooling_rate);

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
        std::string sph_type_str = sphTypeToString(m_param->type);
        std::string dim_str;
        if (m_param->two_and_half_sim)
        {
            dim_str = "2.5D";
        }
        else
        {
            dim_str = std::to_string(DIM) + "D";
        }

        boost::filesystem::path output_path(m_output_dir);
        output_path /= sph_type_str;
        output_path /= m_sample_name;
        output_path /= dim_str;
        std::string full_output_dir = output_path.string();
        WRITE_LOG << "Output directory: " << full_output_dir;
        boost::filesystem::create_directories(output_path);
        m_sim = std::make_shared<Simulation>(m_param);
        m_output = std::make_shared<Output>(full_output_dir, 0, m_unit, m_param->recenterParticles);
        WRITE_LOG << "create_directories successfully: " << full_output_dir;

        if (!m_param->checkpoint_file.empty())
        {
            // If your sample (which sets the modifier) should be applied even with a checkpoint,
            // call it here. This ensures that the modifier gets attached.
            bool recognized = SampleRegistry::instance().create_sample(m_sample_name, m_sim, m_param);
            WRITE_LOG << "Sample (for checkpoint modifier) recognized: " << (recognized ? "yes" : "no");

            WRITE_LOG << "Reading checkpoint_file: " << m_param->checkpoint_file;
            m_output->read_checkpoint(m_param->checkpoint_file, m_sim);
            WRITE_LOG << "Initialized simulation from checkpoint: " << m_param->checkpoint_file;
        }
        else
        {
            std::cout << "Attempting to create sample: " << m_sample_name << "\n";
            bool recognized = SampleRegistry::instance().create_sample(m_sample_name, m_sim, m_param);
            std::cout << "Sample recognized: " << (recognized ? "yes" : "no") << "\n";

            if (!recognized)
            {
                THROW_ERROR("No recognized sample named ", m_sample_name,
                            " (and no code to fill from JSON-based ICs).");
            }
            m_output->output_particle(m_sim);
            m_output->output_energy(m_sim);

            // Updated Density Relaxation Parsing
            if (m_param->density_relaxation.is_valid)
            {

                // Allocate the LaneEmdenRelaxation object once
                m_laneEmdenRelaxation = std::make_unique<LaneEmdenRelaxation>();
                // Load the table using the file name from JSON (see below)
                m_laneEmdenRelaxation->load_table(m_param->density_relaxation.table_file);
                // Then apply the relaxation force
                m_laneEmdenRelaxation->add_relaxation_force(m_sim, *m_param);
                WRITE_LOG << "Density relaxation: LaneEmden-based force applied.";
                WRITE_LOG << "Density relaxation enabled: max_iter=" << m_param->density_relaxation.max_iterations
                          << ", damping=" << m_param->density_relaxation.damping_factor
                          << ", velocity_threshold=" << m_param->density_relaxation.velocity_threshold;
            }
        }

        ModuleFactory &factory = ModuleFactory::instance();
        m_timestep = std::make_shared<TimeStep>();
        m_pre = factory.create_module(m_param->type, "PreInteraction");
        m_fforce = factory.create_module(m_param->type, "FluidForce");
        m_gforce = std::make_shared<GravityForce>();
        if (m_param->heating_cooling.is_valid)
        {
            m_hcool = std::make_shared<HeatingCoolingModule>();
        }

        m_timestep->initialize(m_param);
        m_pre->initialize(m_param);
        m_fforce->initialize(m_param);
        m_gforce->initialize(m_param);
        if (m_hcool)
        {
            m_hcool->initialize(m_param);
        }

        if (m_param->type == SPHType::GSPH || m_param->type == SPHType::GDISPH)
        {
            std::vector<std::string> names = {"grad_density", "grad_pressure", "grad_velocity_0"};
#if DIM >= 2
            names.push_back("grad_velocity_1");
#endif
#if DIM == 3
            names.push_back("grad_velocity_2");
#endif
            m_sim->add_vector_array(names);
        }

        auto &p = m_sim->get_particles();
        const int num = m_sim->get_particle_num();
        const real c_sound = m_param->physics.gamma * (m_param->physics.gamma - 1.0);

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            if (p[i].is_point_mass)
                continue;
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

        WRITE_LOG << "Initialization complete. Particle count=" << m_sim->get_particle_num();
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
        if (m_hcool)
        {
            m_hcool->calculation(m_sim);
        }
        // --- Run Shock Detection ---
        // Call the detect_shocks function using the periodic boundary conditions from m_sim,
        // and use the adiabatic index (gamma) from the simulation parameters.
        sph::detect_shocks(m_sim, m_sim->get_periodic().get(), m_param->physics.gamma, 1.0);

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
            if (p[i].is_point_mass)
                continue;

            if (p[i].is_wall)
            {
                continue;
            }
            p[i].vel_p = p[i].vel + p[i].acc * (0.5 * dt);
            p[i].ene_p = p[i].ene + p[i].dene * (0.5 * dt);

            p[i].pos += p[i].vel_p * dt;
            p[i].vel += p[i].acc * dt;
            p[i].ene += p[i].dene * dt;
            p[i].sound = std::sqrt(c_sound * p[i].ene);

            periodic->apply(p[i].pos);

            if (m_param->two_and_half_sim)
            {
                p[i].pos[2] = 0.0;
                p[i].vel[2] = 0.0;
            }
        }
        // Optionally re-apply LaneEmden-based relaxation if user wants it each step:
        if (m_param->density_relaxation.is_valid && m_laneEmdenRelaxation)
        {
            m_laneEmdenRelaxation->add_relaxation_force(m_sim, *m_param);
            WRITE_LOG << "Density relaxation: LaneEmden-based force (predict step).";
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
            if (p[i].is_point_mass)
                continue;

            if (p[i].is_wall)
                continue;
            p[i].vel = p[i].vel_p + p[i].acc * (0.5 * dt);
            p[i].ene = p[i].ene_p + p[i].dene * (0.5 * dt);
            p[i].sound = std::sqrt(c_sound * p[i].ene);
            if (m_param->two_and_half_sim)
            {
                p[i].pos[2] = 0.0;
                p[i].vel[2] = 0.0;
            }
        }
        if (m_param->density_relaxation.is_valid && m_laneEmdenRelaxation)
        {
            m_laneEmdenRelaxation->add_relaxation_force(m_sim, *m_param);
            WRITE_LOG << "Density relaxation: LaneEmden-based force (predict step).";
        }
    }
} // namespace sph
