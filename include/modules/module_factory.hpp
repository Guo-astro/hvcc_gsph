#pragma once

#include <memory>
#include <unordered_map>
#include <functional>
#include "modules/module.hpp"
#include "core/parameters.hpp"
#include "utilities/exception.hpp"

namespace sph
{

    class ModuleFactory
    {
    public:
        using ModuleCreator = std::function<std::shared_ptr<Module>()>;

        // Singleton access
        static ModuleFactory &instance()
        {
            static ModuleFactory factory;
            return factory;
        }

        // Register a module creator for a specific SPH type and module type
        void register_module(SPHType type, const std::string &module_type, ModuleCreator creator)
        {
            registry_[{type, module_type}] = creator;
        }

        // Create a module based on SPH type and module type
        std::shared_ptr<Module> create_module(SPHType type, const std::string &module_type) const
        {
            auto key = std::make_pair(type, module_type);
            auto it = registry_.find(key);
            if (it == registry_.end())
            {
                THROW_ERROR("No module registered for SPHType: ", static_cast<int>(type), " and module type: ", module_type);
            }
            return it->second();
        }

    private:
        ModuleFactory() = default;

        // Hash function for std::pair
        struct PairHash
        {
            std::size_t operator()(const std::pair<SPHType, std::string> &p) const
            {
                return std::hash<int>()(static_cast<int>(p.first)) ^ std::hash<std::string>()(p.second);
            }
        };

        std::unordered_map<std::pair<SPHType, std::string>, ModuleCreator, PairHash> registry_;
    };

// Macro to register modules
#define REGISTER_MODULE(SPH_TYPE, MODULE_TYPE, CLASS_NAME)                                                                     \
    static struct Register_##CLASS_NAME                                                                                        \
    {                                                                                                                          \
        Register_##CLASS_NAME()                                                                                                \
        {                                                                                                                      \
            ModuleFactory::instance().register_module(SPH_TYPE, MODULE_TYPE, []() { return std::make_shared<CLASS_NAME>(); }); \
        }                                                                                                                      \
    } g_register_##CLASS_NAME;

} // namespace sph