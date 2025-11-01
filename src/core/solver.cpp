/* ================================
 * solver.cpp
 * ================================ */
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <set>
#include <omp.h>
#include <filesystem>
#include <csignal>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/any.hpp>
#include <boost/filesystem.hpp>

#include "modules/module_factory.hpp"
#include "core/solver.hpp"
#include "core/parameters.hpp"
#include "core/particle.hpp"
#include "core/logger.hpp"
#include "utilities/exception.hpp"
#include "core/simulation_run.hpp"
#include "core/output_format.hpp"
#include "core/simulation.hpp"
#include "utilities/periodic.hpp"
#include "tree/bhtree.hpp"
#include "modules/timestep.hpp"
#include "modules/pre_interaction.hpp"
#include "modules/fluid_force.hpp"
#include "modules/gravity_force.hpp"
#include "core/sample_registry.hpp"

// DISPH
#include "algorithms/disph/d_pre_interaction.hpp"
#include "algorithms/disph/d_fluid_force.hpp"

// GSPH
#include "algorithms/gsph/g_pre_interaction.hpp"
#include "algorithms/gsph/g_fluid_force.hpp"

// GDISPH
#include "algorithms/gdisph/gdi_pre_interaction.hpp"
#include "algorithms/gdisph/gdi_fluid_force.hpp"

// Heating and Cooling module
#include "modules/heating_cooling.hpp"

// Unit system and density relaxation helper
#include "utilities/unit_system.hpp"
#include "relaxation/density_relaxation.hpp"
#include "utilities/shock_detection.hpp"

namespace sph
{
    // Global signal handling state
    namespace
    {
        volatile std::sig_atomic_t g_interrupt_requested = 0;
        
        void signal_handler(int signal)
        {
            if (signal == SIGINT)
            {
                g_interrupt_requested = 1;
            }
        }
    }
    
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
                factory.register_module(SPHType::GDISPH, "PreInteraction", []()                                        { return std::make_shared<sph::gdisph::PreInteraction>(); });
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
          m_output_dir("../simulations"),  // Changed: Use project root simulations directory
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

    // **Config Inheritance Helper**
    namespace
    {
        /**
         * @brief Recursively load JSON config with inheritance support
         * 
         * If the JSON has an "extends" field, it loads the parent config first
         * and merges child values on top (child overrides parent).
         * 
         * @param file_path Path to JSON config file
         * @param visited Set of already-loaded files (to detect cycles)
         * @return Merged property tree
         */
        boost::property_tree::ptree load_json_with_extends(
            const std::string& file_path,
            std::set<std::string>& visited)
        {
            namespace pt = boost::property_tree;
            namespace fs = boost::filesystem;
            
            // Get canonical path to handle relative paths and detect cycles
            fs::path canonical_path = fs::canonical(file_path);
            std::string canonical_str = canonical_path.string();
            
            // Check for circular dependencies
            if (visited.count(canonical_str)) {
                THROW_ERROR("Circular config inheritance detected: ", canonical_str);
            }
            visited.insert(canonical_str);
            
            // Load the current config
            pt::ptree current;
            try {
                pt::read_json(file_path, current);
            }
            catch (std::exception &e) {
                THROW_ERROR("Cannot read JSON file: ", file_path, " => ", e.what());
            }
            
            // Check if it extends another config
            std::string extends_path = current.get<std::string>("extends", "");
            if (extends_path.empty()) {
                // No parent, return as-is
                return current;
            }
            
            // Resolve relative path relative to current config's directory
            fs::path current_dir = fs::path(file_path).parent_path();
            fs::path parent_path = current_dir / extends_path;
            
            WRITE_LOG << "Config " << file_path << " extends " << parent_path.string();
            
            // Recursively load parent
            pt::ptree parent = load_json_with_extends(parent_path.string(), visited);
            
            // Merge: iterate over current config and override parent values
            for (auto& item : current) {
                if (item.first == "extends") {
                    // Skip the "extends" field itself
                    continue;
                }
                // Override or add to parent
                parent.put_child(item.first, item.second);
            }
            
            return parent;
        }
        
