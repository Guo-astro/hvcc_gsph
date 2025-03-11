#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include "parameters.hpp" // for SPHParameters
#include "simulation.hpp" // for Simulation

namespace sph
{
    // Type of function that creates the initial condition:
    //   void func(std::shared_ptr<Simulation> sim, std::shared_ptr<SPHParameters> param);
    using SampleCreateFunc = std::function<void(std::shared_ptr<Simulation>, std::shared_ptr<SPHParameters>)>;

    ///
    /// A registry that maps "sampleName" -> "function pointer"
    ///
    class SampleRegistry
    {
    public:
        // Returns a singleton instance
        static SampleRegistry &instance();

        // Register a sample name => creation function
        void register_sample(const std::string &name, SampleCreateFunc func);

        // Create the sample by name. If name is not found, returns false or throws
        bool create_sample(const std::string &name,
                           std::shared_ptr<Simulation> sim,
                           std::shared_ptr<SPHParameters> param) const;

    private:
        // Private constructor for singleton
        SampleRegistry() = default;

        // The map
        std::unordered_map<std::string, SampleCreateFunc> m_registry;
    };

} // namespace sph

///
/// A helper macro so that you can just write:
///   REGISTER_SAMPLE("my_shock_tube", my_shock_tube_func);
/// in your sample .cpp file.
///
/// Explanation:
///  1) We declare a static struct that, in its constructor, calls
///     SampleRegistry::instance().register_sample(name, function).
///  2) This means merely #including sample_registry.hpp and writing
///     REGISTER_SAMPLE(...) in the .cpp is enough to register the sample at startup.
///
#define REGISTER_SAMPLE(NAME, FUNC)                                        \
    namespace                                                              \
    {                                                                      \
        /* Force a unique static boolean so it can’t get optimized out. */ \
        static const bool s_registered_##FUNC = []() {                            \
        std::cout << "Registering sample: " << NAME << "\n";                \
        sph::SampleRegistry::instance().register_sample(NAME, FUNC);         \
        return true; }();                \
    } /* end anonymous namespace */