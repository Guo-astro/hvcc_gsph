#pragma once
#include <memory>
#include <string>
#include <vector>
#include "utilities/unit_system.hpp"
#include "utilities/checkpoint_manager.hpp"
#include "modules/module.hpp"
#include "relaxation/lane_emden_relaxation.hpp"
#include "core/simulation_run.hpp"
#include "core/output_format.hpp"
#include "core/simulation_loader.hpp"

namespace sph
{

    class Simulation;
    struct SPHParameters;
    struct CheckpointData;

    class Solver
    {
    public:
        Solver(int argc, char *argv[]);
        void run();

    private:
        // parse arguments for sample, optional json, optional threads
        // void parse_arguments(int argc, char *argv[]);

        // parse JSON to override param fields
        void parseJsonOverrides();

        void initialize();
        void integrate();
        void predict();
        void correct();
        
        // Checkpoint/resume functionality
        void restore_from_checkpoint(const CheckpointData& data);
        
        std::shared_ptr<SPHParameters> m_param;
        std::unique_ptr<SimulationRun> m_simulation_run;
        std::vector<std::shared_ptr<OutputWriter>> m_output_writers;
        std::shared_ptr<Simulation> m_sim;

        // Modules
        std::shared_ptr<Module> m_timestep;
        std::shared_ptr<Module> m_pre;
        std::shared_ptr<Module> m_fforce;
        std::shared_ptr<Module> m_gforce;
        std::shared_ptr<Module> m_hcool;
        std::unique_ptr<LaneEmdenRelaxation> m_laneEmdenRelaxation;
        std::unique_ptr<SimulationLoader> m_simulation_loader;

        std::string m_sample_name; // e.g. "shock_tube" or path to plugin
        std::string m_json_file;   // e.g. "shock_tube.json" or ""
        int m_num_threads;         // e.g. 4 if user typed that
        bool m_sample_recognized;  // store if the sample was recognized

        UnitSystem m_unit;
        std::string m_output_dir;
        
        // Checkpoint manager for pause/resume functionality
        std::unique_ptr<CheckpointManager> m_checkpoint_manager;
    };

} // namespace sph