        /**
         * @brief Load JSON config with inheritance support (public interface)
         * 
         * @param file_path Path to JSON config file  
         * @return Merged property tree
         */
        boost::property_tree::ptree load_json_with_extends(const std::string& file_path)
        {
            std::set<std::string> visited;
            return load_json_with_extends(file_path, visited);
        }
    }

    void Solver::parseJsonOverrides()
    {
        if (m_json_file.empty())
        {
            return;
        }

        namespace pt = boost::property_tree;
        pt::ptree root;
        
        // Load JSON with inheritance support
        root = load_json_with_extends(m_json_file);

        // Parse output configuration (for new SimulationRun system)
        // Note: This is parsed here but used in initialize() when creating SimulationRun
        // We'll store it temporarily in member variables (to be added to Solver class)
        
        m_param->initial_conditions_file = root.get<std::string>("initialConditionsFile", "");
        
        if (!m_param->initial_conditions_file.empty())
        {
            WRITE_LOG << "Initial conditions file specified: " << m_param->initial_conditions_file;
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
        m_param->cfl.ene = root.get<real>("cflEnergy", 0.3);
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
        
        // Parse resume configuration
        m_param->resume.enabled = root.get<bool>("resumeFromCheckpoint", false);
        if (m_param->resume.enabled)
        {
            m_param->resume.checkpoint_file = root.get<std::string>("resumeCheckpointFile", "");
            if (m_param->resume.checkpoint_file.empty())
            {
                THROW_ERROR("resumeFromCheckpoint=true but resumeCheckpointFile not specified");
            }
            WRITE_LOG << "Resume enabled from: " << m_param->resume.checkpoint_file;
        }
        
        // Parse auto-checkpoint configuration
        m_param->checkpointing.enabled = root.get<bool>("enableCheckpointing", false);
        if (m_param->checkpointing.enabled)
        {
            m_param->checkpointing.interval = root.get<real>("checkpointInterval", 1.0);
            m_param->checkpointing.max_keep = root.get<int>("checkpointMaxKeep", 3);
            m_param->checkpointing.on_interrupt = root.get<bool>("checkpointOnInterrupt", true);
            m_param->checkpointing.directory = root.get<std::string>("checkpointDirectory", "checkpoints");
            
            WRITE_LOG << "Auto-checkpoint enabled:";
            WRITE_LOG << "  Interval: " << m_param->checkpointing.interval;
            WRITE_LOG << "  Max keep: " << m_param->checkpointing.max_keep;
            WRITE_LOG << "  Directory: " << m_param->checkpointing.directory;
        }
    }

    void Solver::run()
    {
        initialize();
        
        // Register signal handler for graceful interruption with checkpoint save
        if (m_param->checkpointing.enabled && m_param->checkpointing.on_interrupt)
        {
            std::signal(SIGINT, signal_handler);
            WRITE_LOG << "Signal handler registered (Ctrl+C will save checkpoint before exit)";
        }
        
        // Handle checkpoint resume if enabled
        if (m_param->resume.enabled && !m_param->resume.checkpoint_file.empty())
        {
            WRITE_LOG << "Resuming from checkpoint: " << m_param->resume.checkpoint_file;
            
            if (!m_checkpoint_manager)
            {
                // Create a temporary checkpoint manager for loading
                CheckpointManager::AutoCheckpointConfig config;
                m_checkpoint_manager = std::make_unique<CheckpointManager>(config);
            }
            
            CheckpointData data = m_checkpoint_manager->load_checkpoint(m_param->resume.checkpoint_file);
            restore_from_checkpoint(data);
            
            WRITE_LOG << "Resume complete. Starting from t=" << m_sim->get_time();
        }
        
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
            // Check for interrupt signal
            if (g_interrupt_requested)
            {
                WRITE_LOG << "\n*** Interrupt signal received (Ctrl+C) ***";
                
                if (m_checkpoint_manager && m_param->checkpointing.on_interrupt)
                {
                    std::string checkpoint_path = m_checkpoint_manager->generate_checkpoint_path(
                        m_simulation_run->get_run_directory(), t
                    );
                    
                    WRITE_LOG << "Saving interrupt checkpoint at t=" << t << " to " << checkpoint_path;
                    m_checkpoint_manager->save_checkpoint(checkpoint_path, *m_sim, *m_param);
                    WRITE_LOG << "Checkpoint saved successfully.";
                    WRITE_LOG << "Resume with: \"resumeFromCheckpoint\": true, \"resumeCheckpointFile\": \"" 
                              << checkpoint_path << "\"";
                }
                
                WRITE_LOG << "Exiting gracefully...";
                break;
            }
            
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
                for (auto& writer : m_output_writers)
                {
                    writer->write_snapshot(m_sim);
                }
                t_out += m_param->time.output;
            }
            
            // Auto-checkpoint if enabled
            if (m_checkpoint_manager && m_checkpoint_manager->should_checkpoint(t))
            {
                std::string checkpoint_path = m_checkpoint_manager->generate_checkpoint_path(
                    m_simulation_run->get_run_directory(), t
                );
                
                WRITE_LOG << "Saving checkpoint at t=" << t << " to " << checkpoint_path;
                m_checkpoint_manager->save_checkpoint(checkpoint_path, *m_sim, *m_param);
            }
            
            if (t > t_ene)
            {
                // TODO: Implement energy output with new system
                // For now, skip energy output during refactoring
                t_ene += m_param->time.energy;
            }
        }
        const auto end = std::chrono::system_clock::now();
        const real calctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        WRITE_LOG << "\ncalculation is finished";
        WRITE_LOG << "calculation time: " << calctime << " ms";

