#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

class ModuleRegistry;

class IModule {
public:
    friend class ModuleRegistry;

    virtual ~IModule() = default;

    virtual const std::string getType() const = 0;
    virtual const std::string getName() const = 0;

protected:
    ModuleRegistry* registry = nullptr;
};

// --- ModuleRegistry ---
class ModuleRegistry {
public:
    void addModule(IModule* m) {
        if (!m) throw std::invalid_argument("Module is null");

        const std::string& name = m->getName();
        const std::string& type = m->getType();

        // Prevent duplicate names
        if (nameType.find(name) != nameType.end()) {
            throw std::runtime_error("Module with name '" + name + "' already registered.");
        }

        // Inject registry into module
        m->registry = this;

        // Register module
        modules[type].push_back(m);
        nameType[name] = type;
    }

    std::vector<IModule*> getModulesByType(const std::string& type) const {
        auto it = modules.find(type);
        if (it != modules.end()) return it->second;
        return {};
    }

    IModule* getModuleByName(const std::string& name) const {
        auto it = nameType.find(name);
        if (it == nameType.end()) return nullptr;

        const std::string& type = it->second;
        auto modList = getModulesByType(type);

        for (auto* mod : modList) {
            if (mod->getName() == name) {
                return mod;
            }
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, std::vector<IModule*>> modules;
    std::unordered_map<std::string, std::string> nameType;
};