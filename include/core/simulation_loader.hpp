#pragma once

#include <memory>
#include <string>
#include "core/simulation.hpp"
#include "core/parameters.hpp"
#include "core/plugin_loader.hpp"
#include "core/sample_registry.hpp"

namespace sph {

/**
 * @brief Helper class to load simulations from either plugins or registry
 * 
 * Supports two modes:
 * 1. Plugin mode: Load .dylib/.so file with SimulationPlugin
 * 2. Registry mode: Use built-in SampleRegistry (backward compatibility)
 */
class SimulationLoader {
public:
    SimulationLoader() {}
    
    /**
     * @brief Load and initialize a simulation
     * 
     * Automatically detects if the name is a plugin path or a registered sample name.
     * 
     * @param name_or_path Either a sample name ("shock_tube") or plugin path ("../simulations/shock_tube/build/shock_tube_plugin.dylib")
     * @param sim The simulation object to populate
     * @param params The SPH parameters
     * @return true if successfully loaded and initialized
     */
    bool load_and_initialize(const std::string& name_or_path,
                            std::shared_ptr<Simulation> sim,
                            std::shared_ptr<SPHParameters> params) {
        // Check if it's a plugin path (ends with .dylib or .so)
        if (is_plugin_path(name_or_path)) {
            return load_from_plugin(name_or_path, sim, params);
        } else {
            return load_from_registry(name_or_path, sim, params);
        }
    }
    
    /**
     * @brief Get the name of the loaded simulation
     */
    std::string get_simulation_name() const {
        return m_simulation_name;
    }
    
    /**
     * @brief Get source files for archiving
     */
    std::vector<std::string> get_source_files() const {
        return m_source_files;
    }
    
    /**
     * @brief Check if loaded from plugin
     */
    bool is_plugin_loaded() const {
        return m_plugin_loader.is_loaded();
    }
    
    /**
     * @brief Get the plugin (if loaded)
     */
    SimulationPlugin* get_plugin() const {
        return m_plugin_loader.get_plugin();
    }

private:
    bool is_plugin_path(const std::string& path) const {
        return (path.find(".dylib") != std::string::npos ||
                path.find(".so") != std::string::npos);
    }
    
    bool load_from_plugin(const std::string& plugin_path,
                         std::shared_ptr<Simulation> sim,
                         std::shared_ptr<SPHParameters> params) {
        try {
            auto* plugin = m_plugin_loader.load(plugin_path);
            if (!plugin) {
                return false;
            }
            
            m_simulation_name = plugin->get_name();
            m_source_files = plugin->get_source_files();
            
            std::cout << "\n=== Loaded Plugin ===" << std::endl;
            std::cout << "Name: " << plugin->get_name() << std::endl;
            std::cout << "Description: " << plugin->get_description() << std::endl;
            std::cout << "Version: " << plugin->get_version() << std::endl;
            std::cout << "=====================\n" << std::endl;
            
            plugin->initialize(sim, params);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error loading plugin: " << e.what() << std::endl;
            return false;
        }
    }
    
    bool load_from_registry(const std::string& sample_name,
                           std::shared_ptr<Simulation> sim,
                           std::shared_ptr<SPHParameters> params) {
        m_simulation_name = sample_name;
        m_source_files = SampleRegistry::instance().get_source_files(sample_name);
        
        return SampleRegistry::instance().create_sample(sample_name, sim, params);
    }
    
    PluginLoader m_plugin_loader;
    std::string m_simulation_name;
    std::vector<std::string> m_source_files;
};

} // namespace sph
