#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include <stdexcept>
#include <iostream>

class ModuleRegistry;

class IModule {
public:
    friend class ModuleRegistry;

    virtual ~IModule() = default;

    virtual const std::string getType() const = 0;
    virtual const std::string getName() const = 0;
    virtual void update() = 0;

protected:
    ModuleRegistry* registry = nullptr;
};

/**
 * @brief Module Registry
 * 
 * ModuleRegistry
 * ├─ groups : std::unordered_map<std::string, ModuleGroup>
 * │   ├─ "GameModule" : ModuleGroup
 * │   │     ├─ position : 0          // index of active module
 * │   │     └─ modules : [Game1, Game2, Game3]
 * │   ├─ "GraphicModule" : ModuleGroup
 * │   │     ├─ position : 1
 * │   │     └─ modules : [Graphic1, Graphic2, Graphic3]
 * │   └─ ...
 * 
 */
class ModuleRegistry {

public:

   void add(IModule* m) {
        if (!m) throw std::invalid_argument("Module null");

        auto& entry = modules[m->getType()];
        auto& vec = entry.second;

        if (std::find(vec.begin(), vec.end(), m) != vec.end())
            throw std::runtime_error("Module déjà présent: " + m->getName());

        vec.push_back(m);
        if (vec.size() == 1) entry.first = 0; // premier module = actif
        m->registry = this;
    }

   // Retrieve active module by type
    IModule* getActive(const std::string& type) const {
        auto it = modules.find(type);
        if (it == modules.end() || it->second.second.empty()) return nullptr;
        size_t pos = std::min(it->second.first, static_cast<size_t>(it->second.second.size() - 1));
        return it->second.second[pos];
    }

    // Retrieve modules by type
    std::vector<IModule*> get(const std::string& type) const {
        auto it = modules.find(type);
        if (it == modules.end()) return {};
        return it->second.second;
    }

    void setActive(const std::string& type, IModule* m) {
        auto it = modules.find(type);
        if (it == modules.end()) return; // type not found

        auto& vec = it->second.second;
        auto pos = std::find(vec.begin(), vec.end(), m);

        if (pos == vec.end()) return; //module not in this group

        it->second.first = static_cast<size_t>(std::distance(vec.begin(), pos));
    }

protected:

    std::unordered_map<
        std::string,
        std::pair<size_t,std::vector<IModule *>>> modules;
};