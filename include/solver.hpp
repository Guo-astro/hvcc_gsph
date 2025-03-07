#pragma once
#include <memory>
#include <string>
#include <vector>
#include "unit_system.hpp"
#include "module.hpp"

namespace sph
{

    class Simulation;
    class Output;
    struct SPHParameters;

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
        std::shared_ptr<SPHParameters> m_param;
        std::shared_ptr<Output> m_output;
        std::shared_ptr<Simulation> m_sim;

        // Modules
        std::shared_ptr<Module> m_timestep;
        std::shared_ptr<Module> m_pre;
        std::shared_ptr<Module> m_fforce;
        std::shared_ptr<Module> m_gforce;
        std::shared_ptr<Module> m_hcool;

        std::string m_sample_name; // e.g. "shock_tube"
        std::string m_json_file;   // e.g. "shock_tube.json" or ""
        int m_num_threads;         // e.g. 4 if user typed that
        bool m_sample_recognized;  // store if the sample was recognized

        UnitSystem m_unit;
        std::string m_output_dir;
    };

} // namespace sph