        // Generate and save metadata with performance metrics
        MetadataGenerator::RunInfo run_info;
        run_info.run_id = m_simulation_run->get_run_id();
        run_info.sample_name = m_sample_name;
        run_info.description = "";
        run_info.duration_seconds = calctime / 1000.0;
        run_info.user = "";
        run_info.hostname = "";

        MetadataGenerator::CodeVersion code_version;
        // Will be filled by MetadataGenerator

        MetadataGenerator::SimulationParams sim_params;
        sim_params.sph_type = sphTypeToString(m_param->type);
        sim_params.dimension = DIM;
        sim_params.particle_count = m_sim->get_particle_num();
        sim_params.gamma = m_param->physics.gamma;
        sim_params.cfl_sound = m_param->cfl.sound;
        sim_params.end_time = m_param->time.end;
        sim_params.output_interval = m_param->time.output;
        sim_params.neighbor_number = m_param->physics.neighbor_number;
        sim_params.use_balsara = m_param->av.use_balsara_switch;
        sim_params.use_time_dependent_av = m_param->av.use_time_dependent_av;

        MetadataGenerator::OutputInfo output_info;
        for (auto format : m_simulation_run->get_config().output_formats)
        {
            if (format == OutputFormat::CSV) output_info.formats.push_back("csv");
            else if (format == OutputFormat::BINARY) output_info.formats.push_back("binary");
            else if (format == OutputFormat::NUMPY) output_info.formats.push_back("numpy");
        }
        output_info.snapshot_count = 0;  // TODO: Track this
        output_info.total_output_size_mb = 0.0;  // TODO: Calculate this

        MetadataGenerator::PerformanceInfo perf_info;
        perf_info.total_timesteps = loop;
        perf_info.wall_time_seconds = calctime / 1000.0;
        perf_info.timesteps_per_second = loop / (calctime / 1000.0);
        perf_info.particles_per_second = m_sim->get_particle_num() * loop / (calctime / 1000.0);

        std::string metadata_json = MetadataGenerator::generate(
            run_info, code_version, sim_params, m_unit, output_info, perf_info
        );
        m_simulation_run->save_metadata(metadata_json);
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

        // Create simulation first
        m_sim = std::make_shared<Simulation>(m_param);
        
        // Initialize simulation loader early to get proper simulation name
        m_simulation_loader = std::make_unique<SimulationLoader>();
        bool loaded = m_simulation_loader->load_and_initialize(m_sample_name, m_sim, m_param);
        
