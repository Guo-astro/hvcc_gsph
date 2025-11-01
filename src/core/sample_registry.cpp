#include "core/sample_registry.hpp"
#include "utilities/exception.hpp" // for THROW_ERROR
#include <algorithm>
#include <vector>

namespace sph
{

    SampleRegistry &SampleRegistry::instance()
    {
        static SampleRegistry s_instance;
        return s_instance;
    }

    void SampleRegistry::register_sample(const std::string &name, SampleCreateFunc func)
    {
        std::cout << "Registering sample: " << name << "\n";
        m_registry[name] = func;
    }
    
    void SampleRegistry::register_source_files(const std::string &name, const std::vector<std::string>& files)
    {
        m_source_files[name] = files;
    }

    bool SampleRegistry::create_sample(const std::string &name,
                                       std::shared_ptr<Simulation> sim,
                                       std::shared_ptr<SPHParameters> param) const
    {
        auto it = m_registry.find(name);
        if (it == m_registry.end())
        {
            return false; // or throw
        }
        // Call the function pointer
        it->second(sim, param);
        return true;
    }

    std::vector<std::string> SampleRegistry::get_all_samples() const
    {
        std::vector<std::string> names;
        names.reserve(m_registry.size());
        
        for (const auto& pair : m_registry) {
            names.push_back(pair.first);
        }
        
        // Sort alphabetically for nicer output
        std::sort(names.begin(), names.end());
        
        return names;
    }
    
    std::vector<std::string> SampleRegistry::get_source_files(const std::string &name) const
    {
        auto it = m_source_files.find(name);
        if (it != m_source_files.end()) {
            return it->second;
        }
        return {};  // Return empty vector if not found
    }

} // namespace sph
