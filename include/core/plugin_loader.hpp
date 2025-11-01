#pragma once

#include <string>
#include <memory>
#include <dlfcn.h>  // For dynamic library loading on Unix
#include "core/simulation_plugin.hpp"
#include "utilities/exception.hpp"

namespace sph {

/**
 * @brief Loads and manages simulation plugins
 * 
 * Handles dynamic loading of shared libraries containing simulation cases.
 */
class PluginLoader {
public:
    PluginLoader() : m_handle(nullptr), m_plugin(nullptr) {}
    
    ~PluginLoader() {
        unload();
    }
    
    /**
     * @brief Load a plugin from a shared library file
     * 
     * @param plugin_path Path to the .so/.dylib file
     * @return Pointer to the loaded plugin
     */
    SimulationPlugin* load(const std::string& plugin_path) {
        // Unload any existing plugin
        unload();
        
        // Load the shared library
        m_handle = dlopen(plugin_path.c_str(), RTLD_LAZY);
        if (!m_handle) {
            THROW_ERROR("Failed to load plugin: " + plugin_path + "\nError: " + std::string(dlerror()));
        }
        
        // Clear any existing error
        dlerror();
        
        // Get the create_plugin function
        CreatePluginFunc create_func = (CreatePluginFunc)dlsym(m_handle, "create_plugin");
        const char* dlsym_error = dlerror();
        if (dlsym_error) {
            dlclose(m_handle);
            m_handle = nullptr;
            THROW_ERROR("Cannot load create_plugin function: " + std::string(dlsym_error));
        }
        
        // Create the plugin instance
        m_plugin = create_func();
        if (!m_plugin) {
            dlclose(m_handle);
            m_handle = nullptr;
            THROW_ERROR("create_plugin() returned nullptr");
        }
        
        m_plugin_path = plugin_path;
        return m_plugin;
    }
    
    /**
     * @brief Unload the current plugin
     */
    void unload() {
        if (m_plugin && m_handle) {
            // Get the destroy function
            DestroyPluginFunc destroy_func = (DestroyPluginFunc)dlsym(m_handle, "destroy_plugin");
            if (destroy_func) {
                destroy_func(m_plugin);
            } else {
                delete m_plugin;  // Fallback
            }
            m_plugin = nullptr;
        }
        
        if (m_handle) {
            dlclose(m_handle);
            m_handle = nullptr;
        }
    }
    
    /**
     * @brief Get the currently loaded plugin
     */
    SimulationPlugin* get_plugin() const {
        return m_plugin;
    }
    
    /**
     * @brief Get the path to the loaded plugin
     */
    std::string get_plugin_path() const {
        return m_plugin_path;
    }
    
    /**
     * @brief Check if a plugin is loaded
     */
    bool is_loaded() const {
        return m_plugin != nullptr;
    }
    
private:
    void* m_handle;                // Dynamic library handle
    SimulationPlugin* m_plugin;    // Plugin instance
    std::string m_plugin_path;     // Path to loaded plugin
};

} // namespace sph