        if (!loaded)
        {
            THROW_ERROR("Failed to load simulation: ", m_sample_name,
                        "\nTry one of:\n",
                        "  - A plugin path (e.g., ../simulations/shock_tube/build/libshock_tube_plugin.dylib)\n",
                        "  - A registered sample name (e.g., shock_tube)");
        }
        
        // Use the actual simulation name (not the plugin path)
        std::string actual_sample_name = m_simulation_loader->get_simulation_name();
        
        std::cout << "Attempting to load simulation: " << m_sample_name << "\n";
        std::cout << "Simulation loaded: yes\n";
        
        if (m_simulation_loader->is_plugin_loaded())
        {
            std::cout << "Loaded as plugin\n";
            std::cout << "  Plugin path: " << m_sample_name << "\n";
            std::cout << "  Simulation name: " << actual_sample_name << "\n";
        }
        else
        {
            std::cout << "Loaded from registry: " << actual_sample_name << "\n";
        }

        // Create SimulationRun with new output system
        SimulationRun::Config run_config;
        run_config.sample_name = actual_sample_name;  // Use actual name, not plugin path
        run_config.base_dir = m_output_dir;
        run_config.sph_type = sph_type_str;
        run_config.dimension = DIM;
        run_config.auto_run_id = true;
        run_config.save_initial_conditions = true;
        run_config.save_config = !m_json_file.empty();
        run_config.save_metadata = true;
        run_config.save_source_code = true;
        run_config.output_formats = {OutputFormat::CSV, OutputFormat::BINARY};  // Default to both formats
        
        // Get source files for this sample (optional - if empty, entire src/samples tree is saved)
        // run_config.source_files = SampleRegistry::instance().get_source_files(m_sample_name);
        // Leave source_files empty to copy entire samples directory tree

        m_simulation_run = std::make_unique<SimulationRun>(run_config);
        
        // Create output writers for each format
        for (auto format : run_config.output_formats)
        {
            m_output_writers.push_back(m_simulation_run->create_writer(format, m_unit));
        }
        
        WRITE_LOG << "Output directory: " << m_simulation_run->get_outputs_directory(OutputFormat::CSV);

        if (!m_param->initial_conditions_file.empty())
        {
            WRITE_LOG << "Loading initial conditions from: " << m_param->initial_conditions_file;
            
            // Create a temporary checkpoint manager for loading if needed
            if (!m_checkpoint_manager)
            {
                CheckpointManager::AutoCheckpointConfig config;
                m_checkpoint_manager = std::make_unique<CheckpointManager>(config);
            }
            
            // Load initial conditions using checkpoint format
            CheckpointData data = m_checkpoint_manager->load_checkpoint(m_param->initial_conditions_file);
            
            WRITE_LOG << "Loaded " << data.particles.size() << " particles from initial conditions";
            WRITE_LOG << "  Time: " << data.time;
            WRITE_LOG << "  Dimension: " << data.dimension;
            
            // Set particles in simulation
            m_sim->set_particles(data.particles);
            m_sim->set_time(data.time);
            m_sim->set_dt(data.dt);
            
            // Apply initial conditions modifier if set
            auto modifier = m_sim->get_initial_conditions_modifier();
            if (modifier)
            {
                WRITE_LOG << "Applying initial conditions modifier...";
                auto& particles = m_sim->get_particles();
                modifier->modifyParticles(particles, m_sim);
                WRITE_LOG << "Modifier applied. Final particle count: " << m_sim->get_particle_num();
            }
            
            WRITE_LOG << "Initial conditions loaded successfully";
        }
        else
        {
            // Simulation already loaded in the beginning of initialize()
            // Just save the initial state and source files

            // Save initial conditions using new system
            const auto& particles = m_sim->get_particles();
            m_simulation_run->save_initial_conditions(particles, m_unit);

            // Save config if provided
            if (!m_json_file.empty())
            {
                std::ifstream config_in(m_json_file);
                std::string config_content((std::istreambuf_iterator<char>(config_in)),
                                          std::istreambuf_iterator<char>());
                m_simulation_run->save_config(config_content);
            }
            
            // Save source code files for reproducibility
            m_simulation_run->save_source_files();

            // Write initial snapshot (t=0) using all output writers
            for (auto& writer : m_output_writers)
            {
                writer->write_snapshot(m_sim);
            }

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

        // Initialize checkpoint manager if checkpointing is enabled
        if (m_param->checkpointing.enabled)
        {
            CheckpointManager::AutoCheckpointConfig config;
            config.interval = m_param->checkpointing.interval;
            config.max_keep = m_param->checkpointing.max_keep;
            config.directory = m_param->checkpointing.directory;
            
            m_checkpoint_manager = std::make_unique<CheckpointManager>(config);
            WRITE_LOG << "Checkpoint manager initialized:";
            WRITE_LOG << "  Interval: " << config.interval;
            WRITE_LOG << "  Max keep: " << config.max_keep;
            WRITE_LOG << "  Directory: " << config.directory;
        }

        WRITE_LOG << "Initialization complete. Particle count=" << m_sim->get_particle_num();
    }

