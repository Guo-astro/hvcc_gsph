#pragma once

#include <memory>
#include <string>
#include "core/simulation.hpp"
#include "core/parameters.hpp"

namespace sph {

/**
 * @brief Interface for simulation plugins
 * 
 * Each simulation case implements this interface and exports it as a plugin.
 * The main executable can dynamically load plugins at runtime.
 */
class SimulationPlugin {
public:
    virtual ~SimulationPlugin() = default;
    
    /**
     * @brief Get the name of this simulation
     */
    virtual std::string get_name() const = 0;
    
    /**
     * @brief Get description of this simulation
     */
    virtual std::string get_description() const = 0;
    
    /**
     * @brief Get version of this simulation
     */
    virtual std::string get_version() const = 0;
    
    /**
     * @brief Initialize the simulation with particles
     * 
     * This is the main function that sets up initial conditions.
     * 
     * @param sim The simulation object to populate
     * @param params The SPH parameters
     */
    virtual void initialize(std::shared_ptr<Simulation> sim, 
                          std::shared_ptr<SPHParameters> params) = 0;
    
    /**
     * @brief Get list of source files used by this plugin
     * 
     * Returns paths to .cpp files that should be archived with results
     */
    virtual std::vector<std::string> get_source_files() const = 0;
};

} // namespace sph

/**
 * @brief Plugin entry point
 * 
 * Every simulation plugin must export a C function that creates the plugin instance:
 * 
 * extern "C" sph::SimulationPlugin* create_plugin() {
 *     return new MySimulationPlugin();
 * }
 * 
 * extern "C" void destroy_plugin(sph::SimulationPlugin* plugin) {
 *     delete plugin;
 * }
 */
typedef sph::SimulationPlugin* (*CreatePluginFunc)();
typedef void (*DestroyPluginFunc)(sph::SimulationPlugin*);

// Macros to simplify plugin creation
#define EXPORT_PLUGIN_API extern "C"

#define DEFINE_SIMULATION_PLUGIN(ClassName) \
    EXPORT_PLUGIN_API sph::SimulationPlugin* create_plugin() { \
        return new ClassName(); \
    } \
    EXPORT_PLUGIN_API void destroy_plugin(sph::SimulationPlugin* plugin) { \
        delete plugin; \
    }
