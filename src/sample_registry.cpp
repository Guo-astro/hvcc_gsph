#include "sample_registry.hpp"
#include "exception.hpp" // for THROW_ERROR

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

} // namespace sph