    void Solver::restore_from_checkpoint(const CheckpointData& data)
    {
        WRITE_LOG << "Restoring simulation from checkpoint...";
        WRITE_LOG << "  Time: " << data.time;
        WRITE_LOG << "  Timestep: " << data.dt;
        WRITE_LOG << "  Particles: " << data.particles.size();
        
        // Restore simulation state
        m_sim->set_time(data.time);
        m_sim->set_dt(data.dt);
        m_sim->set_particles(data.particles);
        
        // Rebuild spatial structures
        WRITE_LOG << "  Rebuilding tree...";
        m_sim->make_tree();
        
        WRITE_LOG << "  Finding neighbors...";
        m_pre->calculation(m_sim);  // Pre-interaction finds neighbors
        
        WRITE_LOG << "Checkpoint restore complete.";
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
        const real ene_min = 1e-10; // Minimum energy threshold

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            if (p[i].is_point_mass || p[i].is_wall)
                continue;

            p[i].vel_p = p[i].vel + p[i].acc * (0.5 * dt);
            p[i].ene_p = p[i].ene + p[i].dene * (0.5 * dt);

            p[i].pos += p[i].vel_p * dt;
            p[i].vel += p[i].acc * dt;
            p[i].ene += p[i].dene * dt;

            // Cap internal energy to prevent negative or very small values
            if (p[i].ene < ene_min)
            {
                p[i].ene = ene_min;
                p[i].ene_floored = 1; // Mark as floored
                WRITE_LOG << "Warning: ene floored to " << ene_min << " for particle " << i << " in predict step.";
            }

            p[i].sound = std::sqrt(c_sound * p[i].ene);

            periodic->apply(p[i].pos);

            if (m_param->two_and_half_sim)
            {
                p[i].pos[2] = 0.0;
                p[i].vel[2] = 0.0;
            }
        }

        // Density relaxation (unchanged)
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
        const real ene_min = 1e-10; // Minimum energy threshold

#pragma omp parallel for
        for (int i = 0; i < num; ++i)
        {
            if (p[i].is_point_mass || p[i].is_wall)
                continue;

            p[i].vel = p[i].vel_p + p[i].acc * (0.5 * dt);
            p[i].ene = p[i].ene_p + p[i].dene * (0.5 * dt);

            // Cap internal energy to prevent negative or very small values
            if (p[i].ene < ene_min)
            {
                p[i].ene = ene_min;
                WRITE_LOG << "Warning: ene floored to " << ene_min << " for particle " << i << " in correct step.";
            }

            p[i].sound = std::sqrt(c_sound * p[i].ene);

            if (m_param->two_and_half_sim)
            {
                p[i].pos[2] = 0.0;
                p[i].vel[2] = 0.0;
            }
        }

        // Density relaxation (unchanged)
        if (m_param->density_relaxation.is_valid && m_laneEmdenRelaxation)
        {
            m_laneEmdenRelaxation->add_relaxation_force(m_sim, *m_param);
            WRITE_LOG << "Density relaxation: LaneEmden-based force (predict step).";
        }
    }
} // namespace sph
